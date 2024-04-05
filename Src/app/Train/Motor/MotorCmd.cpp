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
#include "MotorCmd.h"
#include "MotorInterface.h"

FW_DEFINE_THIS_FILE("MotorCmd.cpp")

namespace APP {

static CmdStatus Test(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("A simple active object. Add your own commands here.\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Start(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.SendReq(new MotorStartReq(), MOTOR, true, console.GetEvtSeq());
            break;
        }
        case MOTOR_START_CFM: {
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
            console.SendReq(new MotorStopReq(), MOTOR, true, console.GetEvtSeq());
            break;
        }
        case MOTOR_STOP_CFM: {
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

// Used for both "f" (forward) and "b" (backward) commands.
static CmdStatus Run(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            auto const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() >= 2) {
                MotorDir dir = STRING_EQUAL(cmd.Argv(0), "b") ? MotorDir::BACKWARD : MotorDir::FORWARD;
                uint32_t level = STRING_TO_NUM(cmd.Argv(1), 0);
                uint32_t accel = 100;
                uint32_t decel = 100;
                if (cmd.Argc() >=3) {
                    accel = STRING_TO_NUM(cmd.Argv(2), 0);
                }
                if (cmd.Argc() >=4) {
                    decel = STRING_TO_NUM(cmd.Argv(3), 0);
                }
                console.Print("level(%c) = %d, accel = %d, decel = %d\n\r",
                              (dir == MotorDir::FORWARD) ? 'f' : 'b', level, accel, decel);
                console.SendReqMsg(new MotorRunReq(dir, level, accel, decel), MOTOR, MSG_UNDEF, true, console.GetMsgSeq());
                break;
            }
            console.Print("motor f|b <level (0-1000)> <accel level/s> <decel level/s>\n\r");
            console.Print("'f' - forward    'b' - backward");
            return CMD_DONE;
        }
        case MOTOR_RUN_CFM: {
            auto const &cfm = static_cast<MotorRunCfm const &>(*e);
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
    { "start",      Start,      "Start the Motor HSM", 0 },
    { "stop",       Stop,       "Stop the Motor HSM", 0 },
    { "f",          Run,        "Run motor (forward)", 0 },
    { "b",          Run,        "Run motor (backward)", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus MotorCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
