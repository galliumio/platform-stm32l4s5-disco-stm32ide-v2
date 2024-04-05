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
#include "LightCmd.h"
#include "LightInterface.h"

FW_DEFINE_THIS_FILE("LightCmd.cpp")

namespace APP {

static CmdStatus Start(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            auto const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() >= 2) {
                uint32_t instance = STRING_TO_NUM(cmd.Argv(1), 0);
                if (instance >= LIGHT_COUNT) {
                    console.Print("Invalid instance %d", instance);
                    return CMD_DONE;
                }
                console.SendReq(new LightStartReq(WS2812), LIGHT + instance, true, console.GetEvtSeq());
                break;
            }
            console.Print("light start <instance>\n\r");
            return CMD_DONE;
        }
        case LIGHT_START_CFM: {
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
            auto const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() >= 2) {
                uint32_t instance = STRING_TO_NUM(cmd.Argv(1), 0);
                if (instance >= LIGHT_COUNT) {
                    console.Print("Invalid instance %d", instance);
                    return CMD_DONE;
                }
                console.SendReq(new LightStopReq, LIGHT + instance, true, console.GetEvtSeq());
                break;
            }
            console.Print("light stop <instance>\n\r");
            return CMD_DONE;
        }
        case LIGHT_STOP_CFM: {
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
            auto const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() >= 3) {
                uint32_t instance = STRING_TO_NUM(cmd.Argv(1), 0);
                if (instance >= LIGHT_COUNT) {
                    console.Print("Invalid instance %d", instance);
                    return CMD_DONE;
                }
                uint32_t color = STRING_TO_NUM(cmd.Argv(2), 16);
                uint32_t rampMs = 1000;
                if (cmd.Argc() >=4) {
                    rampMs = STRING_TO_NUM(cmd.Argv(3), 0);
                }
                console.Print("instance = %d, color = 0x%x, rampMs = %d\n\r", instance, color, rampMs);
                console.SendReq(new LightSetReq(color, rampMs), LIGHT + instance, true, console.GetEvtSeq());
                break;
            }
            console.Print("light set <instance> <hex color (888 xBGR)> <ramp/ms>\n\r");
            return CMD_DONE;
        }
        case LIGHT_SET_CFM: {
            auto const &cfm = static_cast<LightSetCfm const &>(*e);
            console.PrintErrorEvt(cfm);
            bool allReceived;
            if (!console.CheckCfm(cfm, allReceived, console.GetEvtSeq())) {
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
            auto const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() >= 3) {
                uint32_t instance = STRING_TO_NUM(cmd.Argv(1), 0);
                if (instance >= LIGHT_COUNT) {
                    console.Print("Invalid instance %d", instance);
                    return CMD_DONE;
                }
                uint32_t patternIdx = STRING_TO_NUM(cmd.Argv(2), 0);
                if (patternIdx >= static_cast<uint32_t>(LightPatternIdx::INVALID)) {
                    console.Print("Invalid patternIdx %d", patternIdx);
                    return CMD_DONE;
                }
                bool repeat = true;
                if (cmd.Argc() >= 4 && STRING_EQUAL(cmd.Argv(3), "0")) {
                    repeat = false;
                }
                console.Print("instance = %d, patternIdx = %d, repeat = %d\n\r", instance, patternIdx, repeat);
                console.SendReq(new LightPatternReq(static_cast<LightPatternIdx>(patternIdx), repeat), LIGHT + instance, true, console.GetEvtSeq());
                break;
            }
            console.Print("light pattern <instance> <pattern idx> [0=once,*other=repeat]\n\r");
            return CMD_DONE;
        }
        case LIGHT_PATTERN_CFM: {
            auto const &cfm = static_cast<LightPatternCfm const &>(*e);
            console.PrintErrorEvt(cfm);
            bool allReceived;
            if (!console.CheckCfm(cfm, allReceived, console.GetEvtSeq())) {
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
    { "start",      Start,      "Start a Light HSM", 0 },
    { "stop",       Stop,       "Stop a Light HSM", 0 },
    { "set",        Set,        "Set light color", 0 },
    { "pattern",    Pattern,    "Set light pattern", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus LightCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
