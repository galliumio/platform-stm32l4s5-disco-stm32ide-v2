/*******************************************************************************
 * Copyright (C) 2018 Gallium Studio LLC (Lawrence Lo). All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Alternatively, this program may be distributed and modified under the
 * terms of Gallium Studio LLC commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for licensees
 * interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Contact information:
 * Website - https://www.galliumstudio.com
 * Source repository - https://github.com/galliumstudio
 * Email - admin@galliumstudio.com
 ******************************************************************************/

#ifndef LIGHT_H
#define LIGHT_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Colors.h"
#include "LightPattern.h"
#include "Ws2812Interface.h"

using namespace QP;
using namespace FW;

namespace APP {

class Light : public Region {
public:
    Light(Hsmn hsmn, char const *name, uint32_t ledIdx, uint32_t ledCnt, uint32_t ledInc);

protected:
    static QState InitialPseudoState(Light * const me, QEvt const * const e);
    static QState Root(Light * const me, QEvt const * const e);
        static QState Stopped(Light * const me, QEvt const * const e);
        static QState Stopping(Light * const me, QEvt const * const e);
        static QState Started(Light * const me, QEvt const * const e);
            static QState Off(Light * const me, QEvt const * const e);
            static QState On(Light * const me, QEvt const * const e);
                static QState Ramping(Light * const me, QEvt const * const e);
                static QState Stable(Light * const me, QEvt const * const e);
            static QState Pattern(Light * const me, QEvt const * const e);
                static QState Repeating(Light * const me, QEvt const * const e);
                static QState Once(Light * const me, QEvt const * const e);
        static QState Exception(Light * const me, QEvt const * const e);

    void CalRampSteps();
    void UpdateRampColor();
    void FillColors(Color888Bgr const &color);

    uint32_t m_ledIdx;          // Starting 0-based index of the LED(s) driven by this HSM on the WS2812 LED strip.
    uint32_t m_ledCnt;          // Number of LEDs driven by this HSM (must > 0).
    uint32_t m_ledInc;          // Increments in index between adjacent LEDs. It is "don't care" if count = 1.
    Hsmn m_manager;             // Managing HSM.
    Hsmn m_ws2812Hsmn;          // HSM interfacing to the WS2812 LED strip.
    Timer m_stateTimer;
    Timer m_ws2812Timer;
    Timer m_intervalTimer;
    EvtSeqRec m_ws2812Seq;      // Keeps track of sequence numbers of outgoing events to WS2812.
    Ws2812SetReq::LedColor m_colors[Ws2812SetReq::MAX_LED_COUNT];   // Color info passed to WS2812.
    Color888Bgr m_currColor;    // Current color (888 xBGR).
    Color888Bgr m_startColor;   // Starting (original) color when ramping up (to avoid rounding errors).
    Color888Bgr m_setColor;     // Set color (888 xBGR).
    uint32_t m_setRampMs;       // Ramp up time from current color to set color in ms.
    uint32_t m_rampIdx;         // 0-based index to current ramp interval.
    uint32_t m_rampCnt;         // No. of ramp intervals.
    float m_stepRed;            // Step change of red color in each interval.
    float m_stepGreen;          // Step change of green color in each interval.
    float m_stepBlue;           // Step change of blue color in each interval.
    LightPatternSet const &m_patternSet;   // Set of supported patterns.
    LightPattern const *m_currPattern;
    uint32_t m_intervalIndex;
    bool m_isRepeat;
    uint32_t m_nextTimeMs;      // System time in ms to start the next interval.
    Evt m_inEvt;                // Static event copy of a generic incoming req to be confirmed. Added more if needed.

    enum {
        LIGHT_COUNT = 4     // 0 and 3 are used. 1 and 2 are placeholders.
    };

    enum {
        WS2812_TIMEOUT_MS = 100,
        INTERVAL_TIMEOUT_MS = 50,       // Fixed interval timeout in On state.
    };

#define LIGHT_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(WS2812_TIMER) \
    ADD_EVT(INTERVAL_TIMER)

#define LIGHT_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(CONTINUE) \
    ADD_EVT(FAILED) \
    ADD_EVT(NEXT_INTERVAL) \
    ADD_EVT(LAST_INTERVAL)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        LIGHT_TIMER_EVT_START = TIMER_EVT_START(LIGHT),
        LIGHT_TIMER_EVT
    };

    enum {
        LIGHT_INTERNAL_EVT_START = INTERNAL_EVT_START(LIGHT),
        LIGHT_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // LIGHT_H
