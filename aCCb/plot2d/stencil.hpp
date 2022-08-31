#pragma once
#include <stdint.h>

#include "proj.hpp"

typedef uint8_t stencil_t;  // bool: 32 ms; uint8: 4.5 ms; uint16: 6 ms uint32_t: 9 ms uint64_t: 16 ms
class stencil_t {
   public:
    stencil_t(const proj<float>& p) {}
};