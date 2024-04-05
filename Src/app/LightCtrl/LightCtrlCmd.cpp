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
#include "LightCtrlCmd.h"
#include "LightCtrlInterface.h"

FW_DEFINE_THIS_FILE("LightCtrlCmd.cpp")

namespace APP {

static CmdStatus Test(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("A composite active object. Add your own commands here.\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Start(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.SendReq(new LightCtrlStartReq(), LIGHT_CTRL, true, console.GetEvtSeq());
            break;
        }
        case LIGHT_CTRL_START_CFM: {
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
            console.SendReq(new LightCtrlStopReq(), LIGHT_CTRL, true, console.GetEvtSeq());
            break;
        }
        case LIGHT_CTRL_STOP_CFM: {
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

static CmdStatus Op(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            auto const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() >= 2) {
                uint32_t op = STRING_TO_NUM(cmd.Argv(1), 0);
                if (op >= static_cast<uint32_t>(LightCtrlOp::INVALID)) {
                    console.Print("Invalid light control cmd %d\n\r", op);
                    return CMD_DONE;
                }
                console.SendReqMsg(new LightCtrlOpReq(static_cast<LightCtrlOp>(op)), LIGHT_CTRL, MSG_UNDEF, true, console.GetMsgSeq());
                break;
            }
            console.Print("lightCtrl op <operation>\n\r");
            return CMD_DONE;
        }
        case LIGHT_CTRL_OP_CFM: {
            auto const &cfm = static_cast<LightCtrlOpCfm const &>(*e);
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
    { "test",       Test,       "Test command", 0 },
    { "?",          List,       "List commands", 0 },
    { "start",      Start,      "Start the LightCtrl HSM", 0 },
    { "stop",       Stop,       "Stop the LightCtrl HSM", 0 },
    { "op",         Op,         "Perform a light control operation", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus LightCtrlCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
