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
#include "Ws2812Interface.h"
#include "LightPattern.h"
#include "LightInterface.h"
#include "LightCtrlInterface.h"
#include "LightCtrl.h"

FW_DEFINE_THIS_FILE("LightCtrl.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "LIGHT_CTRL_TIMER_EVT_START",
    LIGHT_CTRL_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "LIGHT_CTRL_INTERNAL_EVT_START",
    LIGHT_CTRL_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "LIGHT_CTRL_INTERFACE_EVT_START",
    LIGHT_CTRL_INTERFACE_EVT
};

bool LightCtrl::Perform(LightCtrlOp op) {
    switch(op) {
        case LightCtrlOp::ALL_OFF: {
            for (uint32_t i = 0; i < ARRAY_COUNT(m_light); i++) {
                SendReq(new LightOffReq, LIGHT + i, (i == 0));
            }
            return true;
        }
        case LightCtrlOp::ALL_FADE: {
            for (uint32_t i = 0; i < ARRAY_COUNT(m_light); i++) {
                SendReq(new LightSetReq(0, FADE_MS), LIGHT + i, (i == 0));
            }
            return true;
        }
        case LightCtrlOp::HEADLIGHT_REST_FORWARD: {
            SendReq(new LightPatternReq(LightPatternIdx::BREATHING_WHITE, true, 0), FRONT_LIGHT_0, true);
            SendReq(new LightPatternReq(LightPatternIdx::BREATHING_WHITE, true, 0), FRONT_LIGHT_1, false);
            SendReq(new LightPatternReq(LightPatternIdx::BREATHING_RED, true, 0), REAR_LIGHT_0, false);
            SendReq(new LightPatternReq(LightPatternIdx::BREATHING_RED, true, 0), REAR_LIGHT_1, false);
            return true;
        }
        case LightCtrlOp::HEADLIGHT_REST_BACKWARD: {
            SendReq(new LightPatternReq(LightPatternIdx::BREATHING_RED, true, 0), FRONT_LIGHT_0, true);
            SendReq(new LightPatternReq(LightPatternIdx::BREATHING_RED, true, 0), FRONT_LIGHT_1, false);
            SendReq(new LightPatternReq(LightPatternIdx::BREATHING_WHITE, true, 0), REAR_LIGHT_0, false);
            SendReq(new LightPatternReq(LightPatternIdx::BREATHING_WHITE, true, 0), REAR_LIGHT_1, false);
            return true;
        }
        case LightCtrlOp::HEADLIGHT_RUNNING_FORWARD: {
            SendReq(new LightSetReq(0xffffff, RAMP_ON_MS), FRONT_LIGHT_0, true);
            SendReq(new LightSetReq(0xffffff, RAMP_ON_MS), FRONT_LIGHT_1, false);
            SendReq(new LightSetReq(0x0000ff, RAMP_ON_MS), REAR_LIGHT_0, false);
            SendReq(new LightSetReq(0x0000ff, RAMP_ON_MS), REAR_LIGHT_1, false);
            return true;
        }
        case LightCtrlOp::HEADLIGHT_RUNNING_BACKWARD: {
            SendReq(new LightSetReq(0x0000ff, RAMP_ON_MS), FRONT_LIGHT_0, true);
            SendReq(new LightSetReq(0x0000ff, RAMP_ON_MS), FRONT_LIGHT_1, false);
            SendReq(new LightSetReq(0xffffff, RAMP_ON_MS), REAR_LIGHT_0, false);
            SendReq(new LightSetReq(0xffffff, RAMP_ON_MS), REAR_LIGHT_1, false);
            return true;
        }
        case LightCtrlOp::HEADLIGHT_ALERT: {
            SendReq(new LightPatternReq(LightPatternIdx::ALERT_RED_BLUE), FRONT_LIGHT_0, true);
            SendReq(new LightPatternReq(LightPatternIdx::ALERT_RED_BLUE, true, 12), FRONT_LIGHT_1, false);
            SendReq(new LightPatternReq(LightPatternIdx::ALERT_RED), REAR_LIGHT_0, false);
            SendReq(new LightPatternReq(LightPatternIdx::ALERT_BLUE), REAR_LIGHT_1, false);
            return true;
        }
        case LightCtrlOp::HEADLIGHT_WARNING: {
            SendReq(new LightPatternReq(LightPatternIdx::ALERT_AMBER_WHITE), FRONT_LIGHT_0, true);
            SendReq(new LightPatternReq(LightPatternIdx::ALERT_AMBER_WHITE, true, 12), FRONT_LIGHT_1, false);
            SendReq(new LightPatternReq(LightPatternIdx::ALERT_AMBER_WHITE), REAR_LIGHT_0, false);
            SendReq(new LightPatternReq(LightPatternIdx::ALERT_AMBER_WHITE, true, 12), REAR_LIGHT_1, false);
            return true;
        }
        default: {
            break;
        }
    }
    return false;
}

LightCtrl::LightCtrl() :
    Active((QStateHandler)&LightCtrl::InitialPseudoState, LIGHT_CTRL, "LIGHT_CTRL"),
    // Currently, each LIGHT region controls a single LED, but it may control a group of LEDs showing the same pattern.
    // Since some LEDs on an LED strip are skipped, the total no. of LEDs configured for WS2812 is LED_COUNT (0 - 7).
    m_light{
        { FRONT_LIGHT_0, "FRONT_LIGHT_0", 0, 1, 0 },
        { FRONT_LIGHT_1, "FRONT_LIGHT_1", 3, 1, 0  },
        { REAR_LIGHT_0, "REAR_LIGHT_0",   4, 1, 0  },
        { REAR_LIGHT_1, "REAR_LIGHT_1",   7, 1, 0  }
    },
    m_manager(HSM_UNDEF), m_inEvt(QEvt::STATIC_EVT), m_inMsg(QEvt::STATIC_EVT),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_busyTimer(GetHsmn(), BUSY_TIMER)  {
    SET_EVT_NAME(LIGHT_CTRL);
}

QState LightCtrl::InitialPseudoState(LightCtrl * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&LightCtrl::Root);
}

QState LightCtrl::Root(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_light); i++) {
                me->m_light[i].Init(me);
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LightCtrl::Stopped);
        }
        case LIGHT_CTRL_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LightCtrlStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case LIGHT_CTRL_OP_REQ: {
            auto const &req = MSG_EVT_CAST(*e);
            MSG_EVENT(req);
            me->SendCfmMsg(new LightCtrlOpCfm(MSG_ERROR_STATE, req.GetMsgTo()), req);
            return Q_HANDLED();
        }
        case LIGHT_CTRL_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&LightCtrl::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState LightCtrl::Stopped(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_manager = HSM_UNDEF;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LIGHT_CTRL_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LightCtrlStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case LIGHT_CTRL_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_manager = req.GetFrom();
            me->m_inEvt = req;
            return Q_TRAN(&LightCtrl::Starting);
        }
    }
    return Q_SUPER(&LightCtrl::Root);
}

QState LightCtrl::Starting(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            constexpr uint32_t timeout = LightCtrlStartReq::TIMEOUT_MS;
            static_assert(timeout > (Ws2812StartReq::TIMEOUT_MS + LightStartReq::TIMEOUT_MS));
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LightCtrl::Starting1);
        }
        case LIGHT_EXCEPTION_IND: {
            ErrorEvt const &ind = ERROR_EVT_CAST(*e);
            ERROR_EVENT(ind);
            me->Raise(new Failed(ind.GetError(), ind.GetOrigin(), ind.GetReason()));
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new LightCtrlStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new LightCtrlStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&LightCtrl::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new LightCtrlStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&LightCtrl::Started);
        }
    }
    return Q_SUPER(&LightCtrl::Root);
}

QState LightCtrl::Starting1(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // It configures WS2812 with the number of physical LEDs to be used on the LED strip (LED_COUNT)
            // rather than the number of LIGHT regions (LIGHT_COUNT) since some physical LEDs are skipped.
            me->SendReq(new Ws2812StartReq(LED_COUNT), WS2812, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WS2812_START_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            ERROR_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(NEXT));
            }
            return Q_HANDLED();
        }
        case NEXT: {
            EVENT(e);
            return Q_TRAN(&LightCtrl::Starting2);
        }
    }
    return Q_SUPER(&LightCtrl::Starting);
}

QState LightCtrl::Starting2(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_light); i++) {
                me->SendReq(new LightStartReq(WS2812), LIGHT + i, (i == 0));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LIGHT_START_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            ERROR_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new LightCtrlStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&LightCtrl::Started);
        }
    }
    return Q_SUPER(&LightCtrl::Starting);
}

QState LightCtrl::Stopping(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            constexpr uint32_t timeout = LightCtrlStopReq::TIMEOUT_MS;
            static_assert(timeout > (Ws2812StopReq::TIMEOUT_MS + LightStopReq::TIMEOUT_MS));
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LightCtrl::Stopping1);
        }
        case LIGHT_CTRL_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
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
            return Q_TRAN(&LightCtrl::Stopped);
        }
    }
    return Q_SUPER(&LightCtrl::Root);
}

QState LightCtrl::Stopping1(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_light); i++) {
                me->SendReq(new LightStopReq(), LIGHT + i, (i == 0));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LIGHT_STOP_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            ERROR_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(NEXT));
            }
            return Q_HANDLED();
        }
        case NEXT: {
            EVENT(e);
            return Q_TRAN(&LightCtrl::Stopping2);
        }
    }
    return Q_SUPER(&LightCtrl::Stopping);
}

QState LightCtrl::Stopping2(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReq(new Ws2812StopReq(), WS2812, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WS2812_STOP_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            ERROR_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&LightCtrl::Stopped);
        }
    }
    return Q_SUPER(&LightCtrl::Stopping);
}

QState LightCtrl::Started(LightCtrl * const me, QEvt const * const e) {
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
            return Q_TRAN(&LightCtrl::Idle);
        }
        case LIGHT_EXCEPTION_IND: {
            ErrorEvt const &ind = ERROR_EVT_CAST(*e);
            ERROR_EVENT(ind);
            me->Send(new LightCtrlExceptionInd(ind.GetError(), ind.GetOrigin(), ind.GetReason()), me->m_manager);
            return Q_TRAN(&LightCtrl::Exception);
        }
    }
    return Q_SUPER(&LightCtrl::Root);
}

QState LightCtrl::Idle(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LIGHT_CTRL_OP_REQ: {
            EVENT(e);
            auto const &req = static_cast<LightCtrlOpReq const &>(*e);
            if (me->Perform(req.GetOp())) {
                me->m_inMsg = req;
                me->Raise(new Evt(IN_PROGRESS));
            } else {
                me->SendCfmMsg(new LightCtrlOpCfm(MSG_ERROR_PARAM, req.GetMsgTo(), LIGHT_CTRL_MSG_REASON_INVALID_CMD), req);
            }
            return Q_HANDLED();
        }
        case IN_PROGRESS: {
            EVENT(e);
            return Q_TRAN(&LightCtrl::Busy);
        }
    }
    return Q_SUPER(&LightCtrl::Started);
}

QState LightCtrl::Busy(LightCtrl * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            constexpr uint32_t timeout = LightCtrlOpReq::TIMEOUT_MS;
            static_assert((timeout > LightSetReq::TIMEOUT_MS) && (timeout > LightPatternReq::TIMEOUT_MS) &&
                          (timeout > LightOffReq::TIMEOUT_MS));
            me->m_busyTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_busyTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&LightCtrl::Idle);
        }
        case LIGHT_SET_CFM:
        case LIGHT_PATTERN_CFM:
        case LIGHT_OFF_CFM: {
            auto const &cfm = ERROR_EVT_CAST(*e);
            ERROR_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->SendCfmMsg(new LightCtrlOpCfm(MSG_ERROR_SUCCESS, me->m_inMsg.GetMsgTo()), me->m_inMsg);
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case BUSY_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                auto const &failed = ERROR_EVT_CAST(*e);
                // @todo Translate from event reason to message reason.
                me->SendCfmMsg(new LightCtrlOpCfm(ToMsgError(failed.GetError()), me->m_inMsg.GetMsgTo()), me->m_inMsg);
            } else {
                me->SendCfmMsg(new LightCtrlOpCfm(MSG_ERROR_TIMEOUT, me->m_inMsg.GetMsgTo()), me->m_inMsg);
            }
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case LIGHT_CTRL_OP_REQ: {
            EVENT(e);
            me->Defer(e);
            me->SendCfmMsg(new LightCtrlOpCfm(MSG_ERROR_ABORTED, me->m_inMsg.GetMsgTo()), me->m_inMsg);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&LightCtrl::Started);
}

QState LightCtrl::Exception(LightCtrl * const me, QEvt const * const e) {
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
    return Q_SUPER(&LightCtrl::Root);
}

/*
QState LightCtrl::MyState(LightCtrl * const me, QEvt const * const e) {
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
            return Q_TRAN(&LightCtrl::SubState);
        }
    }
    return Q_SUPER(&LightCtrl::SuperState);
}
*/

} // namespace APP
