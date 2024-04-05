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
#include "LedFrameInterface.h"
#include "GuiMgrInterface.h"
#include "GuiMgr.h"
#include "WM.h"
#include "LCDConf_Lin_Template.h"


FW_DEFINE_THIS_FILE("GuiMgr.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "GUI_MGR_TIMER_EVT_START",
    GUI_MGR_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "GUI_MGR_INTERNAL_EVT_START",
    GUI_MGR_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "GUI_MGR_INTERFACE_EVT_START",
    GUI_MGR_INTERFACE_EVT
};

// Bitmap structure created with EmWin utility (located under GuiMgr/Bitmaps)
extern "C" const GUI_BITMAP bmHierarki_320x90;
extern "C" const GUI_BITMAP bmHydrogen_320x90;
extern "C" const GUI_BITMAP bmRidingStatecharts_320x90;

static const GUI_BITMAP *logoBitmap[] =
{
    &bmHierarki_320x90,
    &bmHydrogen_320x90,
    &bmRidingStatecharts_320x90,
};

const GuiMgr::ScrollMap GuiMgr::m_text1ScrollMap[] = {
    { 4,  {100,  -6 }},
    { 3,  {100,  -4 }},
    { 2,  {100,  -2 }},
    { 1,  {100,  -1 }},
    { 0,  {2000,  -1 }}      // 5000
};

const GuiMgr::ScrollMap GuiMgr::m_bmpScrollMap[] = {
    { 40, {100,  -20 }},
    { 20, {100,  -8 }},
    { 8,  {100,  -4 }},
    { 4,  {100,  -2 }},
    { 1,  {100,  -1 }},
    { 0,  {2000,  -1 }}       // 5000
};

// Helper function to map offset to scroll object for speed control.
GuiMgr::Scroll GuiMgr::GetScroll(ScrollMap const *map, uint32_t mapLen, uint32_t offsetLeft, uint32_t offsetRight) {
    return GetScroll(map, mapLen, LESS(offsetLeft, offsetRight));
}
GuiMgr::Scroll GuiMgr::GetScroll(ScrollMap const *map, uint32_t mapLen, uint32_t offset) {
    uint32_t i;
    FW_ASSERT(map);
    for (i=0; i<mapLen; i++) {
        if (offset >= map[i].m_offset) {
            return map[i].m_scroll;
        }
    }
    FW_ASSERT(0);
    return Scroll();
}

void GuiMgr::SetDirty(Area const &area) {
    if (!m_dirtyAreaList.Find(area)) {
        if (m_dirtyAreaList.Write(&area, 1) < 1) {
            // If list is full, set entire LCD as dirty.
            m_dirtyAreaList.Reset();
            Area full(0, 0, XSIZE_PHYS, YSIZE_PHYS);
            bool result = m_dirtyAreaList.Write(&full, 1);
            FW_ASSERT(result == 1);
        }
    }
}

// Window Manager callback function,
void GuiMgr::WmCallback(WM_MESSAGE *msg) {
    FW_ASSERT(msg);
    if ((msg->MsgId != WM_CREATE) && (msg->MsgId != WM_DELETE) && (msg->MsgId != WM_NOTIFY_PARENT)) {
        // WM_CREATE is sent right after the window is created, before user data (obj) is set
        GuiMgr *obj = NULL;
        WM_GetUserData(msg->hWin, &obj, sizeof(obj));
        FW_ASSERT(obj);
        obj->WmHandler(msg);
    }
}

void GuiMgr::WmHandler(WM_MESSAGE *msg) {
    GuiMgr *me = this;  // For Log.
    FW_ASSERT(msg);
    switch (msg->MsgId) {
        case WM_PAINT: {
            HwinSig *result = m_hwinSigMap.GetByKey(msg->hWin);
            if (result) {
                // Note msg->Data.p points to a GUI_RECT structure containing the invalid rectangle.
                Raise(new WmEvent(result->GetValue(), *msg));
            } else {
                ERROR("WmHandler: WM_PAINT hWin (0x%x) not found", msg->hWin);
            }
            break;
        }
        default: {
            WM_DefaultProc(msg);
            //LOG("WmHandler: Calling WM_DefaultProc msgId=0x%x", msg->MsgId);
            break;
        }
    }
}

void GuiMgr::AddWin(WM_HWIN hwin, QP::QSignal sig) {
    FW_ASSERT(hwin && (sig != Q_USER_SIG));
    m_hwinSigMap.Save(HwinSig(hwin, sig));
}

void GuiMgr::RemoveWin(WM_HWIN hwin) {
    if (hwin) {
        m_hwinSigMap.ClearByKey(hwin);
    }
}

GuiMgr::GuiMgr() :
    Active((QStateHandler)&GuiMgr::InitialPseudoState, GUI_MGR, "GUI_MGR"),
    m_hwinSigMap(m_hwinSigStor, ARRAY_COUNT(m_hwinSigStor), HwinSig(0, Q_USER_SIG)),
    m_dirtyAreaList(m_dirtyAreaStor, DIRTY_AREA_ORDER), m_ticker1CycleCnt(0), m_bmpCycleCnt(0), m_updateIdx(0), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_syncTimer(GetHsmn(), SYNC_TIMER), m_bgWndTimer(GetHsmn(), BG_WND_TIMER),
    m_text1Timer(GetHsmn(), TEXT1_TIMER), m_bmpTimer(GetHsmn(), BMP_TIMER),
    m_updateTimer(GetHsmn(), UPDATE_TIMER){
    SET_EVT_NAME(GUI_MGR);
}

QState GuiMgr::InitialPseudoState(GuiMgr * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&GuiMgr::Root);
}

QState GuiMgr::Root(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_ledFrame.Init(me);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&GuiMgr::Stopped);
        }
        case GUI_MGR_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GuiMgrStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case GUI_MGR_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&GuiMgr::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState GuiMgr::Stopped(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case GUI_MGR_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GuiMgrStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case GUI_MGR_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&GuiMgr::Starting);
        }
    }
    return Q_SUPER(&GuiMgr::Root);
}

QState GuiMgr::Starting(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = GuiMgrStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > LedFrameStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new LedFrameStartReq(), LED_FRAME, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case LED_FRAME_START_CFM: {
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
                me->SendCfm(new GuiMgrStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new GuiMgrStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&GuiMgr::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new GuiMgrStartCfm(ERROR_SUCCESS),  me->m_inEvt);
            return Q_TRAN(&GuiMgr::Started);
        }
    }
    return Q_SUPER(&GuiMgr::Root);
}

QState GuiMgr::Stopping(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = GuiMgrStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > LedFrameStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new LedFrameStopReq(), LED_FRAME, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case GUI_MGR_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case LED_FRAME_STOP_CFM: {
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
            return Q_TRAN(&GuiMgr::Stopped);
        }
    }
    return Q_SUPER(&GuiMgr::Root);
}

QState GuiMgr::Started(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Activate the use of memory device feature.
            WM_SetCreateFlags(WM_CF_MEMDEV);
            // Init the STemWin GUI Library.
            GUI_Init();
            // Test only.
            GUI_EnableAlpha(0);
            me->m_syncTimer.Start(SYNC_TIMEOUT_MS, Timer::PERIODIC);

            me->m_dirtyAreaList.Reset();
            me->AddWin(me->m_bgWnd.Create(me, 0, 0, XSIZE_PHYS, YSIZE_PHYS, &WmCallback), PAINT_BGWND);
            me->m_bgWnd.SetColorIdx(128, 767, GuiBgWnd::GRADIENT_V);
            //me->m_bgWnd.SetColor(0x00ffffff, 0x00ffffff);
            //me->m_bgWndTimer.Start(BG_WND_TIMEOUT_MS, Timer::PERIODIC);
            WM_Exec();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->RemoveWin(me->m_bgWnd.Destroy());
            me->m_bgWndTimer.Stop();
            me->m_syncTimer.Stop();
            GUI_Exit();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&GuiMgr::TrainSign);
        }
        case BG_WND_TIMER: {
            EVENT(e);
            me->m_bgWnd.Update(1);
            WM_Exec();
            return Q_HANDLED();
        }
        case PAINT_BGWND:{
            EVENT(e);
            //WmEvent const &wmEvt = static_cast<WmEvent const &>(*e);
            //LOG("PAINT_BGWND msgId=0x%x", wmEvt.GetMsgId());
            me->SetDirty(me->m_bgWnd.Paint());
            return Q_HANDLED();
        }
        case SYNC_TIMER: {
            //EVENT(e);
            Area area;
            while (me->GetDirty(area)) {
                me->Send(new LedFrameUpdateReq(area), LED_FRAME);
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GuiMgr::Root);
}

QState GuiMgr::TrainSign(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Logo.
            me->AddWin(me->m_bmp.Create(me, me->m_bgWnd.GetHandle(), 0, 0, 320, 90, &WmCallback), PAINT_BMP);
            uint32_t bmpCnt = ARRAY_COUNT(logoBitmap);
            for (uint32_t i=0; i<bmpCnt && i<GuiBmp::IMG_CNT; i++) {
                me->m_bmp.SetBitmap(i, logoBitmap[i]);
            }
            me->m_bmpScroll = GetScroll(m_bmpScrollMap, ARRAY_COUNT(m_bmpScrollMap), 0, 0);
            FW_ASSERT(me->m_bmpScroll.m_timeoutMs && me->m_bmpScroll.m_delta);
            me->m_bmpTimer.Start(me->m_bmpScroll.m_timeoutMs);
            // Origin station (m_ticker1).
            me->AddWin(me->m_ticker1.Create(me, me->m_bgWnd.GetHandle(), 0, 90, 320, 50, 0x3f0000, &WmCallback), PAINT_TICKER1);
            me->m_ticker1.SetText(0, "Upper Garden Main =>", GUI_FONT_32B_1, 0x7f7f7f, 0x3f0000, GUI_TA_LEFT|GUI_TA_VCENTER);
            // Destination station (m_ticker2).
            me->AddWin(me->m_ticker2.Create(me, me->m_bgWnd.GetHandle(), 0, 140, 320, 50, 0x3f0000, &WmCallback), PAINT_TICKER2);
            me->m_ticker2.SetText(0, "Lower Garden Glacier", GUI_FONT_32B_1, 0x7f7f7f, 0x3f0000, GUI_TA_RIGHT|GUI_TA_VCENTER);
            // "via" (m_ticker3)
            me->AddWin(me->m_ticker3.Create(me, me->m_bgWnd.GetHandle(), 0, 190, 50, 50, 0x1f0000, &WmCallback), PAINT_TICKER3);
            me->m_ticker3.SetText(0, "via", GUI_FONT_24_ASCII, 0x7f7f7f, 0x1f0000, GUI_TA_HCENTER|GUI_TA_VCENTER);
            // Stop stations (m_text1).
            me->AddWin(me->m_text1.Create(me, me->m_bgWnd.GetHandle(), 50, 190, 270, 50, 0x1f0000, &WmCallback), PAINT_TEXT1);
            me->m_text1.SetText(0, "Hillside Garden",
                                GUI_FONT_24B_1, 0x7f7f7f, 0x1f0000);
            me->m_text1.SetText(1, "Mid Garden Main",
                                GUI_FONT_24B_1, 0x7f7f7f, 0x1f0000);
            me->m_text1Scroll = GetScroll(m_text1ScrollMap, ARRAY_COUNT(m_text1ScrollMap), 0, 0);
            FW_ASSERT(me->m_text1Scroll.m_timeoutMs && me->m_text1Scroll.m_delta);
            me->m_text1Timer.Start(me->m_text1Scroll.m_timeoutMs);
            WM_Exec();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->RemoveWin(me->m_bmp.Destroy());
            me->RemoveWin(me->m_ticker1.Destroy());
            me->RemoveWin(me->m_ticker2.Destroy());
            me->RemoveWin(me->m_ticker3.Destroy());
            me->RemoveWin(me->m_text1.Destroy());
            me->m_bmpTimer.Stop();
            me->m_text1Timer.Stop();
            return Q_HANDLED();
        }
        case PAINT_BMP: {
            EVENT(e);
            me->SetDirty(me->m_bmp.Paint());
            return Q_HANDLED();
        }
        case PAINT_TICKER1: {
            EVENT(e);
            me->SetDirty(me->m_ticker1.Paint());
            return Q_HANDLED();
        }
        case PAINT_TICKER2: {
            EVENT(e);
            me->SetDirty(me->m_ticker2.Paint());
            return Q_HANDLED();
        }
        case PAINT_TICKER3: {
            EVENT(e);
            me->SetDirty(me->m_ticker3.Paint());
            return Q_HANDLED();
        }
        case PAINT_TEXT1: {
            EVENT(e);
            me->SetDirty(me->m_text1.Paint());
            return Q_HANDLED();
        }
        case BMP_TIMER: {
            EVENT(e);
            uint32_t imgIdx;
            uint32_t offsetLeft;
            uint32_t offsetRight;
            me->m_bmp.Update(me->m_bmpScroll.m_delta, imgIdx, offsetLeft, offsetRight);
            me->m_bmpScroll = GetScroll(m_bmpScrollMap, ARRAY_COUNT(m_bmpScrollMap), offsetLeft, offsetRight);
            FW_ASSERT(me->m_bmpScroll.m_timeoutMs && me->m_bmpScroll.m_delta);
            me->m_bmpTimer.Start(me->m_bmpScroll.m_timeoutMs);
            WM_Exec();
            return Q_HANDLED();
        }
        case TEXT1_TIMER: {
            EVENT(e);
            uint32_t bufIdx;
            uint32_t offsetLeft;
            uint32_t offsetRight;
            me->m_text1.Update(me->m_text1Scroll.m_delta, bufIdx, offsetLeft, offsetRight);
            me->m_text1Scroll = GetScroll(m_text1ScrollMap, ARRAY_COUNT(m_text1ScrollMap), offsetLeft, offsetRight);
            FW_ASSERT(me->m_text1Scroll.m_timeoutMs && me->m_text1Scroll.m_delta);
            me->m_text1Timer.Start(me->m_text1Scroll.m_timeoutMs);
            WM_Exec();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GuiMgr::Started);
}

/*
QState GuiMgr::MyState(GuiMgr * const me, QEvt const * const e) {
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
            return Q_TRAN(&GuiMgr::SubState);
        }
    }
    return Q_SUPER(&GuiMgr::SuperState);
}
*/

} // namespace APP
