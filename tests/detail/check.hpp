#pragma once

#include <cstdio>
#include <cstdlib>

#define QCD_CHECK(condition)                                                       \
    do {                                                                           \
        if (!(condition)) {                                                        \
            std::printf("[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #condition);     \
            std::exit(1);                                                          \
        }                                                                          \
    } while (false)
