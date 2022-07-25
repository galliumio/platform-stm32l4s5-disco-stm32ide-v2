#ifndef HEADLIGHT_PATTERN_H
#define HEADLIGHT_PATTERN_H

#include "bsp.h"
#include <stdint.h>
#include "fw_log.h"
#include "qassert.h"

#define HEADLIGHT_PATTERN_ASSERT(t_) ((t_)? (void)0: Q_onAssert("HeadlightPattern.h", (int32_t)__LINE__))

namespace APP {

class HeadlightInterval {
public:
    uint32_t m_led0Color;       // LED 0 color (888 xBGR).
    uint32_t m_led1Color;       // LED 1 color (888 xBGR).
    uint16_t m_durationMs;      // Duration in millisecond.
    
    uint32_t GetLed0Color() const { return m_led0Color; }
    uint32_t GetLed1Color() const { return m_led1Color; }
    uint16_t GetDurationMs() const { return m_durationMs; }
};

class HeadlightPattern {
public:
    enum {
        COUNT = 256
    };
    uint32_t m_count;                // Number of intervals in use.
    HeadlightInterval m_interval[COUNT];   // Array of intervals. Used ones start from index 0.
    
    // Must perform range check. Assert if invalid.
    uint32_t GetCount() const { 
        return m_count;
    }
    HeadlightInterval const &GetInterval(uint32_t index) const {
        HEADLIGHT_PATTERN_ASSERT(index < m_count);
        return m_interval[index];
    }
};

class HeadlightPatternSet {
public:
    enum {
        COUNT = 8
    };
    uint32_t m_count;               // Number of patterns in use.
    HeadlightPattern m_pattern[COUNT];    // Array of patterns. Used ones start from index 0.
    
    // Must perform range check. Assert if invalid.
    uint32_t GetCount() const {
        return m_count;
    }
    HeadlightPattern const *GetPattern(uint32_t index) const {
        if (index < m_count) {
            HEADLIGHT_PATTERN_ASSERT(index < COUNT);
            return &m_pattern[index];
        }
        return NULL;
    }
};

extern HeadlightPatternSet const HEADLIGHT_PATTERN_SET;

} // namespace APP

#endif // HEADLIGHT_PATTERN_H
