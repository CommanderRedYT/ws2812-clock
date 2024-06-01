#include "digithelper.h"

namespace digithelper {

/*
 One segment

   A
 F   B
   G
 E   C
   D

 */

std::map<char, uint8_t> SEGMENT_MASKS = {
    {'0', A | B | C | D | E | F},
    {'1', B | C},
    {'2', A | B | D | E | G},
    {'3', A | B | C | D | G},
    {'4', B | C | F | G},
    {'5', A | C | D | F | G},
    {'6', A | C | D | E | F | G},
    {'7', A | B | C},
    {'8', A | B | C | D | E | F | G},
    {'9', A | B | C | D | F | G},
    {'A', A | B | C | E | F | G},
    {'B', C | D | E | F | G},
    {'C', A | D | E | F},
    {'D', B | C | D | E | G},
    {'E', A | D | E | F | G},
    {'F', A | E | F | G},
    {'G', A | C | D | E | F},
    {'H', B | C | E | F | G},
    {'I', B | C},
    {'J', B | C | D | E},
    {'K', E | F | G},
    {'L', D | E | F},
    {'M', A | C | E},
    {'N', C | E | G},
    {'O', A | B | C | D | E | F},
    {'P', A | B | E | F | G},
    {'Q', A | B | C | F | G},
    {'R', E | G},
    {'S', A | C | D | F | G},
    {'T', D | E | F | G},
    {'U', B | C | D | E | F},
    {'V', C | D | E},
    {'W', B | D | F},
    {'X', B | C | E | F | G},
    {'Y', B | C | D | F | G},
    {'Z', A | B | D | E | G},
    {' ', 0},
    {'-', G},
    {'_', D},
};

uint8_t getSegmentMask(char c)
{
    // check if char is lowercase
    bool isLowercase = c >= 'a' && c <= 'z';

    auto it = SEGMENT_MASKS.find(c);
    if (it != SEGMENT_MASKS.end())
    {
        return it->second;
    }

    if (isLowercase)
    {
        it = SEGMENT_MASKS.find(c - 32);
        if (it != SEGMENT_MASKS.end())
        {
            return it->second;
        }
    }
    else
    {
        it = SEGMENT_MASKS.find(c + 32);
        if (it != SEGMENT_MASKS.end())
        {
            return it->second;
        }
    }

    return 0;
}

} // namespace digithelper
