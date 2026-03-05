/* sysmon draw.cpp — sysmon TUI renderer
   Copyright 2025 sysmon contributors, Apache-2.0 */

#include "../include/draw.hpp"
#include "../include/theme.hpp"
#include "../include/config.hpp"
#include "../include/cpu.hpp"
#include "../include/mem.hpp"
#include "../include/net.hpp"
#include "../include/proc.hpp"
#include "../include/sysmon.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace Draw {

int width{}, height{};
static struct termios orig_termios{};
static bool raw_mode = false;

// ═════════════════════════════════════════════════════════════════
// ANSI / UTF-8 HELPERS
// ═════════════════════════════════════════════════════════════════

static std::string mv(int x, int y) {
    return "\033[" + std::to_string(y) + ";" + std::to_string(x) + "H";
}
static const std::string RST  = "\033[0m";
static const std::string BOLD = "\033[1m";
static const std::string DIM  = "\033[2m";

static std::string u8(uint32_t cp) {
    std::string s;
    if (cp < 0x80)        { s += char(cp); }
    else if (cp < 0x800)  { s += char(0xC0|(cp>>6));  s += char(0x80|(cp&0x3F)); }
    else                  { s += char(0xE0|(cp>>12));  s += char(0x80|((cp>>6)&0x3F)); s += char(0x80|(cp&0x3F)); }
    return s;
}

// pad string LEFT-aligned to exactly n display columns (truncate/space-fill)
// i.e. the string comes first, then spaces
static std::string ljust(const std::string& s, int n) {
    if (n <= 0) return "";
    // Naive: assume 1 byte = 1 display col for ASCII.
    // For multibyte (UTF-8 box chars) we count codepoints via leading bytes.
    int vis = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) vis++;
    if (vis >= n) {
        // Truncate by codepoints
        std::string out; int cnt=0;
        for (size_t i=0; i<s.size() && cnt<n; i++) {
            unsigned char c = s[i];
            out += s[i];
            if ((c & 0xC0) != 0x80) cnt++;
        }
        return out;
    }
    return s + std::string(n - vis, ' ');
}

// pad string RIGHT-aligned to exactly n columns (leading spaces)
static std::string rjust(const std::string& s, int n) {
    if (n <= 0) return "";
    int vis = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) vis++;
    if (vis >= n) {
        std::string out; int cnt=0;
        for (int i=(int)s.size()-1; i>=0 && cnt<n; i--) {
            unsigned char c = s[i];
            if ((c & 0xC0) != 0x80) cnt++;
            out = s[i] + out;
        }
        return out;
    }
    return std::string(n - vis, ' ') + s;
}

// Fill a region with spaces (clears leftover chars from prev frame)
static void clr(std::ostringstream& o, int x, int y, int w, int h) {
    if (w<=0||h<=0) return;
    std::string blank(w, ' ');
    for (int r=0; r<h; r++) o << mv(x, y+r) << blank;
}

// ═════════════════════════════════════════════════════════════════
// BRAILLE GRAPH  (braille graph, newest sample = rightmost cell)
// Each terminal cell = 2 data-columns × 4 sub-rows
// Left  bits: 0x01 0x02 0x04 0x40  (bottom → top)
// Right bits: 0x08 0x10 0x20 0x80
// ═════════════════════════════════════════════════════════════════

static void graph(std::ostringstream& o,
                  int x, int y, int w, int h,
                  const std::vector<double>& data,
                  const std::string& c_lo, const std::string& c_hi="") {
    if (w<=0||h<=0) return;
    int need = w*2;
    std::vector<double> d;
    if ((int)data.size() >= need)
        d.assign(data.end()-need, data.end());
    else {
        d.assign(need - (int)data.size(), 0.0);
        d.insert(d.end(), data.begin(), data.end());
    }
    // Braille bit tables: index = number of sub-rows filled (0-4)
    static const uint8_t L4[5] = {0x00, 0x40, 0x44, 0x46, 0x47};
    static const uint8_t R4[5] = {0x00, 0x80, 0xa0, 0xb0, 0xb8};
    for (int row=0; row<h; row++) {
        o << mv(x, y+row);
        // This cell-row represents values from top% to bot%
        double bot = double(h-row)   / h * 100.0;
        double top = double(h-row-1) / h * 100.0;
        double span = bot - top;
        for (int col=0; col<w; col++) {
            // sub-row fill count for left and right data columns
            auto fill = [&](double v) -> int {
                double a = std::clamp(v, 0.0, 100.0) - top;
                if (a <= 0) return 0;
                return std::clamp((int)std::ceil(a/span*4.0), 0, 4);
            };
            int lh = fill(d[col*2]), rh = fill(d[col*2+1]);
            double t = (w>1) ? double(col)/(w-1) : 0.0;
            std::string clr_s = c_hi.empty() ? Theme::fg(c_lo) : Theme::gradient(c_lo,c_hi,t);
            if (lh==0 && rh==0)
                o << DIM << clr_s << u8(0x2800) << RST;
            else
                o << clr_s << u8(0x2800 | L4[lh] | R4[rh]) << RST;
        }
    }
}

// ═════════════════════════════════════════════════════════════════
// METER BAR  (solid ▌ blocks with gradient — CPU total bar)
// ═════════════════════════════════════════════════════════════════

static void meter(std::ostringstream& o, int x, int y, int w, double val,
                  const std::string& c1, const std::string& c2) {
    if (w<=0) return;
    int filled = std::clamp((int)std::round(val/100.0*w), 0, w);
    o << mv(x, y);
    for (int i=0; i<w; i++) {
        double t = (w>1) ? double(i)/(w-1) : 0.0;
        o << Theme::gradient(c1, c2, t);
        o << (i<filled ? "\u258a" : (DIM+"\u2592"+RST));
    }
    o << RST;
}

// ═════════════════════════════════════════════════════════════════
// DOT BAR  (thin dotted ▏ chars)
// ═════════════════════════════════════════════════════════════════

static void dotbar(std::ostringstream& o, int x, int y, int w, double val,
                   const std::string& c1, const std::string& c2) {
    if (w<=0) return;
    int filled = std::clamp((int)std::round(val/100.0*w), 0, w);
    o << mv(x, y);
    for (int i=0; i<w; i++) {
        double t = (w>1) ? double(i)/(w-1) : 0.0;
        if (i < filled)
            o << Theme::gradient(c1, c2, t) << "\u258f";   // ▏ left one-eighth block
        else
            o << DIM << Theme::fg("inactive_fg") << "\u2592" << RST;
    }
    o << RST;
}

// ═════════════════════════════════════════════════════════════════
// BOX  (rounded corners ╭╮╰╯, optional title insets ┤title├)
// ═════════════════════════════════════════════════════════════════

static void box(std::ostringstream& o, int x, int y, int w, int h,
                const std::string& bc,
                const std::string& ltitle="",
                const std::string& rtitle="") {
    if (w<2||h<2) return;
    o << bc;
    o << mv(x,y) << u8(0x256d);
    for (int i=0;i<w-2;i++) o << u8(0x2500);
    o << u8(0x256e);
    for (int r=y+1; r<y+h-1; r++)
        o << mv(x,r) << u8(0x2502) << mv(x+w-1,r) << u8(0x2502);
    o << mv(x,y+h-1) << u8(0x2570);
    for (int i=0;i<w-2;i++) o << u8(0x2500);
    o << u8(0x256f) << RST;
    if (!ltitle.empty()) {
        std::string t = " "+ltitle+" ";
        o << mv(x+2,y) << bc << u8(0x2524) << RST
          << BOLD << Theme::fg("title") << t << RST
          << bc << u8(0x251c) << RST;
    }
    if (!rtitle.empty()) {
        std::string t = " "+rtitle+" ";
        int tx = x+w-(int)t.size()-2;
        if (tx > x+4)
            o << mv(tx,y) << bc << u8(0x2524) << RST
              << Theme::fg("hi_fg") << t << RST
              << bc << u8(0x251c) << RST;
    }
}

// Internal vertical divider with T-junctions ┬ and ┴
static void vdiv(std::ostringstream& o, int x, int y1, int y2, const std::string& bc) {
    o << bc << mv(x,y1) << u8(0x252c);
    for (int r=y1+1; r<y2; r++) o << mv(x,r) << u8(0x2502);
    o << mv(x,y2) << u8(0x2534) << RST;
}

// ═════════════════════════════════════════════════════════════════
// LAYOUT  — layout
// ═════════════════════════════════════════════════════════════════

struct Layout {
    int cpu_x,cpu_y,cpu_w,cpu_h;
    int mem_x,mem_y,mem_w,mem_h;
    int net_x,net_y,net_w,net_h;
    int prc_x,prc_y,prc_w,prc_h;
};

static Layout make_layout() {
    // CPU box: full width, top ~38% of terminal
    int cpu_h  = std::max(12, height*38/100);
    // Bottom section: remaining rows minus the menu bar row
    int bot_y  = cpu_h + 2;          // y where bottom section starts
    int bot_h  = height - bot_y;     // rows available for bottom section
    // Left column ~45% for mem+net, right ~55% for proc
    int left_w = width * 45 / 100;
    int right_w= width - left_w;     // proc box includes its left border
    // Mem box top ~58% of bottom height, net box the rest
    int mem_h  = bot_h * 58 / 100;
    int net_h  = bot_h - mem_h;
    Layout L{};
    L.cpu_x=1;         L.cpu_y=2;         L.cpu_w=width;    L.cpu_h=cpu_h;
    L.mem_x=1;         L.mem_y=bot_y;     L.mem_w=left_w;   L.mem_h=mem_h;
    L.net_x=1;         L.net_y=bot_y+mem_h; L.net_w=left_w; L.net_h=net_h;
    L.prc_x=left_w+1;  L.prc_y=bot_y;    L.prc_w=right_w;  L.prc_h=bot_h;
    return L;
}

// ═════════════════════════════════════════════════════════════════
// MENU BAR  ┌─┤cpu├─┬─┤menu├─┬─┤preset *├────────17:28:18─┤2000ms├─┐
// ═════════════════════════════════════════════════════════════════

static void draw_menubar(std::ostringstream& o) {
    time_t now = time(nullptr);
    char tb[16]; strftime(tb,sizeof(tb),"%H:%M:%S",localtime(&now));
    std::string ms = std::to_string(Config::integer("update_ms"))+"ms";

    // helper: ┤ label ├ inset
    auto tag = [&](const std::string& t, bool hi=false) -> std::string {
        std::string c = hi ? Theme::fg("hi_fg") : Theme::fg("title");
        return DIM+u8(0x2524)+RST+BOLD+c+t+RST+DIM+u8(0x251c)+RST;
    };
    auto sep = [&]() -> std::string {
        return DIM+u8(0x252c)+u8(0x2500)+RST;
    };

    // Build left part: ─┤cpu├─┬─┤menu├─┬─┤preset *├─
    std::string L = DIM+u8(0x2500)+RST +tag("cpu")+sep()+tag("menu")+sep()+tag("preset *")+DIM+u8(0x2500)+RST;
    // Build right part: ─17:28:18─┤2000ms├─
    std::string R = DIM+u8(0x2500)+RST+Theme::fg("graph_text")+std::string(tb)+RST
                  + DIM+u8(0x2500)+RST+tag(ms)+DIM+u8(0x2500)+RST;

    // Visible char counts (approximate — these are pure ASCII widths)
    int lvis = 1+5+2+6+2+10+1;   // ─ cpu sep menu sep preset * ─
    int rvis = 1+8+1+(int)ms.size()+2+1;
    int fill = std::max(0, width-2-lvis-rvis);

    o << mv(1,1) << DIM << u8(0x250c) << RST;
    o << L;
    for (int i=0;i<fill;i++) o << DIM << u8(0x2500) << RST;
    o << R << DIM << u8(0x2510) << RST;
}

// ═════════════════════════════════════════════════════════════════
// CPU BOX
//
// ┌─────────────────────────────────────────┬─────────────────────┐
// │  braille history graph                  │ AMD Ryzen 7 5800   3.53 GHz
// │  (fills entire left panel)              │ CPU [======] 13%  60.0°C
// │                                         │ C0  [....] 7%  60°C  C8  [..] 7%  60°C
// │                                         │ ...
// │  up HH:MM:SS                            │ Load avg: 1.82  1.77  1.74
// └─────────────────────────────────────────┘ GPU [======] 1%  2.13/8.0G  ...
// ═════════════════════════════════════════════════════════════════

static void draw_cpu(std::ostringstream& o, const Layout& L) {
    int x=L.cpu_x, y=L.cpu_y, w=L.cpu_w, h=L.cpu_h;
    box(o, x, y, w, h, Theme::fg("cpu_box"));

    // Split: right info panel fixed at ~40% of width, min 50, max 70
    int info_w  = std::clamp(w*40/100, 50, 72);
    int graph_w = w - info_w - 1;   // graph cols between left border+1 and divider-1
    int div_x   = x + graph_w;      // column of the vertical divider
    vdiv(o, div_x, y, y+h-1, Theme::fg("cpu_box"));

    // ── LEFT: braille history graph fills entire left panel ──
    // Area: columns [x+1 .. div_x-1], rows [y+1 .. y+h-2]
    int gx = x+1, gy = y+1;
    int gw = div_x - x - 1;  // = graph_w - 1 (no border or divider)
    int gh = h - 2;           // inner height
    if (gw>0 && gh>0 && !Cpu::history.empty())
        graph(o, gx, gy, gw, gh, Cpu::history, "cpu_start", "cpu_end");
    // "up HH:MM:SS" overwrites bottom-left of graph
    o << mv(x+2, y+h-2)
      << DIM << Theme::fg("graph_text") << "up " << RST
      << Theme::fg("main_fg") << Cpu::uptime << RST;

    // ── RIGHT: info panel ─────────────────────────────────────
    int rx = div_x + 1;       // first col of right panel content
    int rw = info_w - 2;      // usable width (divider col + right border = 2)
    int ir = y + 1;            // current row

    // Clear right panel to remove any stale content from previous frame
    clr(o, rx, y+1, rw, h-2);

    // Row 1: "AMD Ryzen 7 5800 8-Core Processor   3.53 GHz"
    {
        std::string freq  = Cpu::frequency;
        std::string model = Cpu::model_name;
        int flen = (int)freq.size();
        int max_m = rw - flen - 2;
        if (max_m < 1) max_m = 1;
        if ((int)model.size() > max_m) model = model.substr(0, max_m);
        o << mv(rx, ir) << BOLD << Theme::fg("main_fg") << model << RST;
        int fx = rx + rw - flen;
        if (fx > rx + (int)model.size())
            o << mv(fx, ir) << BOLD << Theme::fg("hi_fg") << freq << RST;
        ir++;
    }

    // Row 2: "CPU [============] 13%  60.0°C"
    // Fixed widths: "CPU "(4) + bar + " "(1) + "100%"(4) + "  "(2) + "60.0°C"(6) = 17+bar
    {
        // °C = U+00B0 + C: 2 bytes but 2 display chars — so "60.0°C" = 6 display cols
        int bw = std::max(3, rw - 17);
        o << mv(rx, ir) << DIM << Theme::fg("graph_text") << "CPU " << RST;
        meter(o, rx+4, ir, bw, Cpu::total_usage, "cpu_start", "cpu_end");
        std::ostringstream ps; ps << std::fixed << std::setprecision(0) << Cpu::total_usage << "%";
        o << mv(rx+4+bw+1, ir)
          << BOLD << Theme::fg("hi_fg") << rjust(ps.str(), 4) << RST
          << "  " << Theme::fg("temp_start") << Cpu::temperature << RST;
        ir++;
    }

    // Rows 3+: per-core pairs
    // Layout per column (col_w chars each):
    //   "C0 "(3) + dotbar(cbar) + " "(1) + "19%"(rjust 4) + " "(1) + temp(7) = 16 + cbar
    // temp "60.0°C" = 6 display cols, + 1 space = 7
    // So cbar = col_w - 16
    int num   = (int)Cpu::core_usage.size();
    int half  = (num + 1) / 2;
    int col_w = rw / 2;
    int cbar  = std::max(2, col_w - 16);
    // Temp display width: "60.0°C" needs 6 display cols
    // But we print at fixed offset, so just clamp to col boundary

    for (int i=0; i<half && ir<y+h-3; i++, ir++) {
        auto draw_core = [&](int sx, int ci) {
            if (ci >= num) return;
            // Clamp sx so we never write past right border
            if (sx >= x+w-1) return;
            double v = Cpu::core_usage[ci];
            // Label e.g. "C0 " or "C15" — 3 chars
            std::string lbl = "C"+std::to_string(ci);
            while ((int)lbl.size() < 3) lbl += ' ';
            o << mv(sx, ir) << DIM << Theme::fg("graph_text") << lbl << RST;
            // Dotbar
            int bar_x = sx + 3;
            int bar_end = sx + col_w - 13; // leave room for pct+temp
            int bw2 = std::max(1, bar_end - bar_x);
            bw2 = std::min(bw2, cbar);
            dotbar(o, bar_x, ir, bw2, v, "cpu_start", "cpu_end");
            // Pct — rjust to 4 cols
            std::ostringstream vs; vs << std::fixed << std::setprecision(0) << v << "%";
            std::string pcol = v<50 ? Theme::fg("cpu_start")
                             : v<80 ? Theme::fg("cpu_mid")
                                    : Theme::fg("cpu_end");
            o << mv(sx+3+bw2+1, ir) << pcol << BOLD << rjust(vs.str(),4) << RST;
            // Temp — only if it fits (col boundary check)
            int temp_x = sx+3+bw2+1+4+1;
            int col_right = sx + col_w - 1;
            if (temp_x + 6 <= col_right)
                o << mv(temp_x, ir) << Theme::fg("temp_start") << Cpu::temperature << RST;
        };
        draw_core(rx,          i);
        draw_core(rx + col_w,  i + half);
    }

    // Load avg
    if (ir < y+h-2) {
        double l1=0,l5=0,l15=0;
        { std::ifstream f("/proc/loadavg"); if(f) f>>l1>>l5>>l15; }
        o << mv(rx, ir) << DIM << Theme::fg("graph_text") << "Load avg: " << RST
          << Theme::fg("main_fg") << std::fixed << std::setprecision(2)
          << l1 << "  " << l5 << "  " << l15 << RST;
        ir++;
    }

    // GPU
    if (ir < y+h-1 && !Cpu::gpu_usage_str.empty()) {
        int bw = std::max(3, rw-17);
        o << mv(rx, ir) << DIM << Theme::fg("graph_text") << "GPU " << RST;
        meter(o, rx+4, ir, bw, Cpu::gpu_pct, "process_start", "cpu_end");
        std::ostringstream gs; gs << std::fixed << std::setprecision(0) << Cpu::gpu_pct << "%";
        o << mv(rx+4+bw+1, ir)
          << BOLD << Theme::fg("hi_fg") << rjust(gs.str(),4) << RST;
        if (!Cpu::gpu_mem_str.empty())  o << "  " << Theme::fg("cached_start") << Cpu::gpu_mem_str << RST;
        if (!Cpu::gpu_temp_str.empty()) o << "  " << Theme::fg("temp_start")   << Cpu::gpu_temp_str << RST;
    }
}

// ═════════════════════════════════════════════════════════════════
// MEM BOX
// ═════════════════════════════════════════════════════════════════

static void draw_mem(std::ostringstream& o, const Layout& L) {
    int x=L.mem_x, y=L.mem_y, w=L.mem_w, h=L.mem_h;
    box(o, x, y, w, h, Theme::fg("mem_box"), "mem");

    // Internal divider at ~55%
    int div_x = x + w*55/100;
    vdiv(o, div_x, y, y+h-1, Theme::fg("mem_box"));

    // "disks" title on divider
    o << mv(div_x,y) << Theme::fg("mem_box") << u8(0x2524) << RST
      << BOLD << Theme::fg("title") << " disks " << RST
      << Theme::fg("mem_box") << u8(0x251c) << RST;

    // ── LEFT: RAM stats ───────────────────────────────────────
    int lx      = x+1;
    int l_inner = div_x - x - 2;     // usable width between left border and divider
    int val_w   = 7;                  // right-aligned value (e.g. "31.20G")
    int lbl_w   = l_inner - val_w - 1;
    if (lbl_w < 2) lbl_w = 2;
    int row = y+1;

    // Clear left panel completely
    clr(o, lx, row, l_inner, h-2);

    auto stat = [&](const std::string& lbl, const std::string& val,
                    double pct, const std::string& vc, const std::string& pc,
                    bool show_pct=true) {
        if (row >= y+h-1) return;
        o << mv(lx, row)
          << DIM << Theme::fg("graph_text") << ljust(lbl, lbl_w) << " " << RST
          << Theme::fg(vc) << rjust(val, val_w) << RST;
        row++;
        if (show_pct && row < y+h-1) {
            o << mv(lx+4, row) << BOLD << Theme::fg(pc)
              << std::fixed << std::setprecision(0) << pct << "%" << RST;
            row++;
        }
    };

    stat("Total:",     Mem::fmt_bytes(Mem::ram_total), 0,
         "main_fg","main_fg",false);
    stat("Used:",      Mem::fmt_bytes(Mem::ram_used),  Mem::ram_pct,
         "used_start","used_start");
    if (row<y+h-1) row++;
    stat("Available:", Mem::fmt_bytes(Mem::ram_available), Mem::available_pct,
         "available_start","available_start");
    if (row<y+h-1) row++;
    stat("Cached:",    Mem::fmt_bytes(Mem::ram_cached), Mem::cached_pct,
         "cached_start","cached_start");
    if (row<y+h-1) row++;
    stat("Free:",      Mem::fmt_bytes(Mem::ram_free),
         100.0*Mem::ram_free/std::max(1LL,Mem::ram_total),
         "free_end","free_end");
    if (row<y+h-1) row++;
    if (Mem::swap_total > 0)
        stat("Swap:", Mem::fmt_bytes(Mem::swap_used)+"/"+Mem::fmt_bytes(Mem::swap_total),
             Mem::swap_pct, "used_mid","used_mid");

    // ── RIGHT: disks ──────────────────────────────────────────
    int rx   = div_x+1;
    int rw   = w - (div_x-x) - 2;
    int drow = y+1;
    clr(o, rx, drow, rw, h-2);

    for (auto& d : Mem::disks) {
        if (drow+1 >= y+h-1) break;
        // Size right-aligned
        o << mv(rx, drow) << Theme::fg("main_fg") << rjust(Mem::fmt_bytes(d.total), rw) << RST;
        drow++;
        // Braille usage bar (flat = full-width bar at usage level)
        if (drow < y+h-1) {
            std::vector<double> hist(rw*2, d.used_pct);
            graph(o, rx, drow, rw, 1, hist, "free_end", "used_start");
            drow++;
        }
    }
}

// ═════════════════════════════════════════════════════════════════
// NET BOX
// ═════════════════════════════════════════════════════════════════

static void draw_net(std::ostringstream& o, const Layout& L) {
    int x=L.net_x, y=L.net_y, w=L.net_w, h=L.net_h;
    if (h < 5) return;
    box(o, x, y, w, h, Theme::fg("net_box"), "net");

    // IP addr in left of border
    if (!Net::ip_addr.empty()) {
        std::string t = " "+Net::ip_addr+" ";
        o << mv(x+6,y) << Theme::fg("net_box") << u8(0x2524) << RST
          << DIM << Theme::fg("graph_text") << t << RST
          << Theme::fg("net_box") << u8(0x251c) << RST;
    }
    // Options + interface in right of border
    {
        std::string opts = " sync \u25a1auto \u25a1zero \u25a1+b  "+Net::interface+" n\u2192 ";
        int tx = x+w-(int)opts.size()-2;
        if (tx > x+20)
            o << mv(tx,y) << Theme::fg("net_box") << u8(0x2524) << RST
              << DIM << Theme::fg("graph_text")
              << " sync \u25a1auto \u25a1zero \u25a1+b  " << RST
              << Theme::fg("hi_fg") << Net::interface << RST
              << DIM << Theme::fg("graph_text") << " n\u2192 " << RST
              << Theme::fg("net_box") << u8(0x251c) << RST;
    }

    // Stats panel is right ~38%, graph is left ~62%
    int stats_w = std::min(22, w*38/100);
    int gw      = w - 2 - stats_w - 1;  // graph width in chars
    int gx      = x+1;
    int sx      = x+1+gw+1;             // stats panel start col

    // Midpoint row splits download (top) and upload (bottom)
    int mid = y + h/2;
    int dgh = mid - y - 1;              // download graph height
    int ugh = y + h - 1 - mid - 1;     // upload graph height

    // Scale tick top and bottom left
    o << mv(gx, y+1)   << DIM << Theme::fg("graph_text") << "15K" << RST;
    o << mv(gx, y+h-2) << DIM << Theme::fg("graph_text") << "15K" << RST;

    // Horizontal midline spans across graph area only
    o << Theme::fg("net_box");
    o << mv(x, mid) << u8(0x251c);
    for (int i=0;i<gw+1;i++) o << u8(0x2500);
    o << u8(0x2524) << RST;

    // Graphs
    if (dgh>0 && !Net::rx_history.empty())
        graph(o, gx, y+1,   gw, dgh, Net::rx_history, "download_start","download_end");
    if (ugh>0 && !Net::tx_history.empty())
        graph(o, gx, mid+1, gw, ugh, Net::tx_history, "upload_start","upload_end");

    // Stats — write row by row, check bounds each time
    int dr = y+1;
    auto dline = [&](const std::string& s) {
        if (dr < mid) { o << mv(sx,dr++) << s; }
    };
    auto uline = [&](const std::string& s) {
        if (dr < y+h-1) { o << mv(sx,dr++) << s; }
    };

    dline(DIM+Theme::fg("graph_text")+"download"+RST);
    dline(Theme::fg("download_start")+u8(0x25bc)+" "+BOLD+Net::rx_str+RST);
    dline(DIM+Theme::fg("graph_text")+u8(0x25bc)+" Top:   "+RST+Theme::fg("download_start")+Net::rx_total_str+RST);
    dline(DIM+Theme::fg("graph_text")+u8(0x25bc)+" Total: "+RST+Theme::fg("download_start")+Net::rx_total_str+RST);

    dr = mid+1;
    uline(Theme::fg("upload_start")+u8(0x25b2)+" "+BOLD+Net::tx_str+RST);
    uline(DIM+Theme::fg("graph_text")+u8(0x25b2)+" Top:   "+RST+Theme::fg("upload_start")+Net::tx_total_str+RST);
    uline(DIM+Theme::fg("graph_text")+u8(0x25b2)+" Total: "+RST+Theme::fg("upload_start")+Net::tx_total_str+RST);
    if (y+h-2 > mid+1)
        o << mv(sx,y+h-2) << DIM << Theme::fg("graph_text") << "upload" << RST;
}

// ═════════════════════════════════════════════════════════════════
// PROC BOX
// ═════════════════════════════════════════════════════════════════

static void draw_proc(std::ostringstream& o, const Layout& L) {
    int x=L.prc_x, y=L.prc_y, w=L.prc_w, h=L.prc_h;
    box(o, x, y, w, h, Theme::fg("proc_box"), "proc");

    // Top border: option tags
    auto btag = [&](const std::string& t, bool hi=false) -> std::string {
        std::string c = hi ? Theme::fg("hi_fg") : Theme::fg("title");
        return DIM+u8(0x2524)+RST+BOLD+c+t+RST+DIM+u8(0x251c)+RST;
    };
    auto bsep = [&]() -> std::string {
        return DIM+u8(0x252c)+u8(0x2500)+RST;
    };
    std::string topts =
        btag("filter")+bsep()+btag("per-core")+bsep()+
        btag("reverse")+bsep()+btag("tree")+bsep()+
        btag("\u2190 cpu  direct \u2192",true);
    // Center the options string (approx visible width = 6+9+8+5+16 + separators = ~50)
    int opts_x = x + (w-50)/2;
    if (opts_x < x+2) opts_x = x+2;
    o << mv(opts_x, y) << topts;

    // Process count top-right
    std::string cnt = " "+std::to_string(Proc::total_count)+" ";
    o << mv(x+w-(int)cnt.size()-2, y)
      << Theme::fg("proc_box") << u8(0x2524) << RST
      << DIM << Theme::fg("graph_text") << cnt << RST
      << Theme::fg("proc_box") << u8(0x251c) << RST;

    // ── Columns ───────────────────────────────────────────────
    // Columns: Pid | Program | Command | Threads | User | MemB | Cpu%↑
    // Fixed-width columns, cmd gets the remainder
    // Total fixed = pid+prog+thr+usr+mem+cpu + spaces
    int w_pid  = 7;
    int w_prog = 16;
    int w_thr  = 7;   // "Threads" fits in 7
    int w_usr  = 5;
    int w_mem  = 7;   // "10.2M" "1.90G" etc
    int w_cpu  = 6;   // "113.0" max
    int fixed  = w_pid+1 + w_prog+1 + w_thr + w_usr + w_mem + w_cpu + 2; // +2 borders
    int w_cmd  = std::max(4, w - fixed);

    // Header row
    o << mv(x+1, y+1) << BOLD << Theme::fg("title")
      << ljust("Pid:",    w_pid)  << " "
      << ljust("Program:",w_prog) << " "
      << ljust("Command:",w_cmd)
      << rjust("Threads:",w_thr)
      << rjust("User:",   w_usr)
      << rjust("MemB",    w_mem)
      << rjust("Cpu%\u2191",w_cpu) << RST;

    // Header divider
    o << mv(x+1, y+2) << DIM << Theme::fg("proc_box");
    for (int i=0;i<w-2;i++) o << u8(0x2500);
    o << RST;

    // ── Process rows ──────────────────────────────────────────
    int max_rows = h - 5;  // header + divider + bottom hint bar + 2 borders
    int count = std::min((int)Proc::procs.size(), max_rows);

    for (int i=0; i<count; i++) {
        auto& p  = Proc::procs[i];
        int   ry = y+3+i;
        bool  sel = (i + Proc::scroll_offset == Proc::selected);

        // Background for selected row
        if (sel) {
            o << mv(x+1,ry) << Theme::bg("selected_bg");
            for (int j=0;j<w-2;j++) o << " ";
            o << mv(x+1,ry) << BOLD << Theme::fg("selected_fg");
        } else {
            o << mv(x+1,ry);
        }

        // Truncate fields to column widths
        std::string prog = p.name;
        if ((int)prog.size() > w_prog) prog = prog.substr(0,w_prog-1)+u8(0x2026);

        std::string cmd = p.cmd.empty() ? p.name : p.cmd;
        if ((int)cmd.size() > w_cmd) cmd = cmd.substr(0,w_cmd-1)+u8(0x2026);

        std::string usr = p.user;
        if ((int)usr.size() > w_usr) usr = usr.substr(0,w_usr);

        // CPU% formatted to 1 decimal
        std::ostringstream cs;
        cs << std::fixed << std::setprecision(1) << p.cpu_pct;
        std::string cpu_s = cs.str();
        // Cap cpu_s display at w_cpu chars
        if ((int)cpu_s.size() > w_cpu) cpu_s = cpu_s.substr(0,w_cpu);

        // Colors
        std::string prog_col = (p.cpu_pct>0.5) ? Theme::fg("hi_fg") : Theme::fg("proc_misc");
        std::string cpu_col  = p.cpu_pct<1.0  ? Theme::fg("process_start")
                             : p.cpu_pct<50.0 ? Theme::fg("cpu_start")
                             : p.cpu_pct<80.0 ? Theme::fg("cpu_mid")
                                              : Theme::fg("cpu_end");

        // Pid
        o << Theme::fg("main_fg") << rjust(std::to_string(p.pid), w_pid) << " ";
        // Program (coloured)
        if (!sel) o << prog_col;
        o << ljust(prog, w_prog) << " ";
        if (!sel) o << RST << Theme::fg("main_fg");
        // Command
        o << ljust(cmd, w_cmd);
        // Threads
        o << rjust(p.threads>0 ? std::to_string(p.threads) : "", w_thr);
        // User
        o << rjust(usr, w_usr);
        // Mem
        o << rjust(Mem::fmt_bytes(p.mem_bytes), w_mem);
        // Cpu%
        o << RST << cpu_col << BOLD << rjust(cpu_s, w_cpu) << RST;
    }

    // ── Bottom hint bar ───────────────────────────────────────
    o << mv(x+1, y+h-1) << Theme::fg("proc_box") << u8(0x2570) << u8(0x2500) << RST;

    auto hint = [&](const std::string& k, const std::string& d) {
        o << DIM<<u8(0x2502)<<RST
          << BOLD<<Theme::fg("hi_fg")<<k<<RST
          << DIM<<Theme::fg("graph_text")<<d<<RST;
    };
    hint("\u2193\u2191","select ");
    hint("\u2193\u2191","info ");
    hint("\u2191","terminate ");
    hint("\u2191","kill ");
    hint("\u2191","signals ");
    hint("q","quit ");

    // Scroll position bottom-right
    std::string scr = std::to_string(Proc::scroll_offset)+"/"+std::to_string(Proc::total_count);
    o << mv(x+w-(int)scr.size()-2, y+h-1) << DIM << Theme::fg("graph_text") << scr << RST;
    o << mv(x+w-1, y+h-1) << Theme::fg("proc_box") << u8(0x256f) << RST;
}

// ═════════════════════════════════════════════════════════════════
// TERMINAL INIT / CLEANUP
// ═════════════════════════════════════════════════════════════════

static void get_term_size() {
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)==0) {
        width  = ws.ws_col > 0 ? ws.ws_col : 80;
        height = ws.ws_row > 0 ? ws.ws_row : 24;
    }
    Global::term_width  = width;
    Global::term_height = height;
}

void init() {
    get_term_size();
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO|ICANON);
    raw.c_cc[VMIN]=0; raw.c_cc[VTIME]=0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode = true;
    std::cout << "\033[?1049h\033[?25l" << std::flush;
}

void cleanup() {
    std::cout << "\033[?25h\033[?1049l" << std::flush;
    if (raw_mode) { tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios); raw_mode=false; }
}

void resize()       { get_term_size(); clear_screen(); }
void clear_screen() { std::cout << "\033[2J\033[H" << std::flush; }
void flush()        { std::cout << std::flush; }

// ═════════════════════════════════════════════════════════════════
// MAIN DRAW ENTRY
// ═════════════════════════════════════════════════════════════════

void draw_all() {
    if (width < 80 || height < 24) {
        std::cout << "\033[2J\033[H\033[1;31mTerminal too small — need at least 80×24\033[0m\n" << std::flush;
        return;
    }
    std::ostringstream o;
    o << "\033[2J";       // clear screen
    Layout L = make_layout();
    draw_menubar(o);
    draw_cpu(o, L);
    draw_mem(o, L);
    draw_net(o, L);
    draw_proc(o, L);
    std::cout << o.str() << std::flush;
}

} // namespace Draw
