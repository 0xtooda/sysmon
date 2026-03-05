/* sysmon input.cpp — Copyright 2025, Apache-2.0 */
#include "../include/input.hpp"
#include "../include/sysmon.hpp"
#include "../include/proc.hpp"
#include "../include/config.hpp"
#include <unistd.h>
#include <sys/select.h>

namespace Input {

void init() {}
void cleanup() {}

void handle(int timeout_ms) {
    fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO,&fds);
    struct timeval tv{timeout_ms/1000,(timeout_ms%1000)*1000};
    if(select(STDIN_FILENO+1,&fds,nullptr,nullptr,&tv)<=0) return;

    char ch=0;
    if(read(STDIN_FILENO,&ch,1)<=0) return;

    switch(ch) {
        case 'q': case 'Q': Global::quitting=true; break;
        case 'j': Proc::scroll_offset++; break;
        case 'k': if(Proc::scroll_offset>0)Proc::scroll_offset--; break;
        case 'r': Proc::reverse=!Proc::reverse; break;
        case 't': Proc::tree_view=!Proc::tree_view; break;
        case '\033': {
            char seq[2]={};
            if(read(STDIN_FILENO,&seq[0],1)<=0) break;
            if(seq[0]!='[') break;
            if(read(STDIN_FILENO,&seq[1],1)<=0) break;
            if(seq[1]=='A'&&Proc::scroll_offset>0) Proc::scroll_offset--;
            else if(seq[1]=='B') Proc::scroll_offset++;
            break;
        }
    }
}

} // namespace Input
