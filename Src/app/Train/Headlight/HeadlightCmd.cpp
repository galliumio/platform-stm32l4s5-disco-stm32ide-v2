/*******************************************************************************
 * Copyright (C) Gallium Studio LLC. All rights reserved.
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
#include "HeadlightCmd.h"
#include "HeadlightInterface.h"

FW_DEFINE_THIS_FILE("HeadlightCmd.cpp")

namespace APP {

static CmdStatus Start(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.SendReq(new HeadlightStartReq(), HEADLIGHT, true, console.GetEvtSeq());
            break;
        }
        case HEADLIGHT_START_CFM: {
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
            console.SendReq(new HeadlightStopReq(), HEADLIGHT, true, console.GetEvtSeq());
            break;
        }
        case HEADLIGHT_STOP_CFM: {
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

static CmdStatus Set(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            auto const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() >= 2) {
                uint32_t color = STRING_TO_NUM(ind.Argv(1), 16);
                uint32_t rampMs = 1000;
                if (ind.Argc() >=3) {
                    rampMs = STRING_TO_NUM(ind.Argv(2), 0);
                }
                console.Print("color = 0x%x, rampMs = %d\n\r", color, rampMs);
                console.SendReqMsg(new HeadlightSetReq(color, rampMs), HEADLIGHT, MSG_UNDEF, true, console.GetMsgSeq());
                break;
            }
            console.Print("hl set <hex color (888 xBGR)> <ramp/ms>\n\r");
            return CMD_DONE;
        }
        case HEADLIGHT_SET_CFM: {
            auto const &cfm = static_cast<HeadlightSetCfm const &>(*e);
            console.PrintErrorMsg(cfm);
            bool allReceived;
            if (!console.CheckCfmMsg(cfm, allReceived, console.GetMsgSeq())) {
                console.Print("Done with error\n\r");
                return CMD_DONE;
            } else if (allReceived) {
                console.Print("Done with success\n\r");
                return CMD_DONE;
            }
            break;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus Pattern(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            auto const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() >= 2) {
                uint32_t pattern = STRING_TO_NUM(ind.Argv(1), 0);
                bool repeat = true;
                if (ind.Argc() >= 3 && STRING_EQUAL(ind.Argv(2), "0")) {
                    repeat = false;
                }
                console.Print("pattern = %d, repeat = %d\n\r", pattern, repeat);
                console.SendReqMsg(new HeadlightPatternReq(pattern, repeat), HEADLIGHT, MSG_UNDEF, true, console.GetMsgSeq());
                break;
            }
            console.Print("hl pattern <pattern idx> [0=once,*other=repeat]\n\r");
            return CMD_DONE;
        }
        case HEADLIGHT_PATTERN_CFM: {
            auto const &cfm = static_cast<HeadlightPatternCfm const &>(*e);
            console.PrintErrorMsg(cfm);
            bool allReceived;
            if (!console.CheckCfmMsg(cfm, allReceived, console.GetMsgSeq())) {
                console.Print("Done with error\n\r");
                return CMD_DONE;
            } else if (allReceived) {
                console.Print("Done with success\n\r");
                return CMD_DONE;
            }
            break;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "?",          List,       "List commands", 0 },
    { "start",      Start,      "Start the Headlight HSM", 0 },
    { "stop",       Stop,       "Stop the Headlight HSM", 0 },
    { "set",        Set,        "Set headlight color", 0 },
    { "pattern",    Pattern,    "Set headlight pattern", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus HeadlightCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
