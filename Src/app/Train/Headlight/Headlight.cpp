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
#include "HeadlightInterface.h"
#include "Headlight.h"

FW_DEFINE_THIS_FILE("Headlight.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "HEADLIGHT_TIMER_EVT_START",
    HEADLIGHT_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "HEADLIGHT_INTERNAL_EVT_START",
    HEADLIGHT_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "HEADLIGHT_INTERFACE_EVT_START",
    HEADLIGHT_INTERFACE_EVT
};

// Calculates m_rampCnt, m_stepRed, m_stepGreen, m_stepBlue from m_setColor, m_setRampMs and m_color.
void Headlight::CalRampSteps() {
    m_rampCnt = ROUND_UP_DIV(m_setRampMs, INTERVAL_TIMEOUT_MS);
    if (m_rampCnt == 0) {
        m_rampCnt = 1;
    }
    m_stepRed = (static_cast<float>(m_setColor.Red()) - m_color.Red()) / m_rampCnt;
    m_stepGreen = (static_cast<float>(m_setColor.Green()) - m_color.Green()) / m_rampCnt;
    m_stepBlue = (static_cast<float>(m_setColor.Blue()) - m_color.Blue()) / m_rampCnt;
    m_savedColor = m_color;
}

// Updates color based on m_stepXxx and m_rampIdx.
void Headlight::UpdateRampColor() {
    uint8_t red = m_savedColor.Red() + m_stepRed * m_rampIdx;
    uint8_t green = m_savedColor.Green() + m_stepGreen * m_rampIdx;
    uint8_t blue = m_savedColor.Blue() + m_stepBlue * m_rampIdx;
    m_color.Set(red, green, blue);
}

Headlight::Headlight() :
    Region((QStateHandler)&Headlight::InitialPseudoState, HEADLIGHT, "HEADLIGHT"),
    m_inEvt(QEvt::STATIC_EVT), m_inMsg(QEvt::STATIC_EVT), m_stateTimer(GetHsmn(), STATE_TIMER),
    m_ws2812Timer(GetHsmn(), WS2812_TIMER), m_intervalTimer(GetHsmn(), INTERVAL_TIMER),
    m_ws2812Seq(HSM_UNDEF), m_color(0), m_savedColor(0), m_setColor(0), m_setRampMs(0), m_rampIdx(0), m_rampCnt(0),
    m_stepRed(0.0), m_stepGreen(0.0), m_stepBlue(0.0), m_patternSet(HEADLIGHT_PATTERN_SET),
    m_currPattern(nullptr), m_intervalIndex(0), m_isRepeat(false) {
    SET_EVT_NAME(HEADLIGHT);
}

QState Headlight::InitialPseudoState(Headlight * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Headlight::Root);
}

QState Headlight::Root(Headlight * const me, QEvt const * const e) {
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
            return Q_TRAN(&Headlight::Stopped);
        }
        case HEADLIGHT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new HeadlightStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case HEADLIGHT_SET_REQ: {
            EVENT(e);
            auto const &req = MSG_EVT_CAST(*e);
            me->SendCfmMsg(new HeadlightSetCfm(MSG_ERROR_STATE, req.GetMsgTo()), req);
            return Q_HANDLED();
        }
        case HEADLIGHT_PATTERN_REQ: {
            EVENT(e);
            auto const &req = MSG_EVT_CAST(*e);
            me->SendCfmMsg(new HeadlightPatternCfm(MSG_ERROR_STATE, req.GetMsgTo()), req);
            return Q_HANDLED();
        }
        case HEADLIGHT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Headlight::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Headlight::Stopped(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case HEADLIGHT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new HeadlightStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case HEADLIGHT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&Headlight::Starting);
        }
    }
    return Q_SUPER(&Headlight::Root);
}

QState Headlight::Starting(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = HeadlightStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > Ws2812StartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new Ws2812StartReq(HEADLIGHT_COUNT), WS2812, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case WS2812_START_CFM: {
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
                me->SendCfm(new HeadlightStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new HeadlightStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Headlight::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new HeadlightStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Headlight::Started);
        }
    }
    return Q_SUPER(&Headlight::Root);
}

QState Headlight::Stopping(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = HeadlightStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > Ws2812StopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new Ws2812StopReq(), WS2812, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case HEADLIGHT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case WS2812_STOP_CFM: {
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
            return Q_TRAN(&Headlight::Stopped);
        }
    }
    return Q_SUPER(&Headlight::Root);
}

QState Headlight::Started(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_ws2812Timer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Headlight::Off);
        }
        case WS2812_SET_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->m_ws2812Timer.Stop();
            }
            return Q_HANDLED();
        }
        case HEADLIGHT_OFF_REQ: {
            EVENT(e);
            auto const &req = MSG_EVT_CAST(*e);
            me->SendCfmMsg(new HeadlightOffCfm(MSG_ERROR_SUCCESS), req);
            return Q_TRAN(&Headlight::Off);
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Headlight::Off);
        }
        case HEADLIGHT_PATTERN_REQ: {
            EVENT(e);
            auto const &req = static_cast<HeadlightPatternReq const &>(*e);
            if (req.GetPatternIndex() < me->m_patternSet.GetCount()) {
                me->m_isRepeat = req.IsRepeat();
                me->m_intervalIndex = 0;
                me->m_currPattern = me->m_patternSet.GetPattern(req.GetPatternIndex());
                FW_ASSERT(me->m_currPattern);
                me->SendCfmMsg(new HeadlightPatternCfm(MSG_ERROR_SUCCESS), req);
                return Q_TRAN(&Headlight::Pattern);
            } else {
                ERROR("Invalid pattern idx %lu", req.GetPatternIndex());
                me->SendCfmMsg(new HeadlightPatternCfm(MSG_ERROR_PARAM, req.GetMsgTo(), MSG_HEADLIGHT_REASON_INVALID_PATTERN), req);
                return Q_HANDLED();
            }
        }
        case HEADLIGHT_SET_REQ: {
            EVENT(e);
            auto const &req = static_cast<HeadlightSetReq const &>(*e);
            me->m_setColor = req.GetColor();
            me->m_setRampMs = req.GetRampMs();
            me->m_inMsg = req;
            return Q_TRAN(&Headlight::On);
        }
        case FAILED:
        case WS2812_TIMER: {
            EVENT(e);
            return Q_TRAN(&Headlight::Faulted);
        }
    }
    return Q_SUPER(&Headlight::Root);
}

QState Headlight::Off(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_color.Set(0);
            Ws2812SetReq::LedColor colors[] = {{0, me->m_color.Get()}, {3, me->m_color.Get()}};
            me->SendReq(new Ws2812SetReq(colors, ARRAY_COUNT(colors)), WS2812, true, me->m_ws2812Seq);
            me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case HEADLIGHT_OFF_REQ: {
            EVENT(e);
            auto const &req = MSG_EVT_CAST(*e);
            me->SendCfmMsg(new HeadlightOffCfm(MSG_ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Headlight::Started);
}

QState Headlight::On(Headlight * const me, QEvt const * const e) {
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
            return Q_TRAN(&Headlight::Ramping);
        }
        case HEADLIGHT_PATTERN_REQ:
        case HEADLIGHT_STOP_REQ: {
            EVENT(e);
            me->Raise(new Evt(DONE));
            me->Defer(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Headlight::Started);
}

QState Headlight::Ramping(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_intervalTimer.Start(INTERVAL_TIMEOUT_MS, Timer::PERIODIC);
            me->m_rampIdx = 0;
            me->CalRampSteps();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_intervalTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case INTERVAL_TIMER: {
            EVENT(e);
            me->UpdateRampColor();
            Ws2812SetReq::LedColor colors[] = {{0, me->m_color.Get()}, {3, me->m_color.Get()}};
            me->SendReq(new Ws2812SetReq(colors, ARRAY_COUNT(colors)), WS2812, true, me->m_ws2812Seq);
            me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            if ((++me->m_rampIdx) == me->m_rampCnt) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case HEADLIGHT_SET_REQ:
        case HEADLIGHT_PATTERN_REQ: {
            EVENT(e);
            me->Raise(new Evt(ABORT));
            me->Defer(e);
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            me->SendCfmMsg(new HeadlightSetCfm(MSG_ERROR_SUCCESS), me->m_inMsg);
            return Q_TRAN(&Headlight::Stable);
        }
        case ABORT: {
            EVENT(e);
            auto const &req = MSG_EVT_CAST(*e);
            me->SendCfmMsg(new HeadlightSetCfm(MSG_ERROR_ABORTED, req.GetMsgTo()), me->m_inMsg);
            return Q_TRAN(&Headlight::Aborted);
        }
    }
    return Q_SUPER(&Headlight::On);
}

QState Headlight::Stable(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            if (me->m_color != me->m_setColor) {
                me->m_color = me->m_setColor;
                Ws2812SetReq::LedColor colors[] = {{0, me->m_color.Get()}, {3, me->m_color.Get()}};
                me->SendReq(new Ws2812SetReq(colors, ARRAY_COUNT(colors)), WS2812, true, me->m_ws2812Seq);
                me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Headlight::On);
}

QState Headlight::Aborted(Headlight * const me, QEvt const * const e) {
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
    return Q_SUPER(&Headlight::On);
}

QState Headlight::Pattern(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            auto const &currInterval = me->m_currPattern->GetInterval(me->m_intervalIndex);
            me->m_intervalTimer.Start(currInterval.GetDurationMs());
            Ws2812SetReq::LedColor colors[] = {{0, currInterval.GetLed0Color()}, {3, currInterval.GetLed1Color()}};
            me->SendReq(new Ws2812SetReq(colors, ARRAY_COUNT(colors)), WS2812, true, me->m_ws2812Seq);
            me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_intervalTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            if (me->m_isRepeat) {
                return Q_TRAN(&Headlight::Repeating);
            } else {
                return Q_TRAN(&Headlight::Once);
            }
        }
        case INTERVAL_TIMER: {
            EVENT(e);
            uint32_t intervalCount = me->m_currPattern->GetCount();
            FW_ASSERT(intervalCount > 0);
            if (me->m_intervalIndex < (intervalCount - 1)) {
                me->Raise(new Evt(NEXT_INTERVAL));
            } else if (me->m_intervalIndex == (intervalCount - 1)) {
                me->Raise(new Evt(LAST_INTERVAL));
            } else {
                FW_ASSERT(0);
            }
            return Q_HANDLED();
        }
        case HEADLIGHT_SET_REQ:
        case HEADLIGHT_STOP_REQ: {
            EVENT(e);
            me->Raise(new Evt(DONE));
            me->Defer(e);
            return Q_HANDLED();
        }
        case NEXT_INTERVAL: {
            EVENT(e);
            me->m_intervalIndex++;
            return Q_TRAN(&Headlight::Pattern);
        }
        case LAST_INTERVAL: {
            EVENT(e);
            me->m_intervalIndex = 0;
            return Q_TRAN(&Headlight::Pattern);
        }
    }
    return Q_SUPER(&Headlight::Started);
}

QState Headlight::Repeating(Headlight * const me, QEvt const * const e) {
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
    return Q_SUPER(&Headlight::Pattern);
}

QState Headlight::Once(Headlight * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LAST_INTERVAL: {
            EVENT(e);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Headlight::Pattern);
}

QState Headlight::Faulted(Headlight * const me, QEvt const * const e) {
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
    return Q_SUPER(&Headlight::Started);
}

/*
QState Headlight::MyState(Headlight * const me, QEvt const * const e) {
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
            return Q_TRAN(&Headlight::SubState);
        }
    }
    return Q_SUPER(&Headlight::SuperState);
}
*/

} // namespace APP

