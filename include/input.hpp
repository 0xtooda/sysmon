#pragma once
/* sysmon input.hpp — Copyright 2025, Apache-2.0 */

namespace Input {
    void init();
    void cleanup();
    void handle(int timeout_ms);
}
