/*******************************************************************************
 * Copyright (C) 2022 Gallium Studio LLC (Lawrence Lo). All rights reserved.
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

#ifndef TRAIN_H
#define TRAIN_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Headlight.h"
#include "Motor.h"
#include "MotorInterface.h"

using namespace QP;
using namespace FW;

namespace APP {

class Train : public Active {
public:
    Train();

protected:
    static QState InitialPseudoState(Train * const me, QEvt const * const e);
    static QState Root(Train * const me, QEvt const * const e);
        static QState Stopped(Train * const me, QEvt const * const e);
        static QState Starting(Train * const me, QEvt const * const e);
        static QState Stopping(Train * const me, QEvt const * const e);
        static QState Started(Train * const me, QEvt const * const e);
            static QState Faulted(Train * const me, QEvt const * const e);
            static QState Normal(Train * const me, QEvt const * const e);
                static QState Local(Train * const me, QEvt const * const e);
                    static QState Manual(Train * const me, QEvt const * const e);
                        static QState Idle(Train * const me, QEvt const * const e);
                        static QState Activated(Train * const me, QEvt const * const e);
                            static QState Rest(Train * const me, QEvt const * const e);
                            static QState Accel(Train * const me, QEvt const * const e);
                            static QState Decel(Train * const me, QEvt const * const e);
                                static QState RestWait(Train * const me, QEvt const * const e);
                                static QState IdleWait(Train * const me, QEvt const * const e);
                            static QState Running(Train * const me, QEvt const * const e);
                    static QState Auto(Train * const me, QEvt const * const e);
                static QState Remote(Train * const me, QEvt const * const e);
                    static QState Commanded(Train * const me, QEvt const * const e);
                    static QState Scheduled(Train * const me, QEvt const * const e);

    MotorDir GetDir();

    Evt m_inEvt;                // Static event copy of a generic incoming req to be confirmed. Added more if needed.
    Timer m_stateTimer;
    Timer m_headlightTimer;
    Timer m_restTimer;
    Timer m_runTimer;
    Headlight m_headlight;      // Orthogonal region for headlight control.
    Motor m_motor;              // Orthogonal region for motor control.
    MsgSeqRec m_headlightSeq;   // Keeps track of sequence numbers of outgoing messages to HEADLIGHT.
    MsgSeqRec m_motorSeq;       // Keeps track of sequence numbers of outgoing messages to MOTOR.
    bool m_pull;                // True if train is pulling. False if pushing.
    bool m_drive;               // True if train is driving. False if reversing.

    enum {
        HEADLIGHT_SET_TIMEOUT_MS = 10200,
        HEADLIGHT_PATTERN_TIMEOUT_MS = 200,
        // The following timeout periods are configurable.
        REST_TIMEOUT_MS = 2500,
        RUN_TIMEOUT_MS = 50000,
        RUN_LEVEL = 850,            // PWM duty-cycle permil level (0-1000).
        DEFAULT_ACCEL = 150,        // PWM permil steps per second.
        DEFAULT_DECEL = 150,        // PWM permil steps per second.
    };

#define TRAIN_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(HEADLIGHT_TIMER) \
    ADD_EVT(REST_TIMER) \
    ADD_EVT(RUN_TIMER)

#define TRAIN_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(BTN_A_PRESS) \
    ADD_EVT(BTN_B_PRESS) \
    ADD_EVT(BTN_A_HOLD) \
    ADD_EVT(BTN_B_HOLD) \
    ADD_EVT(HALL_DETECT)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        TRAIN_TIMER_EVT_START = TIMER_EVT_START(TRAIN),
        TRAIN_TIMER_EVT
    };

    enum {
        TRAIN_INTERNAL_EVT_START = INTERNAL_EVT_START(TRAIN),
        TRAIN_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // TRAIN_H
