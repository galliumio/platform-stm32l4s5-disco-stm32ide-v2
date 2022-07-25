/*******************************************************************************
 * Copyright (C) Gallium Studio LLC. All rights reserved.
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

#ifndef HEADLIGHT_H
#define HEADLIGHT_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Colors.h"
#include "HeadlightPattern.h"

using namespace QP;
using namespace FW;

namespace APP {

class Headlight : public Region {
public:
    Headlight();

protected:
    static QState InitialPseudoState(Headlight * const me, QEvt const * const e);
    static QState Root(Headlight * const me, QEvt const * const e);
        static QState Stopped(Headlight * const me, QEvt const * const e);
        static QState Starting(Headlight * const me, QEvt const * const e);
        static QState Stopping(Headlight * const me, QEvt const * const e);
        static QState Started(Headlight * const me, QEvt const * const e);
            static QState Off(Headlight * const me, QEvt const * const e);
            static QState On(Headlight * const me, QEvt const * const e);
                static QState Ramping(Headlight * const me, QEvt const * const e);
                static QState Stable(Headlight * const me, QEvt const * const e);
                static QState Aborted(Headlight * const me, QEvt const * const e);
            static QState Pattern(Headlight * const me, QEvt const * const e);
                static QState Repeating(Headlight * const me, QEvt const * const e);
                static QState Once(Headlight * const me, QEvt const * const e);
            static QState Faulted(Headlight * const me, QEvt const * const e);

    void CalRampSteps();
    void UpdateRampColor();

    Evt m_inEvt;                // Static event copy of a generic incoming req to be confirmed. Added more if needed.
    MsgBaseEvt m_inMsg;         // Static event copy of the base of an incoming request message to be confirmed.
    Timer m_stateTimer;
    Timer m_ws2812Timer;
    Timer m_intervalTimer;
    EvtSeqRec m_ws2812Seq;      // Keeps track of sequence numbers of outgoing events to WS2812.
    Color888Bgr m_color;        // Current color (888 xBGR).
    Color888Bgr m_savedColor;
    Color888Bgr m_setColor;     // Set color (888 xBGR).
    uint32_t m_setRampMs;       // Ramp up time from current color to set color in ms.
    uint32_t m_rampIdx;         // 0-based index to current ramp interval.
    uint32_t m_rampCnt;         // No. of ramp intervals.
    float m_stepRed;            // Step change of red color in each interval.
    float m_stepGreen;          // Step change of green color in each interval.
    float m_stepBlue;           // Step change of blue color in each interval.
    HeadlightPatternSet const &m_patternSet;   // Set of supported patterns.
    HeadlightPattern const *m_currPattern;
    uint32_t m_intervalIndex;
    bool m_isRepeat;

    enum {
        HEADLIGHT_COUNT = 4     // 0 and 3 are used. 1 and 2 are placeholders.
    };

    enum {
        WS2812_TIMEOUT_MS = 100,
        INTERVAL_TIMEOUT_MS = 50,       // Fixed interval timeout in On state.
    };

#define HEADLIGHT_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(WS2812_TIMER) \
    ADD_EVT(INTERVAL_TIMER)

#define HEADLIGHT_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(ABORT) \
    ADD_EVT(NEXT_INTERVAL) \
    ADD_EVT(LAST_INTERVAL)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        HEADLIGHT_TIMER_EVT_START = TIMER_EVT_START(HEADLIGHT),
        HEADLIGHT_TIMER_EVT
    };

    enum {
        HEADLIGHT_INTERNAL_EVT_START = INTERNAL_EVT_START(HEADLIGHT),
        HEADLIGHT_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // HEADLIGHT_H
