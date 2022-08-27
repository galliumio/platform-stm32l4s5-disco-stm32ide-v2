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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "GpioInInterface.h"
#include "MotorInterface.h"
#include "HeadlightInterface.h"
#include "TrainInterface.h"
#include "Train.h"

FW_DEFINE_THIS_FILE("Train.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "TRAIN_TIMER_EVT_START",
    TRAIN_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "TRAIN_INTERNAL_EVT_START",
    TRAIN_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "TRAIN_INTERFACE_EVT_START",
    TRAIN_INTERFACE_EVT
};

MotorDir Train::GetDir() {
    if (m_pull) {
        return m_drive ? MotorDir::FORWARD : MotorDir::BACKWARD;
    } else {
        return m_drive ? MotorDir::BACKWARD : MotorDir::FORWARD;
    }
}

Train::Train() :
    Active((QStateHandler)&Train::InitialPseudoState, TRAIN, "TRAIN"), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_headlightTimer(GetHsmn(), HEADLIGHT_TIMER),
    m_restTimer(GetHsmn(), REST_TIMER), m_runTimer(GetHsmn(), RUN_TIMER),
    m_headlightSeq(""), m_motorSeq(""), m_pull(false), m_drive(true) {
    SET_EVT_NAME(TRAIN);
}

QState Train::InitialPseudoState(Train * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Train::Root);
}

QState Train::Root(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Initialize regions.
            me->m_headlight.Init(me);
            me->m_motor.Init(me);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::Stopped);
        }
        case TRAIN_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new TrainStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case TRAIN_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Train::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Train::Stopped(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case TRAIN_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new TrainStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case TRAIN_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&Train::Starting);
        }
    }
    return Q_SUPER(&Train::Root);
}

QState Train::Starting(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = TrainStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > HeadlightStartReq::TIMEOUT_MS);
            FW_ASSERT(timeout > MotorStartReq::TIMEOUT_MS);
            FW_ASSERT(timeout > GpioInStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            // Test only - Disables headlight to avoid conflict with GuiMgr using Adafruit LCD module.
            me->SendReq(new HeadlightStartReq(), HEADLIGHT, true);
            me->SendReq(new MotorStartReq(), MOTOR, false);
            me->SendReq(new GpioInStartReq(), BTN_A, false);
            me->SendReq(new GpioInStartReq(), BTN_B, false);
            // Test only - Disables hall sensor before it's hooked up.
            //me->SendReq(new GpioInStartReq(), HALL_SENSOR, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case HEADLIGHT_START_CFM:
        case MOTOR_START_CFM:
        case GPIO_IN_START_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new TrainStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new TrainStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Train::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new TrainStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Train::Started);
        }
    }
    return Q_SUPER(&Train::Root);
}

QState Train::Stopping(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = TrainStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > HeadlightStopReq::TIMEOUT_MS);
            FW_ASSERT(timeout > MotorStopReq::TIMEOUT_MS);
            FW_ASSERT(timeout > GpioInStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new HeadlightStopReq(), HEADLIGHT, true);
            me->SendReq(new MotorStopReq(), MOTOR, false);
            me->SendReq(new GpioInStopReq(), BTN_A, false);
            me->SendReq(new GpioInStopReq(), BTN_B, false);
            me->SendReq(new GpioInStopReq(), HALL_SENSOR, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case TRAIN_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case HEADLIGHT_STOP_CFM:
        case MOTOR_STOP_CFM:
        case GPIO_IN_STOP_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Train::Stopped);
        }
    }
    return Q_SUPER(&Train::Root);
}

QState Train::Started(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Tests headlight color setting.
            uint32_t timeout = HEADLIGHT_SET_TIMEOUT_MS;
            FW_ASSERT(timeout > HeadlightSetReq::TIMEOUT_MS);
            // Test only - Disables headlight to avoid conflict with GuiMgr using Adafruit LCD module.
            me->m_headlightTimer.Start(timeout);
            me->SendReqMsg(new HeadlightSetReq(0x7f7f7f, 200), HEADLIGHT, MSG_UNDEF, true, me->m_headlightSeq);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_headlightTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::Normal);
        }
        // TestS headlight color setting.
        case HEADLIGHT_SET_CFM: {
            EVENT(e);
            auto const &cfm = static_cast<HeadlightSetCfm const &>(*e);
            bool allReceived;
            if (!me->CheckCfmMsg(cfm, allReceived, me->m_headlightSeq)) {
                me->Raise(new Failed(ERROR_MSG, HEADLIGHT, TRAIN_REASON_HEADLIGHT_SET_FAILED));
            } else if (allReceived) {
                me->m_headlightTimer.Stop();
            }
            return Q_HANDLED();
        }
        case GPIO_IN_PULSE_IND: {
            EVENT(e);
            auto const &ind = EVT_CAST(*e);
            Hsmn from = ind.GetFrom();
            if (from == BTN_A) {
                me->Raise(new Evt(BTN_A_PRESS));
            } else if (from == BTN_B) {
                me->Raise(new Evt(BTN_B_PRESS));
            } else if (from == HALL_DETECT) {
                me->Raise(new Evt(HALL_DETECT));
            }
            return Q_HANDLED();
        }
        case GPIO_IN_HOLD_IND: {
            EVENT(e);
            auto const &ind = EVT_CAST(*e);
            Hsmn from = ind.GetFrom();
            if (from == BTN_A) {
                me->Raise(new Evt(BTN_A_HOLD));
            } else if (from == BTN_B) {
                me->Raise(new Evt(BTN_B_HOLD));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case HEADLIGHT_TIMER: {
            EVENT(e);
            return Q_TRAN(&Train::Faulted);
        }
    }
    return Q_SUPER(&Train::Root);
}


QState Train::Faulted(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // @todo Refines fault handling.
            FW_ASSERT(false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::Started);
}

QState Train::Normal(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::Local);
        }
    }
    return Q_SUPER(&Train::Started);
}

QState Train::Local(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::Manual);
        }
    }
    return Q_SUPER(&Train::Normal);
}

QState Train::Manual(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::Idle);
        }
    }
    return Q_SUPER(&Train::Local);
}

QState Train::Idle(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_pull = false;
            me->m_drive = true;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case BTN_A_PRESS: {
            EVENT(e);
            return Q_TRAN(&Train::Activated);
        }
    }
    return Q_SUPER(&Train::Manual);
}

QState Train::Activated(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::Accel);
        }
        case BTN_A_PRESS: {
            EVENT(e);
            return Q_TRAN(&Train::Decel);
        }
        case TRAIN_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Train::IdleWait);
        }
    }
    return Q_SUPER(&Train::Manual);
}

QState Train::Rest(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_restTimer.Start(REST_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_restTimer.Stop();
            return Q_HANDLED();
        }
        case REST_TIMER:
        case BTN_A_PRESS: {
            EVENT(e);
            return Q_TRAN(&Train::Accel);
        }
        case BTN_A_HOLD: {
            EVENT(e);
            return Q_TRAN(&Train::Idle);
        }
    }
    return Q_SUPER(&Train::Activated);
}

QState Train::Accel(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new MotorRunReq(me->GetDir(), RUN_LEVEL, DEFAULT_ACCEL, DEFAULT_DECEL), MOTOR, MSG_UNDEF, true, me->m_motorSeq);
            // Test only.
            me->m_headlightTimer.Start(HEADLIGHT_SET_TIMEOUT_MS);
            me->SendReqMsg(new HeadlightSetReq(me->m_pull ? 0x7f7f7f : 0xff, 200), HEADLIGHT, MSG_UNDEF, true, me->m_headlightSeq);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case MOTOR_RUN_CFM: {
            auto const &cfm = static_cast<MotorRunCfm const &>(*e);
            ERROR_MSG_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfmMsg(cfm, allReceived, me->m_motorSeq)) {
                me->Raise(new Failed(ERROR_MSG, MOTOR, TRAIN_REASON_ACCEL_MOTOR_RUN_FAILED));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Train::Running);
        }
    }
    return Q_SUPER(&Train::Activated);
}

QState Train::Decel(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new MotorRunReq(me->GetDir(), 0, DEFAULT_ACCEL, DEFAULT_DECEL), MOTOR, MSG_UNDEF, true, me->m_motorSeq);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::RestWait);
        }
        case MOTOR_RUN_CFM: {
            auto const &cfm = static_cast<MotorRunCfm const &>(*e);
            ERROR_MSG_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfmMsg(cfm, allReceived, me->m_motorSeq)) {
                me->Raise(new Failed(ERROR_MSG, MOTOR, TRAIN_REASON_DECEL_MOTOR_RUN_FAILED));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case BTN_A_PRESS: {
            EVENT(e);
            // Ignored.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            me->m_pull = !me->m_pull;
            return Q_TRAN(&Train::Rest);
        }
    }
    return Q_SUPER(&Train::Activated);
}

QState Train::RestWait(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::Decel);
}

QState Train::IdleWait(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->Recall();
            return Q_HANDLED();
        }
        case TRAIN_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Train::Idle);
        }
    }
    return Q_SUPER(&Train::Decel);
}

QState Train::Running(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_runTimer.Start(RUN_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_runTimer.Stop();
            return Q_HANDLED();
        }
        case RUN_TIMER: {
            EVENT(e);
            return Q_TRAN(&Train::Decel);
        }
    }
    return Q_SUPER(&Train::Activated);
}

QState Train::Auto(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::Local);
}

QState Train::Remote(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::Commanded);
        }
    }
    return Q_SUPER(&Train::Normal);
}

QState Train::Commanded(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::Remote);
}

QState Train::Scheduled(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::Remote);
}

/*
QState Train::MyState(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::SubState);
        }
    }
    return Q_SUPER(&Train::SuperState);
}
*/

} // namespace APP
