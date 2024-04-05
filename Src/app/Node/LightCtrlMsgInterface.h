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

#ifndef LIGHT_CTRL_MSG_INTERFACE_H
#define LIGHT_CTRL_MSG_INTERFACE_H

#include "fw_def.h"
#include "fw_msg.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

// This file defines messages for the "LightCtrl (Light Control)" role.
// LightCtrl is responsible for the overall lighting control of a device, which may involve a group of
// individual lights (each managed by a related "Light" role).

#define LIGHT_CTRL_MSG_REASON_INVALID_CMD   "INVALID_CMD"       // Invalid light control command.

// Light control operations.
enum class LightCtrlOp: uint8_t {
    ALL_OFF,    // All lights off.
    ALL_FADE,   // All lights fades out until off.
    // Headlight (front and rear) commands.
    HEADLIGHT_REST_FORWARD,     // Motor stopped and ready to run in forward direction.
    HEADLIGHT_REST_BACKWARD,    // Motor stopped and ready to run in backward direction.
    HEADLIGHT_RUNNING_FORWARD,  // Motor running in 'forward' direction.
    HEADLIGHT_RUNNING_BACKWARD, // Motor running in 'backward' direction.
    HEADLIGHT_ALERT,            // Headlight showing red-blue flashing pattern.
    HEADLIGHT_WARNING,          // Headlight showing amber-white flashing pattern.
    // @todo Add cabin light operations.
    INVALID
};

// A request to send a command to LightCtrl.
class LightCtrlOpReqMsg final: public Msg {
public:
    LightCtrlOpReqMsg(LightCtrlOp op) :
        Msg("LightCtrlOpReqMsg"), m_op(static_cast<uint8_t>(op)) {
        m_len = sizeof(*this);
    }
    LightCtrlOp GetOp() const {
        if (m_op < static_cast<uint8_t>(LightCtrlOp::INVALID)) {
            return static_cast<LightCtrlOp>(m_op);
        }
        return LightCtrlOp::INVALID;
    }
private:
    uint8_t m_op;
} __attribute__((packed));

class LightCtrlOpCfmMsg final : public ErrorMsg {
public:
    LightCtrlOpCfmMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("LightCtrlOpCfmMsg", error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));


} // namespace APP

#endif // LIGHT_CTRL_MSG_INTERFACE_H
