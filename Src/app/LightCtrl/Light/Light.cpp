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
#include "bsp.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "Gamma.h"
#include "Ws2812Interface.h"
#include "LightInterface.h"
#include "Light.h"

FW_DEFINE_THIS_FILE("Light.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "LIGHT_TIMER_EVT_START",
    LIGHT_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "LIGHT_INTERNAL_EVT_START",
    LIGHT_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "LIGHT_INTERFACE_EVT_START",
    LIGHT_INTERFACE_EVT
};

// Calculates m_rampCnt, m_stepRed, m_stepGreen, m_stepBlue from m_setColor, m_setRampMs and m_color.
void Light::CalRampSteps() {
    m_rampCnt = ROUND_UP_DIV(m_setRampMs, INTERVAL_TIMEOUT_MS);
    if (m_rampCnt == 0) {
        m_rampCnt = 1;
    }
    m_stepRed = (static_cast<float>(m_setColor.Red()) - m_startColor.Red()) / m_rampCnt;
    m_stepGreen = (static_cast<float>(m_setColor.Green()) - m_startColor.Green()) / m_rampCnt;
    m_stepBlue = (static_cast<float>(m_setColor.Blue()) - m_startColor.Blue()) / m_rampCnt;
}

// Updates color based on m_startColor, m_stepXxx and m_rampIdx.
void Light::UpdateRampColor() {
    // When (m_rampIdx + 1) == m_rampCnt, color is equal to set color.
    FW_ASSERT(m_rampIdx < m_rampCnt);
    uint8_t red = m_startColor.Red() + m_stepRed * (m_rampIdx + 1);
    uint8_t green = m_startColor.Green() + m_stepGreen * (m_rampIdx + 1);
    uint8_t blue = m_startColor.Blue() + m_stepBlue * (m_rampIdx + 1);
    m_currColor.Set(red, green, blue);
}

// Fills the specified color (888 xBGR) to the internal WS2812 color buffer.
void Light::FillColors(Color888Bgr const &color) {
    FW_ASSERT((m_ledCnt == 0) ||
              ((m_ledIdx  + ((m_ledCnt - 1) * m_ledInc)) < Ws2812SetReq::MAX_LED_COUNT));
    FW_ASSERT(m_ledCnt <= ARRAY_COUNT(m_colors));
    uint32_t idx = m_ledIdx;
    for (uint32_t i = 0; i < m_ledCnt; i++) {
        m_colors[i].Set(idx, color.Gamma());
        idx += m_ledInc;
    }
}

Light::Light(Hsmn hsmn, char const *name, uint32_t ledIdx, uint32_t ledCnt, uint32_t ledInc) :
    Region((QStateHandler)&Light::InitialPseudoState, hsmn, name),
    m_ledIdx(ledIdx), m_ledCnt(ledCnt), m_ledInc(ledInc), m_manager(HSM_UNDEF), m_ws2812Hsmn(HSM_UNDEF),
    m_stateTimer(GetHsmn(), STATE_TIMER),
    m_ws2812Timer(GetHsmn(), WS2812_TIMER), m_intervalTimer(GetHsmn(), INTERVAL_TIMER),
    m_ws2812Seq(HSM_UNDEF), m_currColor(0), m_startColor(0), m_setColor(0), m_setRampMs(0), m_rampIdx(0), m_rampCnt(0),
    m_stepRed(0.0), m_stepGreen(0.0), m_stepBlue(0.0), m_patternSet(LIGHT_PATTERN_SET),
    m_currPattern(nullptr), m_intervalIndex(0), m_isRepeat(false), m_nextTimeMs(0),  m_inEvt(QEvt::STATIC_EVT) {
    SET_EVT_NAME(LIGHT);
    FW_ASSERT(static_cast<uint32_t>(LightPatternIdx::COUNT) == m_patternSet.GetCount());
}

QState Light::InitialPseudoState(Light * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Light::Root);
}

QState Light::Root(Light * const me, QEvt const * const e) {
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
            return Q_TRAN(&Light::Stopped);
        }
        case LIGHT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LightStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case LIGHT_SET_REQ: {
            EVENT(e);
            auto const &req = EVT_CAST(*e);
            me->SendCfm(new LightSetCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case LIGHT_PATTERN_REQ: {
            EVENT(e);
            auto const &req = EVT_CAST(*e);
            me->SendCfm(new LightPatternCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case LIGHT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Light::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Light::Stopped(Light * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_manager = HSM_UNDEF;
            me->m_ws2812Hsmn = HSM_UNDEF;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LIGHT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LightStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case LIGHT_START_REQ: {
            EVENT(e);
            auto const &req = static_cast<LightStartReq const &>(*e);
            me->m_manager = req.GetFrom();
            me->m_ws2812Hsmn = req.GetWs2812Hsmn();
            me->SendCfm(new LightStartCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&Light::Started);
        }
    }
    return Q_SUPER(&Light::Root);
}

QState Light::Stopping(Light * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_currColor.Set(0);
            me->FillColors(me->m_currColor);
            me->SendReq(new Ws2812SetReq(me->m_colors, me->m_ledCnt), me->m_ws2812Hsmn, true, me->m_ws2812Seq);
            me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_ws2812Timer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case LIGHT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case WS2812_SET_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived, me->m_ws2812Seq)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }

        case FAILED:
        case WS2812_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Light::Stopped);
        }
    }
    return Q_SUPER(&Light::Root);
}

QState Light::Started(Light * const me, QEvt const * const e) {
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
            return Q_TRAN(&Light::Off);
        }
        case WS2812_SET_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived, me->m_ws2812Seq)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->m_ws2812Timer.Stop();
            }
            return Q_HANDLED();
        }
        case LIGHT_OFF_REQ: {
            EVENT(e);
            auto const &req = EVT_CAST(*e);
            me->SendCfm(new LightOffCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&Light::Off);
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Light::Off);
        }
        case LIGHT_PATTERN_REQ: {
            EVENT(e);
            auto const &req = static_cast<LightPatternReq const &>(*e);
            me->SendCfm(new LightSetCfm(ERROR_ABORTED), me->m_inEvt);
            uint32_t patternIdx = static_cast<uint32_t>(req.GetPatternIndex());
            if (patternIdx < me->m_patternSet.GetCount()) {
                me->m_currPattern = me->m_patternSet.GetPattern(patternIdx);
                FW_ASSERT(me->m_currPattern);
                me->m_isRepeat = req.IsRepeat();
                me->m_intervalIndex = req.GetStartInterval() % me->m_currPattern->GetCount();
                me->m_nextTimeMs = GetSystemMs();
                me->SendCfm(new LightPatternCfm(ERROR_SUCCESS), req);
                return Q_TRAN(&Light::Pattern);
            } else {
                ERROR("Invalid pattern idx %lu", patternIdx);
                me->SendCfm(new LightPatternCfm(ERROR_PARAM, me->GetHsmn(), LIGHT_REASON_INVALID_PATTERN), req);
                return Q_HANDLED();
            }
        }
        case LIGHT_SET_REQ: {
            EVENT(e);
            auto const &req = static_cast<LightSetReq const &>(*e);
            me->SendCfm(new LightSetCfm(ERROR_ABORTED), me->m_inEvt);
            me->m_inEvt = req;
            me->m_setColor = req.GetColor();
            me->m_setRampMs = req.GetRampMs();
            return Q_TRAN(&Light::On);
        }
        case FAILED:
        case WS2812_TIMER: {
            EVENT(e);
            return Q_TRAN(&Light::Faulted);
        }
    }
    return Q_SUPER(&Light::Root);
}

QState Light::Off(Light * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_currColor.Set(0);
            me->FillColors(me->m_currColor);
            me->SendReq(new Ws2812SetReq(me->m_colors, me->m_ledCnt), me->m_ws2812Hsmn, true, me->m_ws2812Seq);
            me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LIGHT_OFF_REQ: {
            EVENT(e);
            auto const &req = EVT_CAST(*e);
            me->SendCfm(new LightOffCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Light::Started);
}

QState Light::On(Light * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_rampIdx = 0;
            me->m_startColor = me->m_currColor;
            me->CalRampSteps();
            me->m_intervalTimer.Start(INTERVAL_TIMEOUT_MS, Timer::PERIODIC);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_intervalTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Light::Ramping);
        }
    }
    return Q_SUPER(&Light::Started);
}

QState Light::Ramping(Light * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->UpdateRampColor();
            me->FillColors(me->m_currColor);
            LOG("currColor = %x", me->m_currColor.Gamma());
            me->SendReq(new Ws2812SetReq(me->m_colors, me->m_ledCnt), me->m_ws2812Hsmn, true, me->m_ws2812Seq);
            me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case INTERVAL_TIMER: {
            EVENT(e);
            FW_ASSERT(me->m_rampCnt > 0);
            if (me->m_rampIdx < (me->m_rampCnt - 1)) {
                me->Raise(new Evt(CONTINUE));
            } else if (me->m_rampIdx == (me->m_rampCnt - 1)) {
                me->Raise(new Evt(DONE));
            } else {
                FW_ASSERT(0);
            }
            return Q_HANDLED();
        }
        case CONTINUE: {
            EVENT(e);
            ++me->m_rampIdx;
            return Q_TRAN(&Light::Ramping);
        }
        case DONE: {
            EVENT(e);
            me->m_intervalTimer.Stop();
            return Q_TRAN(&Light::Stable);
        }
    }
    return Q_SUPER(&Light::On);
}

QState Light::Stable(Light * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            if (me->m_currColor != me->m_setColor) {
                me->m_currColor = me->m_setColor;
                me->FillColors(me->m_currColor);
                LOG("currColor = %x", me->m_currColor.Gamma());
                me->SendReq(new Ws2812SetReq(me->m_colors, me->m_ledCnt), me->m_ws2812Hsmn, true, me->m_ws2812Seq);
                me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            }
            me->SendCfm(new LightSetCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Light::On);
}

QState Light::Pattern(Light * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            auto const &currInterval = me->m_currPattern->GetInterval(me->m_intervalIndex);
            me->m_currColor = currInterval.GetLedColor();
            me->m_nextTimeMs += currInterval.GetDurationMs();
            me->m_intervalTimer.Start(Timer::GetTimeoutMs(me->m_nextTimeMs));
            me->FillColors(me->m_currColor.Get());
            me->SendReq(new Ws2812SetReq(me->m_colors, me->m_ledCnt), me->m_ws2812Hsmn, true, me->m_ws2812Seq);
            me->m_ws2812Timer.Restart(WS2812_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_intervalTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            if (me->m_isRepeat) {
                return Q_TRAN(&Light::Repeating);
            } else {
                return Q_TRAN(&Light::Once);
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
        case NEXT_INTERVAL: {
            EVENT(e);
            ++me->m_intervalIndex;
            return Q_TRAN(&Light::Pattern);
        }
        case LAST_INTERVAL: {
            EVENT(e);
            me->m_intervalIndex = 0;
            return Q_TRAN(&Light::Pattern);
        }
    }
    return Q_SUPER(&Light::Started);
}

QState Light::Repeating(Light * const me, QEvt const * const e) {
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
    return Q_SUPER(&Light::Pattern);
}

QState Light::Once(Light * const me, QEvt const * const e) {
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
    return Q_SUPER(&Light::Pattern);
}

QState Light::Faulted(Light * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // @todo Refines fault handling.
            FW_ASSERT(false);
            me->m_ws2812Timer.Stop();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LIGHT_OFF_REQ:
        case LIGHT_SET_REQ:
        case LIGHT_PATTERN_REQ: {
            EVENT(e);
            LOG("Request ignored in Faulted state");
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Light::Started);
}

/*
QState Light::MyState(Light * const me, QEvt const * const e) {
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
            return Q_TRAN(&Light::SubState);
        }
    }
    return Q_SUPER(&Light::SuperState);
}
*/

} // namespace APP

