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

#ifndef TRAIN_H
#define TRAIN_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Motor.h"
#include "MotorInterface.h"
#include "LightCtrlInterface.h"

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
                        static QState ManualIdle(Train * const me, QEvt const * const e);
                        static QState ManualActive(Train * const me, QEvt const * const e);
                            static QState ManualRest(Train * const me, QEvt const * const e);
                                static QState ManualLightWait(Train * const me, QEvt const * const e);
                                    static QState ManualRestLightWait(Train * const me, QEvt const * const e);
                                    static QState ManualRunningLightWait(Train * const me, QEvt const * const e);
                                static QState ManualRestReady(Train * const me, QEvt const * const e);
                            static QState ManualAccel(Train * const me, QEvt const * const e);
                            static QState ManualDecel(Train * const me, QEvt const * const e);
                                static QState ManualRestWait(Train * const me, QEvt const * const e);
                                static QState ManualIdleWait(Train * const me, QEvt const * const e);
                            static QState ManualRunning(Train * const me, QEvt const * const e);
                    static QState Auto(Train * const me, QEvt const * const e);
                        static QState AutoIdle(Train * const me, QEvt const * const e);
                        static QState AutoActive(Train * const me, QEvt const * const e);
                            static QState AutoRest(Train * const me, QEvt const * const e);
                                static QState AutoRestLightOn(Train * const me, QEvt const * const e);
                                static QState AutoRestWait(Train * const me, QEvt const * const e);
                                static QState AutoRestLightOff(Train * const me, QEvt const * const e);
                            static QState AutoAccel(Train * const me, QEvt const * const e);
                                static QState AutoRunningLightOn(Train * const me, QEvt const * const e);
                                static QState AutoMotorAccel(Train * const me, QEvt const * const e);
                            static QState AutoDecel(Train * const me, QEvt const * const e);
                                static QState AutoDecelRest(Train * const me, QEvt const * const e);
                                    static QState AutoMotorRest(Train * const me, QEvt const * const e);
                                        static QState AutoMotorSlowing(Train * const me, QEvt const * const e);
                                        static QState AutoMotorCoasting(Train * const me, QEvt const * const e);
                                        static QState AutoMotorStopping(Train * const me, QEvt const * const e);
                                    static QState AutoRunningLightOff(Train * const me, QEvt const * const e);
                                static QState AutoDecelIdle(Train * const me, QEvt const * const e);
                                    static QState AutoMotorIdle(Train * const me, QEvt const * const e);
                                    static QState AutoAllLightOff(Train * const me, QEvt const * const e);
                            static QState AutoRunning(Train * const me, QEvt const * const e);
                static QState Remote(Train * const me, QEvt const * const e);
                    static QState Commanded(Train * const me, QEvt const * const e);
                    static QState Scheduled(Train * const me, QEvt const * const e);

    MotorDir GetDir();
    LightCtrlOp GetRunningLightCtrlOp() {
        return GetDir() == MotorDir::FORWARD ? LightCtrlOp::HEADLIGHT_RUNNING_FORWARD : LightCtrlOp::HEADLIGHT_RUNNING_BACKWARD;
    }
    LightCtrlOp GetRestLightCtrlOp() {
        return GetDir() == MotorDir::BACKWARD ? LightCtrlOp::HEADLIGHT_REST_BACKWARD : LightCtrlOp::HEADLIGHT_REST_FORWARD;
    }

    Evt m_inEvt;                // Static event copy of a generic incoming req to be confirmed. Added more if needed.
    Timer m_stateTimer;
    Timer m_lightCtrlTimer;
    Timer m_motorTimer;
    Timer m_restTimer;
    Timer m_runTimer;
    Timer m_coastTimer;
    Motor m_motor;              // Orthogonal region for motor control.
    MsgSeqRec m_lightCtrlSeq;   // Keeps track of sequence numbers of outgoing messages to LIGHT_CTRL.
    MsgSeqRec m_motorSeq;       // Keeps track of sequence numbers of outgoing messages to MOTOR.
    bool m_pull;                // True if train is pulling. False if pushing.
    bool m_drive;               // True if train is driving. False if reversing.
    bool m_autoDirect;          // True if train does not stop at intermediate stations in Auto state.
    uint32_t m_magnetCount;     // Magnet counter to detect the next station.
    uint32_t m_stationCount;    // Station counter to detect the end station (terminal).
    uint16_t m_decel;           // Variable deceleration.

    enum {
        LIGHT_CTRL_OP_TIMEOUT_MS = 6100,
        // The following timeout periods are configurable.
        REST_TIMEOUT_MS = 6000,
        RUNNING_LIGHT_ON_WAIT_MS = 3000,
        RUNNING_LIGHT_OFF_WAIT_MS = 3000,
        MOTOR_PENDING_WAIT_MS = 500,    // Max time to wait for pending confirmation from MOTOR.
        MOTOR_DONE_WAIT_MS = 60000,     // Max time to wait for final/done confirmation from MOTOR.
        RUN_TIMEOUT_MS = 50000,
        COAST_TIMEOUT_MS = 120000,      // Max coasting time.
        RUN_LEVEL_DEFAULT = 850,        // PWM duty-cycle permil level (0-1000).
        RUN_LEVEL_PUSH = 875,           // PWM duty-cycle permil level (0-1000).
        RUN_LEVEL_PULL = 925,           // PWM duty-cycle permil level (0-1000).
        ACCEL_DEFAULT = 80,             // PWM permil steps per second.
        DECEL_DEFAULT = 80,             // PWM permil steps per second.
        DECEL_BREAK = 500,              // PWM permil steps per second.
        DECEL_STOP = 80,                // PWM permil steps per second.
        COAST_LEVEL = 200,
        MAGNET_COUNT_ARRIVING = 3,
        MAGNET_COUNT_ARRIVED = 4,
        STATION_COUNT_TOTAL = 3
    };

#define TRAIN_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(LIGHT_CTRL_TIMER) \
    ADD_EVT(MOTOR_TIMER) \
    ADD_EVT(REST_TIMER) \
    ADD_EVT(RUN_TIMER) \
    ADD_EVT(COAST_TIMER)

#define TRAIN_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(LIGHT_DONE) \
    ADD_EVT(MOTOR_DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(BTN_A_PRESS) \
    ADD_EVT(BTN_B_PRESS) \
    ADD_EVT(BTN_A_HOLD) \
    ADD_EVT(BTN_B_HOLD) \
    ADD_EVT(HALL_DETECT) \
    ADD_EVT(MANUAL_MODE) \
    ADD_EVT(AUTO_MODE) \
    ADD_EVT(ARRIVING) \
    ADD_EVT(ARRIVED) \
    ADD_EVT(MISSED_STOP)

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

    class HoldEvt : public Evt {
    public:
        HoldEvt(QSignal signal, uint32_t count) :
            Evt(signal), m_count(count) {}
        uint32_t GetCount() const { return m_count; }
    private:
        uint32_t m_count;
    };

    class BtnAHold : public HoldEvt {
    public:
        BtnAHold(uint32_t count) :
            HoldEvt(BTN_A_HOLD, count) {}
    };

    class BtnBHold : public HoldEvt {
    public:
        BtnBHold(uint32_t count) :
            HoldEvt(BTN_B_HOLD, count) {}
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason = 0) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };

    class MissedStop : public ErrorEvt {
    public:
        MissedStop(Error error, Hsmn origin, Reason reason = 0) :
            ErrorEvt(MISSED_STOP, error, origin, reason) {}
    };
};

} // namespace APP

#endif // TRAIN_H
