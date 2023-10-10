#pragma once

// system includes
#include <cstdint>
#include <map>

namespace digithelper {

enum SegmentMask : uint8_t
{
    A = 1 << 0,
    B = 1 << 1,
    C = 1 << 2,
    D = 1 << 3,
    E = 1 << 4,
    F = 1 << 5,
    G = 1 << 6,
};

extern std::map<char, uint8_t> SEGMENT_MASKS;

uint8_t getSegmentMask(char c);

} // namespace digithelper
