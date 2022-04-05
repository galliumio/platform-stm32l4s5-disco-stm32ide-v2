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

#ifndef MOTOR_MSG_INTERFACE_H
#define MOTOR_MSG_INTERFACE_H

#include "fw_def.h"
#include "fw_msg.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

// This file defines messages for the "Motor" role.

enum MotorDir : uint8_t {
    FORWARD,
    BACKWARD,
    INVALID,
};

class MotorRunReqMsg final: public Msg {
public:
    MotorRunReqMsg(MotorDir dir = MotorDir::INVALID, uint16_t speed = 0, uint16_t accel = 0, uint16_t decel = 0) :
        Msg("MotorRunReqMsg"), m_speed(speed), m_accel(accel), m_decel(decel),  m_dir(dir) {
        m_len = sizeof(*this);
    }
    MotorDir GetDir() const {
        if (m_dir < MotorDir::INVALID) {
            return static_cast<MotorDir>(m_dir);
        }
        return MotorDir::INVALID;
    }
    uint16_t GetSpeed() const { return m_speed; }
    uint16_t GetAccel() const { return m_accel; }
    uint16_t GetDecel() const { return m_decel; }
private:
    uint16_t m_speed;       // PWM duty-cycle permil level (0-1000).
    uint16_t m_accel;       // Acceleration in PWM permil steps per second. 1000 means 0 to full speed in 1s.
    uint16_t m_decel;       // Deceleration in PWM perml steps per second. 1000 means full speed to stop in 1s.
    uint8_t m_dir;
} __attribute__((packed));

class MotorRunCfmMsg final : public ErrorMsg {
public:
    MotorRunCfmMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("MotorRunCfmMsg", error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));


} // namespace APP

#endif // MOTOR_MSG_INTERFACE_H
