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
extern "C" const GUI_BITMAP bmRealtimeDesign;
extern "C" const GUI_BITMAP bmhelloworld;
extern "C" const GUI_BITMAP bmb_0001;
extern "C" const GUI_BITMAP bmb_0002;
extern "C" const GUI_BITMAP bmb_0003;
extern "C" const GUI_BITMAP bmb_0004;
extern "C" const GUI_BITMAP bmb_0005;
extern "C" const GUI_BITMAP bmb_0006;
extern "C" const GUI_BITMAP bmb_0007;
extern "C" const GUI_BITMAP bmb_0008;
extern "C" const GUI_BITMAP bmColordots;
extern "C" const GUI_BITMAP bmLedpanel6;
// Gallium logos for train sign.
extern "C" const GUI_BITMAP bmLogo_90x90;
extern "C" const GUI_BITMAP bmStem_90x90;

static const GUI_BITMAP *guiBitmap[] =
{
    &bmRealtimeDesign,
    &bmhelloworld,
    &bmb_0001,
    &bmb_0002,
    &bmb_0003,
    &bmb_0004,
    &bmb_0005,
    &bmb_0006,
    &bmb_0007,
    &bmb_0008,
    &bmColordots,
    &bmLedpanel6,
};

static const GUI_BITMAP *logoBitmap[] =
{
    &bmLogo_90x90,
    &bmStem_90x90,
};


const GuiMgr::TimeoutMap GuiMgr::m_ticker1Timeout[] = {
    { 4,  40   },
    { 3,  60   },
    { 2,  90   },
    { 1,  120  },
    { 0,  1500 }
};

const GuiMgr::TimeoutMap GuiMgr::m_ticker2Timeout[] = {
    { 4,  10   },
    { 3,  20  },
    { 2,  40  },
    { 1,  80  },
    { 0,  3000 }
};

const GuiMgr::TimeoutMap GuiMgr::m_ticker3Timeout[] = {
    { 4,  20   },
    { 3,  40   },
    { 2,  80  },
    { 1,  120  },
    { 0,  3000 }
};

const GuiMgr::TimeoutMap GuiMgr::m_ticker4Timeout[] = {
    { 4,  60   },
    { 3,  100  },
    { 2,  140  },
    { 1,  180  },
    { 0,  3000 }
};

const GuiMgr::TimeoutMap GuiMgr::m_text1Timeout[] = {
    //{ 10, 30   },
    //{ 6,  30   },
    { 4,  50   },
    { 3,  80   },
    { 2,  120  },
    { 1,  160  },
    { 0,  5000 }
};

const GuiMgr::TimeoutMap GuiMgr::m_bmpTimeout[] = {
    { 10, 50   },
    { 8,  60   },
    { 6,  80   },
    { 4,  100  },
    { 2,  150  },
    { 1,  200  },
    { 0,  5000 }    // was 5000
};

// Helper function to map offset to timeout for speed control.
uint32_t GuiMgr::GetTimeout(TimeoutMap const *map, uint32_t mapLen, uint32_t offsetLeft, uint32_t offsetRight) {
    uint32_t timeoutLeft  = GetTimeout(map, mapLen, offsetLeft);
    uint32_t timeoutRight = GetTimeout(map, mapLen, offsetRight);
    return GREATER(timeoutLeft, timeoutRight);
}
uint32_t GuiMgr::GetTimeout(TimeoutMap const *map, uint32_t mapLen, uint32_t offset) {
    uint32_t i;
    FW_ASSERT(map);
    for (i=0; i<mapLen; i++) {
        if (offset >= map[i].offset) {
            return map[i].timeoutMs;
        }
    }
    FW_ASSERT(0);
    return 0;
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
    m_stateTimer(GetHsmn(), STATE_TIMER), m_syncTimer(GetHsmn(), SYNC_TIMER),
    m_bgWndTimer(GetHsmn(), BG_WND_TIMER),
    m_ticker1Timer(GetHsmn(), TICKER1_TIMER),
    m_ticker2Timer(GetHsmn(), TICKER2_TIMER),
    m_ticker3Timer(GetHsmn(), TICKER3_TIMER),
    m_ticker4Timer(GetHsmn(), TICKER4_TIMER),
    m_text1Timer(GetHsmn(), TEXT1_TIMER),
    m_bmpTimer(GetHsmn(), BMP_TIMER),
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
            //return Q_TRAN(&GuiMgr::Ticker);
            //return Q_TRAN(&GuiMgr::Signage);
            //return Q_TRAN(&GuiMgr::Bmp);
            //return Q_TRAN(&GuiMgr::ColorTest);
            //return Q_HANDLED();
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
            me->AddWin(me->m_bmp.Create(me, me->m_bgWnd.GetHandle(), 0, 0, 90, 90, &WmCallback), PAINT_BMP);
            uint32_t bmpCnt = ARRAY_COUNT(logoBitmap);
            for (uint32_t i=0; i<bmpCnt && i<GuiBmp::IMG_CNT; i++) {
                me->m_bmp.SetBitmap(i, logoBitmap[i]);
            }
            me->m_bmpTimer.Start(GetTimeout(m_bmpTimeout, ARRAY_COUNT(m_bmpTimeout), 0, 0));
            // Info bar (m_ticker1).
            me->AddWin(me->m_ticker1.Create(me, me->m_bgWnd.GetHandle(), 90, 0, 230, 45, 0xff0000, &WmCallback), PAINT_TICKER1);
            me->m_ticker1.SetText(0, "Gallium IO", GUI_FONT_32B_ASCII, 0x7f7f7f, 0xff0000, GUI_TA_HCENTER|GUI_TA_VCENTER);
            // Route number (m_ticker2).
            me->AddWin(me->m_ticker2.Create(me, me->m_bgWnd.GetHandle(), 90, 45, 230, 45, 0xff0000, &WmCallback), PAINT_TICKER2);
            me->m_ticker2.SetText(0, "IGX 1042", GUI_FONT_32B_ASCII, 0x7f7f7f, 0xff0000, GUI_TA_HCENTER|GUI_TA_VCENTER);
            // Origin station (m_ticker3).
            me->AddWin(me->m_ticker3.Create(me, me->m_bgWnd.GetHandle(), 0, 90, 320, 50, 0x3f0000, &WmCallback), PAINT_TICKER3);
            me->m_ticker3.SetText(0, "Bellevue, WA =>", GUI_FONT_32B_1, 0x7f7f7f, 0x3f0000, GUI_TA_LEFT|GUI_TA_VCENTER);
            // Destination station (m_ticker4).
            me->AddWin(me->m_ticker4.Create(me, me->m_bgWnd.GetHandle(), 0, 140, 320, 50, 0x3f0000, &WmCallback), PAINT_TICKER4);
            me->m_ticker4.SetText(0, "=> San Jose, CA", GUI_FONT_32B_1, 0x7f7f7f, 0x3f0000, GUI_TA_RIGHT|GUI_TA_VCENTER);
            // "via" (m_ticker5)
            me->AddWin(me->m_ticker5.Create(me, me->m_bgWnd.GetHandle(), 0, 190, 50, 50, 0x1f0000, &WmCallback), PAINT_TICKER5);
            me->m_ticker5.SetText(0, "via", GUI_FONT_24_ASCII, 0x7f7f7f, 0x1f0000, GUI_TA_HCENTER|GUI_TA_VCENTER);
            // Stop stations (m_text1).
            me->AddWin(me->m_text1.Create(me, me->m_bgWnd.GetHandle(), 50, 190, 270, 50, 0x1f0000, &WmCallback), PAINT_TEXT1);
            me->m_text1.SetText(0, "Portland, OR\nSan Francisco, CA\nPalo Alto, CA",
                                GUI_FONT_24B_1, 0x7f7f7f, 0x1f0000);
            me->m_text1Timer.Start(GetTimeout(m_text1Timeout, ARRAY_COUNT(m_text1Timeout), 0, 0));
            WM_Exec();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->RemoveWin(me->m_bmp.Destroy());
            me->RemoveWin(me->m_ticker1.Destroy());
            me->RemoveWin(me->m_ticker2.Destroy());
            me->RemoveWin(me->m_ticker3.Destroy());
            me->RemoveWin(me->m_ticker4.Destroy());
            me->RemoveWin(me->m_ticker5.Destroy());
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
        case PAINT_TICKER4: {
            EVENT(e);
            me->SetDirty(me->m_ticker4.Paint());
            return Q_HANDLED();
        }
        case PAINT_TICKER5: {
            EVENT(e);
            me->SetDirty(me->m_ticker5.Paint());
            return Q_HANDLED();
        }
        case PAINT_TEXT1: {
            EVENT(e);
            me->SetDirty(me->m_text1.Paint());
            return Q_HANDLED();
        }
        case BMP_TIMER: {
            EVENT(e);
            uint32_t imgIdx = 0;
            uint32_t offsetLeft = 0;
            uint32_t offsetRight = 0;
            me->m_bmp.Update(-1, imgIdx, offsetLeft, offsetRight);
            me->m_bmpTimer.Start(GetTimeout(m_bmpTimeout, ARRAY_COUNT(m_bmpTimeout), offsetLeft, offsetRight));
            WM_Exec();
            return Q_HANDLED();
        }
        case TEXT1_TIMER: {
            EVENT(e);
            uint32_t bufIdx;
            uint32_t offsetLeft;
            uint32_t offsetRight;
            me->m_text1.Update(-1, bufIdx, offsetLeft, offsetRight);
            me->m_text1Timer.Restart(GetTimeout(m_text1Timeout, ARRAY_COUNT(m_text1Timeout), offsetLeft, offsetRight));
            WM_Exec();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GuiMgr::Started);
}

QState GuiMgr::ColorTest(GuiMgr * const me, QEvt const * const e) {
    static const uint32_t TEST_COLOR[] = {
            0x0000ff,   // red  (0:0:1)
            0x007fff,   // orange (0:1:2)
            0x00ffff,   // yellow (0:1:1)
            0x7f00bf,   // pinkish purple (2:0:3)
            0xff7f7f,   // lavendar (4:3:3)
            0x3f7fbf,   // light brown (1:2:3)
            0xffdf00,   // turqoise (tiffany blue)
            0x7fbf00,   // lake green
    };
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            //me->m_bgWnd.SetColor(0x0000ff, 0x0000ff);
            me->m_bgWnd.SetColor(0x00ff00, 0x00ff00);
            me->m_updateIdx = 1;
            WM_Exec();
            // Commented for test.
            me->m_updateTimer.Start(COLOR_TEST_TIMEOUT_MS, Timer::PERIODIC);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->RemoveWin(me->m_text1.Destroy());
            return Q_HANDLED();
        }
        case UPDATE_TIMER: {
            EVENT(e);
            FW_ASSERT(me->m_updateIdx < ARRAY_COUNT(TEST_COLOR));
            uint32_t color = TEST_COLOR[me->m_updateIdx];
            me->m_bgWnd.SetColor(color, color);
            WM_Exec();
            me->m_updateIdx++;
            if (me->m_updateIdx >= ARRAY_COUNT(TEST_COLOR)) {
                me->m_updateIdx = 0;
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GuiMgr::Started);
}

QState GuiMgr::Signage(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            WM_HWIN handle;
            handle = me->m_text1.Create(me, me->m_bgWnd.GetHandle(), 3, 3, 122, 78, 0x200000, &WmCallback);
            me->AddWin(handle, PAINT_TEXT1);
            me->m_text1.SetText  (0, "  Real-time Design with Statecharts",
                                  &GUI_Font24B_ASCII, 0xffffff, 0x200000);
            handle = me->m_ticker1.Create(me, me->m_bgWnd.GetHandle(), 3, 81, 122, 20, 0x000020, &WmCallback);
            me->AddWin(handle, PAINT_TICKER1);
            me->m_ticker1.SetText(0, " Robust",
                                  GUI_FONT_20B_ASCII, 0xffffff, 0x000020);
            handle = me->m_ticker2.Create(me, me->m_bgWnd.GetHandle(), 3, 101, 122, 24, 0x000020, &WmCallback);
            me->AddWin(handle, PAINT_TICKER2);
            me->m_ticker2.SetText(0, "            Agile",
                                  GUI_FONT_20B_ASCII, 0xffffff, 0x000020);
            WM_Exec();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->RemoveWin(me->m_text1.Destroy());
            me->RemoveWin(me->m_ticker1.Destroy());
            me->RemoveWin(me->m_ticker2.Destroy());
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
        case PAINT_TEXT1: {
            EVENT(e);
            me->SetDirty(me->m_text1.Paint());
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GuiMgr::Started);
}

QState GuiMgr::Ticker(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            WM_HWIN handle;
            // Test only.
            me->m_bgWnd.SetColor(0x3f3f3f, 0x3f3f3f);
            handle = me->m_ticker1.Create(me, me->m_bgWnd.GetHandle(), 100, 100, 128, 22, 0x1f1f1f, &WmCallback);
            me->AddWin(handle, PAINT_TICKER1);
            me->m_ticker1.SetText(0, "Gallium Studio        ",
                                  GUI_FONT_20B_ASCII, 0x7f7f7f, 0x0f0000);
            me->m_ticker1.SetText(1, "Promoting STEM with Design     ",
                                  GUI_FONT_20B_ASCII, 0x7f7f7f, 0x00000f);
            me->m_ticker1.SetText(2, "Visit www.galliumstudio.com       ",
                                  GUI_FONT_20B_ASCII, 0x7f7f7f, 0x000f00);
            me->m_ticker1Timer.Start(GetTimeout(m_ticker1Timeout, ARRAY_COUNT(m_ticker1Timeout), 0, 0));

            handle = me->m_text1.Create(me, me->m_bgWnd.GetHandle(), 100, 122, 128, 106, 0x1f1f1f, &WmCallback);
            me->AddWin(handle, PAINT_TEXT1);
            me->m_text1.SetText  (0, "Real-time Design with Statecharts",
                                  GUI_FONT_16B_ASCII, 0x003d66, 0x400000);
            me->m_text1.SetText  (1, "Robust and reliable",
                                  GUI_FONT_20_ASCII, 0x007fff, 0x330800);
            me->m_text1.SetText  (2, "C++, Javascript, Python and more...",
                                  GUI_FONT_24_ASCII, 0xc0c0c0, 0x001911);
            me->m_text1Timer.Start(GetTimeout(m_text1Timeout, ARRAY_COUNT(m_text1Timeout), 0, 0));
            WM_Exec();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->RemoveWin(me->m_ticker1.Destroy());
            me->m_ticker1Timer.Stop();
            me->RemoveWin(me->m_text1.Destroy());
            me->m_text1Timer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&GuiMgr::TickerNormal);
        }
        case GUI_MGR_TICKER_REQ: {
            EVENT(e);
            auto const &req = static_cast<GuiMgrTickerReq const &>(*e);
            // Indices 0-2 are for ticker1. Indices 3-5 are for text1.
            if (req.GetIndex() < 3) {
                me->m_ticker1.SetText(req.GetIndex(), req.GetText(), GUI_FONT_20B_ASCII, req.GetFgColor(), req.GetBgColor());
            } else if (req.GetIndex() < 6) {
                me->m_text1.SetText(req.GetIndex() - 3, req.GetText(), GUI_FONT_13B_ASCII, req.GetFgColor(), req.GetBgColor());
            }
            me->SendCfmMsg(new GuiMgrTickerCfm(MSG_ERROR_SUCCESS), req);
            WM_Exec();
            return Q_HANDLED();
        }
        case PAINT_TICKER1: {
            EVENT(e);
            me->SetDirty(me->m_ticker1.Paint());
            return Q_HANDLED();
        }
        case PAINT_TEXT1: {
            EVENT(e);
            me->SetDirty(me->m_text1.Paint());
            return Q_HANDLED();
        }
        case TICKER1_TIMER: {
            EVENT(e);
            uint32_t bufIdx;
            uint32_t offsetLeft;
            uint32_t offsetRight;
            me->m_ticker1.Update(-1, bufIdx, offsetLeft, offsetRight);
            me->m_ticker1Timer.Restart(GetTimeout(m_ticker1Timeout, ARRAY_COUNT(m_ticker1Timeout), offsetLeft, offsetRight));
            WM_Exec();
            return Q_HANDLED();
        }
        case TEXT1_TIMER: {
            EVENT(e);
            uint32_t bufIdx;
            uint32_t offsetLeft;
            uint32_t offsetRight;
            me->m_text1.Update(-1, bufIdx, offsetLeft, offsetRight);
            me->m_text1Timer.Restart(GetTimeout(m_text1Timeout, ARRAY_COUNT(m_text1Timeout), offsetLeft, offsetRight));
            WM_Exec();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GuiMgr::Started);
}

QState GuiMgr::TickerNormal(GuiMgr * const me, QEvt const * const e) {
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
    return Q_SUPER(&GuiMgr::Ticker);
}

QState GuiMgr::TickerAlert(GuiMgr * const me, QEvt const * const e) {
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
    return Q_SUPER(&GuiMgr::Ticker);
}


QState GuiMgr::Bmp(GuiMgr * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            WM_HWIN handle = me->m_bmp.Create(me, me->m_bgWnd.GetHandle(), 0, 0, 128, 128, &WmCallback);
            LOG("bmp win handle = %d", handle);
            me->AddWin(handle, PAINT_BMP);
            uint32_t bmpCnt = ARRAY_COUNT(guiBitmap);
            for (uint32_t i=0; i<bmpCnt && i<GuiBmp::IMG_CNT; i++) {
                me->m_bmp.SetBitmap(i, guiBitmap[i]);
            }
            me->m_bmpTimer.Start(GetTimeout(m_bmpTimeout, ARRAY_COUNT(m_bmpTimeout), 0, 0));
            me->m_bmpCycleCnt = 0;
            WM_Exec();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->RemoveWin(me->m_bmp.Destroy());
            me->m_bmpTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&GuiMgr::BmpNormal);
        }
        case PAINT_BMP: {
            EVENT(e);
            me->SetDirty(me->m_bmp.Paint());
            return Q_HANDLED();
        }
        case BMP_TIMER: {
            EVENT(e);
            uint32_t imgIdx = 0;
            uint32_t offsetLeft = 0;
            uint32_t offsetRight = 0;
            me->m_bmp.Update(-1, imgIdx, offsetLeft, offsetRight);
            me->m_bmpTimer.Start(GetTimeout(m_bmpTimeout, ARRAY_COUNT(m_bmpTimeout), offsetLeft, offsetRight));
            WM_Exec();
            INFO("%d, %d (%d)\n\r", offsetLeft, offsetRight, me->m_bmpCycleCnt);
            if (offsetLeft == 1) {
                me->m_bmpCycleCnt++;
                if (me->m_bmpCycleCnt >= ARRAY_COUNT(guiBitmap)) {
                    // Commented to stay in bmp state.
                    //return Q_TRAN(&GuiMgr::Ticker);
                }
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GuiMgr::Started);
}

QState GuiMgr::BmpNormal(GuiMgr * const me, QEvt const * const e) {
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
    return Q_SUPER(&GuiMgr::Bmp);
}

QState GuiMgr::BmpAlert(GuiMgr * const me, QEvt const * const e) {
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
    return Q_SUPER(&GuiMgr::Bmp);
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
