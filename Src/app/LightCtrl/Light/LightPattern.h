#ifndef LIGHT_PATTERN_H
#define LIGHT_PATTERN_H

#include "bsp.h"
#include <stdint.h>
#include "fw_log.h"
#include "qassert.h"

#define LIGHT_PATTERN_ASSERT(t_) ((t_)? (void)0: Q_onAssert("LightPattern.h", (int32_t)__LINE__))

namespace APP {

class LightInterval {
public:
    uint32_t GetLedColor() const { return m_ledColor; }
    uint16_t GetDurationMs() const { return m_durationMs; }
    // Data member.
    uint32_t m_ledColor;        // LED 0 color (888 xBGR).
    uint16_t m_durationMs;      // Duration in millisecond.
};

class LightPattern {
public:
    enum {
        COUNT = 64
    };
    uint32_t GetCount() const {
        return m_count;
    }
    LightInterval const &GetInterval(uint32_t index) const {
        LIGHT_PATTERN_ASSERT((index < m_count) && (index < COUNT));
        return m_interval[index];
    }
    // Data member.
    uint32_t m_count;                   // Number of intervals in use.
    LightInterval m_interval[COUNT];    // Array of intervals. Used ones start from index 0.
};

class LightPatternSet {
public:
    enum {
        COUNT = 16
    };
    uint32_t GetCount() const {
        return m_count;
    }
    LightPattern const *GetPattern(uint32_t index) const {
        if (index < m_count) {
            LIGHT_PATTERN_ASSERT(index < COUNT);
            return &m_pattern[index];
        }
        return NULL;
    }
    // Data member.
    uint32_t m_count;                   // Number of patterns in use.
    LightPattern m_pattern[COUNT];      // Array of patterns. Used ones start from index 0.
};

extern LightPatternSet const LIGHT_PATTERN_SET;

} // namespace APP

#endif // LIGHT_PATTERN_H
