#pragma once
/* sysmon draw.hpp — Copyright 2025, Apache-2.0 */

namespace Draw {
    extern int width, height;
    void init();
    void cleanup();
    void resize();
    void clear_screen();
    void flush();
    void draw_all();
}
