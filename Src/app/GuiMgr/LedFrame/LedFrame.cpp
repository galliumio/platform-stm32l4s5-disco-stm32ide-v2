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
#include "fw_macro.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "LedFrameInterface.h"
#include "LedFrame.h"
#include "DispInterface.h"
#include "LCDConf_Lin_Template.h"

FW_DEFINE_THIS_FILE("LedFrame.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "LED_FRAME_TIMER_EVT_START",
    LED_FRAME_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "LED_FRAME_INTERNAL_EVT_START",
    LED_FRAME_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "LED_FRAME_INTERFACE_EVT_START",
    LED_FRAME_INTERFACE_EVT
};

LedFrame::LedFrame() :
    Region((QStateHandler)&LedFrame::InitialPseudoState, LED_FRAME, "LED_FRAME"),
    m_frameBuf(lcdBuf), m_frameBufLen(sizeof(lcdBuf)), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER) {
    FW_ASSERT(sizeof(m_dmaBuf) >= m_frameBufLen);
    SET_EVT_NAME(LED_FRAME);
}

QState LedFrame::InitialPseudoState(LedFrame * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&LedFrame::Root);
}

QState LedFrame::Root(LedFrame * const me, QEvt const * const e) {
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
            return Q_TRAN(&LedFrame::Stopped);
        }
        case LED_FRAME_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LedFrameStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case LED_FRAME_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&LedFrame::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState LedFrame::Stopped(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LED_FRAME_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LedFrameStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case LED_FRAME_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&LedFrame::Starting);
        }
    }
    return Q_SUPER(&LedFrame::Root);
}

QState LedFrame::Starting(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LedFrameStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > DispStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            // @todo Currently hardcodes rotation mode to 3. To be passed in via req.
            me->SendReq(new DispStartReq(3), ILI9341, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new LedFrameStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new LedFrameStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&LedFrame::Stopping);
        }
        case DISP_START_CFM: {
            EVENT(e);
            DispStartCfm const &cfm = static_cast<DispStartCfm const &>(*e);
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
            me->SendCfm(new LedFrameStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&LedFrame::Started);
        }
    }
    return Q_SUPER(&LedFrame::Root);
}

QState LedFrame::Stopping(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LedFrameStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > DispStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new DispStopReq(), ILI9341, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case LED_FRAME_STOP_REQ: {
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
        case DISP_STOP_CFM: {
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
        case DONE: {
            EVENT(e);
            return Q_TRAN(&LedFrame::Stopped);
        }
    }
    return Q_SUPER(&LedFrame::Root);
}

QState LedFrame::Started(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // @todo Need to wait for response.
            me->Send(new DispDrawBeginReq(), ILI9341);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            // @todo Need to wait for response.
            me->Send(new DispDrawEndReq(), ILI9341);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LedFrame::Idle);
        }
    }
    return Q_SUPER(&LedFrame::Root);
}

QState LedFrame::Idle(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LED_FRAME_UPDATE_REQ: {
            EVENT(e);
            auto const &req = static_cast<LedFrameUpdateReq const &>(*e);
            me->m_inEvt = req;
            me->m_updateArea = req.GetArea();
            return Q_TRAN(&LedFrame::Busy);
        }
    }
    return Q_SUPER(&LedFrame::Started);
}

QState LedFrame::Busy(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LedFrameUpdateReq::TIMEOUT_MS;
            FW_ASSERT(timeout > DispDrawBitmapReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            // Copies pixel data (2 bytes each) in the specified area from frameBuf to dmaBuf.
            // Swap bytes to convert from little-endian to big-endia.
            // @todo - Use Chrom-art DMA2D to offload to hardware accelerator.
            uint32_t x = me->m_updateArea.GetX();
            uint32_t y = me->m_updateArea.GetY();
            uint32_t w = me->m_updateArea.GetWidth();
            uint32_t h = me->m_updateArea.GetHeight();
            if (((x + w) > XSIZE_PHYS) || ((y + h) > YSIZE_PHYS)) {
                ERROR("Invalid area x=%d, y=%d, w=%d, h=%d", x, y, w, h);
                // Avoids using Raise() in entry action. OK to use Send().
                me->Send(new Failed(ERROR_PARAM, me->GetHsmn()), me->GetHsmn());
                return Q_HANDLED();
            }
            if (me->m_updateArea.IsEmpty()) {
                me->Send(new Evt(DONE));
                return Q_HANDLED();
            }
            // Range-checking for m_frameBuf.
            // m_dmaBuf has been checked in constructor to >= m_frameBufLen.
            FW_ASSERT(((y + h - 1) * XSIZE_PHYS + x + w) * 2 <= me->m_frameBufLen);
            uint32_t dmaLen = 0;
            for (uint32_t ypos = y; ypos < y + h; ypos++) {
                for (uint32_t xpos = x; xpos < x + w; xpos++) {
                    uint32_t idx = (ypos * XSIZE_PHYS + xpos)*2;
                    me->m_dmaBuf[dmaLen++] = me->m_frameBuf[idx + 1];
                    me->m_dmaBuf[dmaLen++] = me->m_frameBuf[idx];
                }
            }
            FW_ASSERT(dmaLen);
            // @todo For MCU with cache, add cache flushing/cleaning here.
            me->SendReq(new DispDrawBitmapReq(me->m_updateArea, me->m_dmaBuf, dmaLen), ILI9341, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->Recall();
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case STATE_TIMER: {
            EVENT(e);
            me->Raise(new Failed(ERROR_TIMEOUT, me->GetHsmn()));
            return Q_HANDLED();
        }
        case LED_FRAME_STOP_REQ:
        case LED_FRAME_UPDATE_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case DISP_DRAW_BITMAP_CFM: {
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
            me->SendCfm(new LedFrameUpdateCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&LedFrame::Idle);
        }
        case FAILED: {
            ErrorEvt const &failed = ERROR_EVT_CAST(*e);
            ERROR_EVENT(failed);
            me->SendCfm(new LedFrameUpdateCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            return Q_TRAN(&LedFrame::Idle);
        }
    }
    return Q_SUPER(&LedFrame::Started);
}

/*
QState LedFrame::MyState(LedFrame * const me, QEvt const * const e) {
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
            return Q_TRAN(&LedFrame::SubState);
        }
    }
    return Q_SUPER(&LedFrame::SuperState);
}
*/

} // namespace APP
