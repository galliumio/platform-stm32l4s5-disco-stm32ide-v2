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

#ifndef MOTOR_INTERFACE_H
#define MOTOR_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "MotorMsgInterface.h"

#define MOTOR_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("MotorInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define MOTOR_INTERFACE_EVT \
    ADD_EVT(MOTOR_START_REQ) \
    ADD_EVT(MOTOR_START_CFM) \
    ADD_EVT(MOTOR_STOP_REQ) \
    ADD_EVT(MOTOR_STOP_CFM) \
    ADD_EVT(MOTOR_RUN_REQ) \
    ADD_EVT(MOTOR_RUN_CFM)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    MOTOR_INTERFACE_EVT_START = INTERFACE_EVT_START(MOTOR),
    MOTOR_INTERFACE_EVT
};

enum {
    MOTOR_REASON_UNSPEC = 0,
};

class MotorStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200
    };
    MotorStartReq() :
        Evt(MOTOR_START_REQ) {}
};

class MotorStartCfm : public ErrorEvt {
public:
    MotorStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(MOTOR_START_CFM, error, origin, reason) {}
};

class MotorStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 3000
    };
    MotorStopReq() :
        Evt(MOTOR_STOP_REQ) {}
};

class MotorStopCfm : public ErrorEvt {
public:
    MotorStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(MOTOR_STOP_CFM, error, origin, reason) {}
};

class MotorRunReq : public MsgEvt {
public:
    enum {
        TIMEOUT_MS = 60000
    };
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    MotorRunReq(MotorRunReqMsg const &r) :
        MsgEvt(MOTOR_RUN_REQ, m_msg), m_msg(r) {
        MOTOR_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    MotorDir GetDir() const { return m_msg.GetDir(); }
    uint32_t GetSpeed() const { return m_msg.GetSpeed(); }
    uint32_t GetAccel() const { return m_msg.GetAccel(); }
    uint32_t GetDecel() const { return m_msg.GetDecel(); }
protected:
    MotorRunReqMsg m_msg;
};

class MotorRunCfm : public ErrorMsgEvt {
public:
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    MotorRunCfm(MotorRunCfmMsg const &r) :
        ErrorMsgEvt(MOTOR_RUN_CFM, m_msg), m_msg(r) {
        MOTOR_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
protected:
    MotorRunCfmMsg m_msg;
};

} // namespace APP

#endif // MOTOR_INTERFACE_H
