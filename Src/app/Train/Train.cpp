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
#include "TestPins.h"
#include "GpioInInterface.h"
#include "MotorInterface.h"
#include "LightCtrlInterface.h"
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
    m_stateTimer(GetHsmn(), STATE_TIMER), m_lightCtrlTimer(GetHsmn(), LIGHT_CTRL_TIMER),
    m_motorTimer(GetHsmn(), MOTOR_TIMER), m_restTimer(GetHsmn(), REST_TIMER),
    m_runTimer(GetHsmn(), RUN_TIMER), m_coastTimer(GetHsmn(), COAST_TIMER),
    m_lightCtrlSeq(""), m_motorSeq(""), m_pull(false), m_drive(true), m_autoDirect(true),
    m_magnetCount(0), m_stationCount(1), m_decel(DECEL_DEFAULT) {
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
            // Initialize region.
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
            constexpr uint32_t timeout = TrainStartReq::TIMEOUT_MS;
            static_assert(timeout > LightCtrlStartReq::TIMEOUT_MS);
            static_assert(timeout > MotorStartReq::TIMEOUT_MS);
            static_assert(timeout > GpioInStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            // Test only - Disables lightCtrl to avoid conflict with GuiMgr using Adafruit LCD module.
            me->SendReq(new LightCtrlStartReq(), LIGHT_CTRL, true);
            me->SendReq(new MotorStartReq(), MOTOR, false);
            me->SendReq(new GpioInStartReq(), BTN_A, false);
            me->SendReq(new GpioInStartReq(), BTN_B, false);
            me->SendReq(new GpioInStartReq(), HALL_SENSOR, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case LIGHT_CTRL_START_CFM:
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
            constexpr uint32_t timeout = TrainStopReq::TIMEOUT_MS;
            static_assert(timeout > LightCtrlStopReq::TIMEOUT_MS);
            static_assert(timeout > MotorStopReq::TIMEOUT_MS);
            static_assert(timeout > GpioInStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new LightCtrlStopReq(), LIGHT_CTRL, true);
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
        case LIGHT_CTRL_STOP_CFM:
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
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::Normal);
        }
        case GPIO_IN_PULSE_IND: {
            EVENT(e);
            auto const &ind = EVT_CAST(*e);
            Hsmn from = ind.GetFrom();
            if (from == BTN_A) {
                me->Raise(new Evt(BTN_A_PRESS));
            } else if (from == BTN_B) {
                me->Raise(new Evt(BTN_B_PRESS));
            }
            return Q_HANDLED();
        }
        case GPIO_IN_HOLD_IND: {
            EVENT(e);
            auto const &ind = static_cast<GpioInHoldInd const &>(*e);
            Hsmn from = ind.GetFrom();
            if (from == BTN_A) {
                me->Raise(new BtnAHold(ind.GetCount()));
            } else if (from == BTN_B) {
                me->Raise(new BtnBHold(ind.GetCount()));
            }
            return Q_HANDLED();
        }
        case GPIO_IN_INACTIVE_IND: {
            //TEST_PIN_TOGGLE(TP_1);
            EVENT(e);
            auto const &ind = EVT_CAST(*e);
            Hsmn from = ind.GetFrom();
            if (from == HALL_SENSOR) {
                me->Raise(new Evt(HALL_DETECT));
            }
            //TEST_PIN_TOGGLE(TP_1);
            return Q_HANDLED();
        }
        case FAILED: {
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
            return Q_TRAN(&Train::Auto);
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
            return Q_TRAN(&Train::ManualIdle);
        }
        case AUTO_MODE: {
            return Q_TRAN(&Train::Auto);
        }
    }
    return Q_SUPER(&Train::Local);
}

QState Train::ManualIdle(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::ManualActive);
        }
        case BTN_B_PRESS: {
            EVENT(e);
            me->Raise(new Evt(AUTO_MODE));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::Manual);
}

QState Train::ManualActive(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::ManualRunningLightWait);
        }
        case BTN_A_PRESS: {
            EVENT(e);
            return Q_TRAN(&Train::ManualDecel);
        }
        case TRAIN_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Train::ManualIdleWait);
        }
    }
    return Q_SUPER(&Train::Manual);
}

QState Train::ManualRest(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::ManualLightWait);
        }
    }
    return Q_SUPER(&Train::ManualActive);
}

QState Train::ManualLightWait(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            static_assert(LIGHT_CTRL_OP_TIMEOUT_MS > static_cast<uint32_t>(LightCtrlOpReq::TIMEOUT_MS));
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_lightCtrlTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::ManualRestLightWait);
        }
        case LIGHT_CTRL_OP_CFM: {
            EVENT(e);
            auto const &cfm = static_cast<LightCtrlOpCfm const &>(*e);
            bool allReceived;
            if (!me->CheckCfmMsg(cfm, allReceived, me->m_lightCtrlSeq)) {
                me->Raise(new Failed(ToEvtError(cfm.GetMsgError()), cfm.GetFrom(), TRAIN_REASON_LIGHT_CTRL_OP_FAILED));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
                me->m_lightCtrlTimer.Stop();
            }
            return Q_HANDLED();
        }
        case LIGHT_CTRL_TIMER: {
            EVENT(e);
            me->Raise(new Failed(ERROR_TIMEOUT, me->GetHsmn()));
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Train::ManualRestReady);
        }
    }
    return Q_SUPER(&Train::ManualRest);
}


QState Train::ManualRestLightWait(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new LightCtrlOpReq(me->GetRestLightCtrlOp()), LIGHT_CTRL, MSG_UNDEF, true, me->m_lightCtrlSeq);
            me->m_lightCtrlTimer.Start(LIGHT_CTRL_OP_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::ManualLightWait);
}

QState Train::ManualRunningLightWait(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new LightCtrlOpReq(me->GetRunningLightCtrlOp()), LIGHT_CTRL, MSG_UNDEF, true, me->m_lightCtrlSeq);
            me->m_lightCtrlTimer.Start(LIGHT_CTRL_OP_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Train::ManualAccel);
        }
    }
    return Q_SUPER(&Train::ManualLightWait);
}

QState Train::ManualRestReady(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::ManualRunningLightWait);
        }
        case BTN_A_HOLD: {
            EVENT(e);
            me->SendReqMsg(new LightCtrlOpReq(LightCtrlOp::ALL_OFF), LIGHT_CTRL, MSG_UNDEF, true, me->m_lightCtrlSeq);
            return Q_TRAN(&Train::ManualIdle);
        }
    }
    return Q_SUPER(&Train::ManualRest);
}

QState Train::ManualAccel(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new MotorRunReq(me->GetDir(), RUN_LEVEL_DEFAULT, ACCEL_DEFAULT, DECEL_DEFAULT), MOTOR, MSG_UNDEF, true, me->m_motorSeq);
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
                me->Raise(new Failed(ToEvtError(cfm.GetMsgError()), cfm.GetFrom(), TRAIN_REASON_MOTOR_RUN_FAILED));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Train::ManualRunning);
        }
    }
    return Q_SUPER(&Train::ManualActive);
}

QState Train::ManualDecel(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new MotorRunReq(me->GetDir(), 0, ACCEL_DEFAULT, DECEL_DEFAULT), MOTOR, MSG_UNDEF, true, me->m_motorSeq);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::ManualRestWait);
        }
        case MOTOR_RUN_CFM: {
            auto const &cfm = static_cast<MotorRunCfm const &>(*e);
            ERROR_MSG_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfmMsg(cfm, allReceived, me->m_motorSeq)) {
                me->Raise(new Failed(ToEvtError(cfm.GetMsgError()), cfm.GetFrom(), TRAIN_REASON_MOTOR_RUN_FAILED));
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
            return Q_TRAN(&Train::ManualRest);
        }
    }
    return Q_SUPER(&Train::ManualActive);
}

QState Train::ManualRestWait(Train * const me, QEvt const * const e) {
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
    return Q_SUPER(&Train::ManualDecel);
}

QState Train::ManualIdleWait(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::ManualIdle);
        }
    }
    return Q_SUPER(&Train::ManualDecel);
}

QState Train::ManualRunning(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::ManualDecel);
        }
    }
    return Q_SUPER(&Train::ManualActive);
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
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::AutoIdle);
        }
        case MANUAL_MODE: {
            return Q_TRAN(&Train::Manual);
        }
    }
    return Q_SUPER(&Train::Local);
}


QState Train::AutoIdle(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_pull = false;
            me->m_drive = true;
            me->m_autoDirect = true;
            me->m_stationCount = 1;
            me->m_magnetCount = 0;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case BTN_A_PRESS: {
            EVENT(e);
            return Q_TRAN(&Train::AutoActive);
        }
        case BTN_A_HOLD: {
            auto const &hold = static_cast<BtnAHold const &>(*e);
            if (hold.GetCount() == 1) {
                EVENT(e);
                me->m_autoDirect = !me->m_autoDirect;
            }
            return Q_HANDLED();
        }
        case BTN_B_PRESS: {
            EVENT(e);
            me->Raise(new Evt(MANUAL_MODE));
            return Q_HANDLED();
        }
        case MISSED_STOP: {
            EVENT(e);
            // @todo - Shows error message on LCD
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::Auto);
}

QState Train::AutoActive(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            static_assert(LIGHT_CTRL_OP_TIMEOUT_MS > static_cast<uint32_t>(LightCtrlOpReq::TIMEOUT_MS));
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::AutoAccel);
        }
        case HALL_DETECT: {
            EVENT(e);
            if (++me->m_magnetCount == MAGNET_COUNT_ARRIVING) {
                me->Raise(new Evt(ARRIVING));
            }
            else if (me->m_magnetCount >= MAGNET_COUNT_ARRIVED) {
                me->Raise(new Evt(ARRIVED));
            }
            return Q_HANDLED();
        }
        case LIGHT_CTRL_OP_CFM: {
            EVENT(e);
            auto const &cfm = static_cast<LightCtrlOpCfm const &>(*e);
            bool allReceived;
            if (!me->CheckCfmMsg(cfm, allReceived, me->m_lightCtrlSeq)) {
                me->Raise(new Failed(ToEvtError(cfm.GetMsgError()), cfm.GetFrom(), TRAIN_REASON_LIGHT_CTRL_OP_FAILED));
            } else if (allReceived) {
                me->Raise(new Evt(LIGHT_DONE));
            }
            return Q_HANDLED();
        }
        case LIGHT_CTRL_TIMER: {
            EVENT(e);
            me->Raise(new Failed(ERROR_TIMEOUT, me->GetHsmn()));
            return Q_HANDLED();
        }
        case MOTOR_RUN_CFM: {
            auto const &cfm = static_cast<MotorRunCfm const &>(*e);
            ERROR_MSG_EVENT(cfm);
            bool allReceived;
            bool pending;
            if (!me->CheckCfmMsg(cfm, allReceived, me->m_motorSeq, &pending)) {
                me->Raise(new Failed(ToEvtError(cfm.GetMsgError()), cfm.GetFrom(), TRAIN_REASON_MOTOR_RUN_FAILED));
            } else if (pending) {
                me->m_motorTimer.Restart(MOTOR_DONE_WAIT_MS);
            } else if (allReceived) {
                me->Raise(new Evt(MOTOR_DONE));
            }
            return Q_HANDLED();
        }
        case MOTOR_TIMER: {
            EVENT(e);
            me->Raise(new Failed(ERROR_TIMEOUT, me->GetHsmn()));
            return Q_HANDLED();
        }
        case ARRIVING: {
            EVENT(e);
            if (!me->m_autoDirect || ((me->m_stationCount + 1) >= STATION_COUNT_TOTAL)) {
                return Q_TRAN(&Train::AutoDecel);
            }
            break;
        }
        case ARRIVED: {
            EVENT(e);
            FW_ASSERT(me->m_autoDirect);
            FW_ASSERT((me->m_stationCount + 1) < STATION_COUNT_TOTAL);
            me->m_stationCount++;
            me->m_magnetCount = 0;
            return Q_HANDLED();
        }
        case TRAIN_STOP_REQ:
        case MISSED_STOP: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Train::AutoDecelIdle);
        }
        case BTN_A_PRESS: {
            EVENT(e);
            return Q_TRAN(&Train::AutoDecelIdle);
        }
    }
    return Q_SUPER(&Train::Auto);
}

QState Train::AutoRest(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            if (++me->m_stationCount >= STATION_COUNT_TOTAL) {
                me->m_pull = !me->m_pull;
                me->m_stationCount = 1;
            }
            me->m_magnetCount = 0;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::AutoRestLightOn);
        }
        case BTN_A_HOLD: {
            auto const &hold = static_cast<BtnAHold const &>(*e);
            if (hold.GetCount() == 1) {
                EVENT(e);
                me->m_autoDirect = !me->m_autoDirect;
            }
            return Q_HANDLED();
        }
        case LIGHT_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoAccel);
        }
    }
    return Q_SUPER(&Train::AutoActive);
}

QState Train::AutoRestLightOn(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new LightCtrlOpReq(me->GetRestLightCtrlOp()), LIGHT_CTRL, MSG_UNDEF, true, me->m_lightCtrlSeq);
            me->m_lightCtrlTimer.Start(LIGHT_CTRL_OP_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_lightCtrlTimer.Stop();
            return Q_HANDLED();
        }
        case LIGHT_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoRestWait);
        }
    }
    return Q_SUPER(&Train::AutoRest);
}

QState Train::AutoRestWait(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::AutoRestLightOff);
        }
    }
    return Q_SUPER(&Train::AutoRest);
}

QState Train::AutoRestLightOff(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new LightCtrlOpReq(LightCtrlOp::ALL_OFF), LIGHT_CTRL, MSG_UNDEF, true, me->m_lightCtrlSeq);
            me->m_lightCtrlTimer.Start(LIGHT_CTRL_OP_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_lightCtrlTimer.Stop();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::AutoRest);
}

QState Train::AutoAccel(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::AutoRunningLightOn);
        }
        case MOTOR_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoRunning);
        }
    }
    return Q_SUPER(&Train::AutoActive);
}

QState Train::AutoRunningLightOn(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new LightCtrlOpReq(me->GetRunningLightCtrlOp()), LIGHT_CTRL, MSG_UNDEF, true, me->m_lightCtrlSeq);
            me->m_lightCtrlTimer.Start(LIGHT_CTRL_OP_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_lightCtrlTimer.Stop();
            return Q_HANDLED();
        }
        case LIGHT_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoMotorAccel);
        }
    }
    return Q_SUPER(&Train::AutoAccel);
}


QState Train::AutoMotorAccel(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint16_t runLevel = me->m_pull ? RUN_LEVEL_PULL : RUN_LEVEL_PUSH;
            me->SendReqMsg(new MotorRunReq(me->GetDir(), runLevel, ACCEL_DEFAULT, DECEL_DEFAULT), MOTOR, MSG_UNDEF, true, me->m_motorSeq);
            me->m_motorTimer.Start(MOTOR_PENDING_WAIT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_motorTimer.Stop();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::AutoAccel);
}

QState Train::AutoDecel(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::AutoDecelRest);
        }
        case ARRIVING: {
            EVENT(e);
            // Ignored.
            return Q_HANDLED();
        }
        case LIGHT_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoRest);
        }
    }
    return Q_SUPER(&Train::AutoActive);
}

QState Train::AutoDecelRest(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::AutoMotorRest);
        }
    }
    return Q_SUPER(&Train::AutoDecel);
}

QState Train::AutoMotorRest(Train * const me, QEvt const * const e) {
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
            return Q_TRAN(&Train::AutoMotorSlowing);
        }
        case ARRIVED: {
            EVENT(e);
            me->m_decel = DECEL_BREAK;
            return Q_TRAN(&Train::AutoMotorStopping);
        }
        case MOTOR_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoRunningLightOff);
        }
    }
    return Q_SUPER(&Train::AutoDecelRest);
}

QState Train::AutoMotorSlowing(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new MotorRunReq(me->GetDir(), COAST_LEVEL, ACCEL_DEFAULT, DECEL_DEFAULT), MOTOR, MSG_UNDEF, true, me->m_motorSeq);
            me->m_motorTimer.Start(MOTOR_PENDING_WAIT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_motorTimer.Stop();
            return Q_HANDLED();
        }
        case MOTOR_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoMotorCoasting);
        }
    }
    return Q_SUPER(&Train::AutoMotorRest);
}

QState Train::AutoMotorCoasting(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_coastTimer.Start(COAST_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_coastTimer.Stop();
            return Q_HANDLED();
        }
        case COAST_TIMER: {
            EVENT(e);
            me->Raise(new MissedStop(ERROR_TIMEOUT, me->GetHsmn(), TRAIN_REASON_COASTING_TIMEOUT));
            return Q_HANDLED();
        }
        case ARRIVED: {
            EVENT(e);
            me->m_decel = DECEL_STOP;
            return Q_TRAN(&Train::AutoMotorStopping);
        }
    }
    return Q_SUPER(&Train::AutoMotorRest);
}

QState Train::AutoMotorStopping(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new MotorRunReq(me->GetDir(), 0, ACCEL_DEFAULT, me->m_decel), MOTOR, MSG_UNDEF, true, me->m_motorSeq);
            me->m_motorTimer.Start(MOTOR_PENDING_WAIT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_motorTimer.Stop();
            return Q_HANDLED();
        }
        case MOTOR_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoRunningLightOff);
        }
    }
    return Q_SUPER(&Train::AutoMotorRest);
}

QState Train::AutoRunningLightOff(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new LightCtrlOpReq(LightCtrlOp::ALL_FADE), LIGHT_CTRL, MSG_UNDEF, true, me->m_lightCtrlSeq);
            me->m_lightCtrlTimer.Start(LIGHT_CTRL_OP_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_lightCtrlTimer.Stop();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::AutoDecelRest);
}

QState Train::AutoDecelIdle(Train * const me, QEvt const * const e) {
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
        case Q_INIT_SIG: {
            return Q_TRAN(&Train::AutoMotorIdle);
        }
        case TRAIN_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case BTN_A_PRESS: {
            EVENT(e);
            // Ignored.
            return Q_HANDLED();
        }
        case LIGHT_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoIdle);
        }
    }
    return Q_SUPER(&Train::AutoDecel);
}

QState Train::AutoMotorIdle(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new MotorRunReq(me->GetDir(), 0, ACCEL_DEFAULT, DECEL_BREAK), MOTOR, MSG_UNDEF, true, me->m_motorSeq);
            me->m_motorTimer.Start(MOTOR_PENDING_WAIT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_motorTimer.Stop();
            return Q_HANDLED();
        }
        case MOTOR_DONE: {
            EVENT(e);
            return Q_TRAN(&Train::AutoAllLightOff);
        }
    }
    return Q_SUPER(&Train::AutoDecelIdle);
}

QState Train::AutoAllLightOff(Train * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReqMsg(new LightCtrlOpReq(LightCtrlOp::ALL_OFF), LIGHT_CTRL, MSG_UNDEF, true, me->m_lightCtrlSeq);
            me->m_lightCtrlTimer.Start(LIGHT_CTRL_OP_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_lightCtrlTimer.Stop();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Train::AutoDecelIdle);
}

QState Train::AutoRunning(Train * const me, QEvt const * const e) {
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
    return Q_SUPER(&Train::AutoActive);
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
