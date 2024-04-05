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

#include <string.h>
#include "fw_log.h"
#include "fw_assert.h"
#include "Console.h"
#include "Colors.h"
#include "Ws2812Cmd.h"
#include "Ws2812Interface.h"

FW_DEFINE_THIS_FILE("Ws2812Cmd.cpp")

namespace APP {



static CmdStatus Start(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            uint32_t ledCount = 8;
            auto const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() >= 2) {
                ledCount = STRING_TO_NUM(ind.Argv(1), 0);
            }
            console.Print("ledCount = %d\n\r", ledCount);
            console.SendReq(new Ws2812StartReq(ledCount), WS2812, true, console.GetEvtSeq());
            break;
        }
        case WS2812_START_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            console.PrintErrorEvt(cfm);
            bool allReceived;
            if (!console.CheckCfm(cfm, allReceived, console.GetEvtSeq())) {
                console.Print("Done with error\n\r");
                return CMD_DONE;
            } else if (allReceived) {
                console.Print("Done with success\n\r");
                return CMD_DONE;
            }
            console.Print("Continue to wait for matching cfm...\n\r");
            break;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus Stop(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.SendReq(new Ws2812StopReq(), WS2812, true, console.GetEvtSeq());
            break;
        }
        case WS2812_STOP_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            console.PrintErrorEvt(cfm);
            bool allReceived;
            if (!console.CheckCfm(cfm, allReceived, console.GetEvtSeq())) {
                console.Print("Done with error\n\r");
                return CMD_DONE;
            } else if (allReceived) {
                console.Print("Done with success\n\r");
                return CMD_DONE;
            }
            console.Print("Continue to wait for matching cfm...\n\r");
            break;
        }
    }
    return CMD_CONTINUE;
}

static Ws2812SetReq::LedColor sColorBuf[Ws2812SetReq::MAX_LED_COUNT];
static bool FillColor(Console &console, Color888Bgr color, uint32_t count) {
    if (count > ARRAY_COUNT(sColorBuf)) {
        console.Print("FillColor count (%d) too large\n\r", count);
        return false;
    }
    console.Print("FillColor color = 0x%06x, count = %d\n\r", color.Get(), count);
    for (uint32_t i = 0; i < count; i++) {
        sColorBuf[i].Set(i, color.Gamma());
    }
    return true;
}

static CmdStatus BusyStop(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            uint32_t ledCount = 8;
            auto const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() >= 2) {
                ledCount = STRING_TO_NUM(ind.Argv(1), 0);
            }
            if (!FillColor(console, Color888Bgr(0x00ffff), ledCount)) {
                return CMD_DONE;
            }
            console.SendReq(new Ws2812SetReq(sColorBuf, ledCount), WS2812, true, console.GetEvtSeq());
            console.SendReq(new Ws2812StopReq(), WS2812, true, console.GetEvtSeq());
            break;
        }
        case WS2812_STOP_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            console.PrintErrorEvt(cfm);
            bool allReceived;
            if (!console.CheckCfm(cfm, allReceived, console.GetEvtSeq())) {
                console.Print("Done with error\n\r");
                return CMD_DONE;
            } else if (allReceived) {
                console.Print("Done with success\n\r");
                return CMD_DONE;
            }
            console.Print("Continue to wait for matching cfm...\n\r");
            break;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus BusySet(Console &console, Evt const *e) {
    constexpr uint32_t MAX_SET_COUNT = 10;
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            uint32_t ledCount = 8;
            auto const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() >= 2) {
                ledCount = STRING_TO_NUM(ind.Argv(1), 0);
            }
            if (!FillColor(console, Color888Bgr(0x00ffff), ledCount)) {
                return CMD_DONE;
            }
            console.SendReq(new Ws2812SetReq(sColorBuf, ledCount), WS2812, true, console.GetEvtSeq());
            console.Var(0) = ledCount;
            console.Var(1) = 0;
            break;
        }
        case WS2812_SET_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            //console.PrintErrorEvt(cfm);
            bool allReceived;
            if (!console.CheckCfm(cfm, allReceived, console.GetEvtSeq())) {
                console.Print("[%d] Set colors failed\n\r", console.Var(1));
                return CMD_DONE;
            } else if (allReceived) {
                //console.Print("[%d] Set colors success\n\r", console.Var(1));
                if (++console.Var(1) >= MAX_SET_COUNT) {
                    return CMD_DONE;
                }
                console.SendReq(new Ws2812SetReq(sColorBuf, console.Var(0)), WS2812, true, console.GetEvtSeq());
                break;
            }
            console.Print("Continue to wait for matching cfm...\n\r");
            break;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "?",          List,       "List commands", 0 },
    { "start",      Start,      "Start the Ws2812 HSM", 0 },
    { "stop",       Stop,       "Stop the Ws2812 HSM", 0 },
    { "busyStop",   BusyStop,   "Test stopping when busy", 0 },
    { "busySet",    BusySet,    "Test setting colors when busy", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus Ws2812Cmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
