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

#ifndef LIGHT_CTRL_INTERFACE_H
#define LIGHT_CTRL_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "LightCtrlMsgInterface.h"

#define LIGHT_CTRL_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("LightCtrlInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define LIGHT_CTRL_INTERFACE_EVT \
    ADD_EVT(LIGHT_CTRL_START_REQ) \
    ADD_EVT(LIGHT_CTRL_START_CFM) \
    ADD_EVT(LIGHT_CTRL_STOP_REQ) \
    ADD_EVT(LIGHT_CTRL_STOP_CFM) \
    ADD_EVT(LIGHT_CTRL_EXCEPTION_IND) \
    ADD_EVT(LIGHT_CTRL_OP_REQ) \
    ADD_EVT(LIGHT_CTRL_OP_CFM)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    LIGHT_CTRL_INTERFACE_EVT_START = INTERFACE_EVT_START(LIGHT_CTRL),
    LIGHT_CTRL_INTERFACE_EVT
};

enum {
    LIGHT_CTRL_REASON_UNSPEC = 0,
};

class LightCtrlStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    LightCtrlStartReq() :
        Evt(LIGHT_CTRL_START_REQ) {}
};

class LightCtrlStartCfm : public ErrorEvt {
public:
    LightCtrlStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_CTRL_START_CFM, error, origin, reason) {}
};

class LightCtrlStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 400
    };
    LightCtrlStopReq() :
        Evt(LIGHT_CTRL_STOP_REQ) {}
};

class LightCtrlStopCfm : public ErrorEvt {
public:
    LightCtrlStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_CTRL_STOP_CFM, error, origin, reason) {}
};

class LightCtrlExceptionInd : public ErrorEvt {
public:
    LightCtrlExceptionInd(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_CTRL_EXCEPTION_IND, error, origin, reason) {}
};

class LightCtrlOpReq : public MsgEvt {
public:
    enum {
        TIMEOUT_MS = 6000
    };
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    LightCtrlOpReq(LightCtrlOpReqMsg const &r) :
        MsgEvt(LIGHT_CTRL_OP_REQ, m_msg), m_msg(r) {
        LIGHT_CTRL_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    LightCtrlOpReq(LightCtrlOp op) :
        LightCtrlOpReq(LightCtrlOpReqMsg(op)) {}
    LightCtrlOp GetOp() const { return m_msg.GetOp(); }
private:
    LightCtrlOpReqMsg m_msg;
};

class LightCtrlOpCfm : public ErrorMsgEvt {
public:
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    LightCtrlOpCfm(LightCtrlOpCfmMsg const &r) :
        ErrorMsgEvt(LIGHT_CTRL_OP_CFM, m_msg), m_msg(r) {
        LIGHT_CTRL_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    LightCtrlOpCfm(char const *error, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        LightCtrlOpCfm(LightCtrlOpCfmMsg(error, origin, reason)) {}
protected:
    LightCtrlOpCfmMsg m_msg;
};

} // namespace APP

#endif // LIGHT_CTRL_INTERFACE_H
