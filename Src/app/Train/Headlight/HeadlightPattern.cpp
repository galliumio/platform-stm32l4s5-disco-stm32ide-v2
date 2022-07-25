#include "HeadlightPattern.h"

namespace APP {

// Color format is (888 xBGR).
HeadlightPatternSet const HEADLIGHT_PATTERN_SET = {
    3,
    {
        // Pattern 0
        {21, 
            {
                {0x080808,0x080808,50}, {0x101010,0x101010,50}, {0x181818,0x181818,50}, {0x202020,0x202020,50}, {0x303030,0x303030,50},
                {0x404040,0x404040,50}, {0x606060,0x606060,50}, {0x909090,0x909090,50}, {0xc0c0c0,0xc0c0c0,50}, {0xffffff,0xffffff,50}, // ramp up
                {0xffffff,0xffffff,2000},                                                                                               // constant
                {0xffffff,0xffffff,50}, {0xc0c0c0,0xc0c0c0,50}, {0x909090,0x909090,50}, {0x606060,0x606060,50}, {0x404040,0x404040,50},
                {0x303030,0x303030,50}, {0x202020,0x202020,50}, {0x181818,0x181818,50}, {0x101010,0x101010,50}, {0x080808,0x080808,50}, // ramp down
            }
        },
        // Pattern 1
        {4, 
            {
                {0xffffff,0xffffff,200}, {0,0,200}, {0xffffff,0xffffff,200}, {0,0,1000}      // two short blinks.
            }
        },
        // Pattern 2 - Alternating 3-blinks (3) red and blue.
        {12,
            {
                {0x0000ff,0,100}, {0,0,100}, {0x0000ff,0,100}, {0,0,100}, {0x0000ff,0,100}, {0,0,100}, // 3 red blinks on LED 0.
                {0,0xff0000,100}, {0,0,100}, {0,0xff0000,100}, {0,0,100}, {0,0xff0000,100}, {0,0,100}, // 3 blue blinks on LED 1.
            }
        }
    }
};

} // namespace APP
