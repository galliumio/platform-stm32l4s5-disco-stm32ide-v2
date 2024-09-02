#include "LightPattern.h"

namespace APP {

// ### IMPORTANT ###
// Indices to the following patterns are defined in enum class LightPatternIdx in LightInterface.h
// It MUST be kept in sync with the pattern set definition below.

// Color format is (888 xBGR).
LightPatternSet const LIGHT_PATTERN_SET = {
    7,
    {
        // #0 BREATHING_WHITE.
        {22,
            {
                {0x080808,50}, {0x101010,50}, {0x181818,50}, {0x202020,50}, {0x282828,50}, // ramp up
                {0x303030,50}, {0x383838,50}, {0x404040,50}, {0x484848,50}, {0x505050,50},
                {0x585858,500},                                                            // constant
                {0x505050,50}, {0x484848,50}, {0x404040,50}, {0x383838,50}, {0x303030,50}, // ramp down
                {0x282828,50}, {0x202020,50}, {0x181818,50}, {0x101010,50}, {0x080808,50},
                {0x000000,500},                                                            // constant
            }
        },
        // #1 BREATHING_RED.
        {22,
            {
                {0x000008,50}, {0x000010,50}, {0x000018,50}, {0x000020,50}, {0x000028,50}, // ramp up
                {0x000030,50}, {0x000038,50}, {0x000040,50}, {0x000048,50}, {0x000050,50},
                {0x000058,500},                                                            // constant
                {0x000050,50}, {0x000048,50}, {0x000040,50}, {0x000038,50}, {0x000030,50},
                {0x000028,50}, {0x000020,50}, {0x000018,50}, {0x000010,50}, {0x000008,50}, // ramp down
                {0x000000,500},                                                            // constant
            }
        },
        // #2 ALERT_RED (4 fast blinks followed by 2 slow blinks).
        {12,
            {
                {0x0000ff,50}, {0,50}, {0x0000ff,50}, {0,50}, {0x0000ff,50}, {0,50}, {0x0000ff,50}, {0,50},
                {0x0000ff,800}, {0,400}, {0x0000ff,800}, {0,400},
            }
        },
        // #3 ALERT_BLUE (2 slow blinks followed by 4 fast blinks).
        {12,
            {
                {0xff0000,800}, {0,400}, {0xff0000,800}, {0,400},
                {0xff0000,50}, {0,50}, {0xff0000,50}, {0,50}, {0xff0000,50}, {0,50}, {0xff0000,50}, {0,50},
            }
        },
        // #4 ALERT_RED_BLUE (4 fast blinks in red, 2 slow blinks in white, 4 fast blinks in blue, 2 slow blinks in yellow).
        {24,
            {
                {0x0000ff,100}, {0,100}, {0x0000ff,100}, {0,100}, {0x0000ff,100}, {0,100}, {0x0000ff,100}, {0,100},
                {0xffffff,800}, {0,400}, {0xffffff,800}, {0,400},
                {0,100}, {0xff0000,100}, {0,100}, {0xff0000,100}, {0,100}, {0xff0000,100}, {0,100}, {0xff0000,100},
                {0x00ffff,800}, {0,400}, {0x00ffff,800}, {0,400},
            }
        },
        // #5 ALERT_AMBER_WHITE (4 fast blinks in white, 1 slow blink in amber and white each,
        //                    4 fast blinks in amber, 1 slow blink in white and amber each).
        {24,
            {
                {0xffffff,50}, {0,50}, {0xffffff,50}, {0,50}, {0xffffff,50}, {0,50}, {0xffffff,50}, {0,50},
                {0x00bfff,500}, {0,200}, {0xffffff,500}, {0,200},
                {0,50}, {0x00bfff,50}, {0,50}, {0x00bfff,50}, {0,50}, {0x00bfff,50}, {0,50}, {0x00bfff,50},
                {0xffffff,500}, {0,200}, {0x00bfff,500}, {0,200},
            }
        },
        // #6 STROBE_WHITE (Fast blink with breathing effect).
        {32,
            {
                {0x101010,1}, {0x202020,1}, {0x303030,1}, {0x404040,1}, {0x505050,1}, {0x606060,1}, {0x707070,1}, {0x808080,1},
                {0x909090,1}, {0xa0a0a0,1}, {0xb0b0b0,1}, {0xc0c0c0,1}, {0xd0d0d0,1}, {0xe0e0e0,1}, {0xf0f0f0,1}, {0xffffff,1},
                {0xf0f0f0,1}, {0xe0e0e0,1}, {0xd0d0d0,1}, {0xc0c0c0,1}, {0xb0b0b0,1}, {0xa0a0a0,1}, {0x909090,1}, {0x808080,1},
                {0x707070,1}, {0x606060,1}, {0x505050,1}, {0x404040,1}, {0x303030,1}, {0x202020,1}, {0x101010,1}, {0x000000,1},
            }
        },        
    }
};

} // namespace APP
