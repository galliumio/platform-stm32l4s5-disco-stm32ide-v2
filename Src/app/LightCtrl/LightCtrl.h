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

#ifndef LIGHT_CTRL_H
#define LIGHT_CTRL_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Light.h"
#include "LightCtrlInterface.h"

using namespace QP;
using namespace FW;

namespace APP {

class LightCtrl : public Active {
public:
    LightCtrl();

protected:
    static QState InitialPseudoState(LightCtrl * const me, QEvt const * const e);
    static QState Root(LightCtrl * const me, QEvt const * const e);
        static QState Stopped(LightCtrl * const me, QEvt const * const e);
        static QState Starting(LightCtrl * const me, QEvt const * const e);
            static QState Starting1(LightCtrl * const me, QEvt const * const e);
            static QState Starting2(LightCtrl * const me, QEvt const * const e);
        static QState Stopping(LightCtrl * const me, QEvt const * const e);
            static QState Stopping1(LightCtrl * const me, QEvt const * const e);
            static QState Stopping2(LightCtrl * const me, QEvt const * const e);
        static QState Started(LightCtrl * const me, QEvt const * const e);
            static QState Idle(LightCtrl * const me, QEvt const * const e);
            static QState Busy(LightCtrl * const me, QEvt const * const e);
        static QState Exception(LightCtrl * const me, QEvt const * const e);

    bool Perform(LightCtrlOp op);

    enum {
        LED_COUNT = 8,          // No. of LEDs on the LED strip to be controlled by the WS2812 HSM.
                                // Since the LEDs used are not contiguous, LED_COUNT is larger than LIGHT_COUNT.
        FADE_MS = 2000,         // Time in milliseconds to fade out lights when turning them off.
        RAMP_ON_MS = 2000,      // Time in milliseconds to ramp up lights when turning them on.
    };

    Light m_light[LIGHT_COUNT]; // LIGHT_COUNT is the no. of actual LEDs used (each LED controlled by a LIGHT HSM/region).
    Hsmn m_manager;             // Managing HSM.
    Evt m_inEvt;                // Static event copy of a generic incoming req to be confirmed. Added more if needed.
    MsgBaseEvt m_inMsg;         // Static event copy of the base of an incoming request message to be confirmed.
    Timer m_stateTimer;
    Timer m_busyTimer;

#define LIGHT_CTRL_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(BUSY_TIMER)

#define LIGHT_CTRL_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(NEXT) \
    ADD_EVT(IN_PROGRESS) \
    ADD_EVT(FAILED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        LIGHT_CTRL_TIMER_EVT_START = TIMER_EVT_START(LIGHT_CTRL),
        LIGHT_CTRL_TIMER_EVT
    };

    enum {
        LIGHT_CTRL_INTERNAL_EVT_START = INTERNAL_EVT_START(LIGHT_CTRL),
        LIGHT_CTRL_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // LIGHT_CTRL_H
