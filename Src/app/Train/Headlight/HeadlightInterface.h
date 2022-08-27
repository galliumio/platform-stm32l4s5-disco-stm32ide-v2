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

#ifndef HEADLIGHT_INTERFACE_H
#define HEADLIGHT_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "HeadlightMsgInterface.h"

#define HEADLIGHT_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("HeadlightInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define HEADLIGHT_INTERFACE_EVT \
    ADD_EVT(HEADLIGHT_START_REQ) \
    ADD_EVT(HEADLIGHT_START_CFM) \
    ADD_EVT(HEADLIGHT_STOP_REQ) \
    ADD_EVT(HEADLIGHT_STOP_CFM) \
    ADD_EVT(HEADLIGHT_SET_REQ) \
    ADD_EVT(HEADLIGHT_SET_CFM) \
    ADD_EVT(HEADLIGHT_PATTERN_REQ) \
    ADD_EVT(HEADLIGHT_PATTERN_CFM) \
    ADD_EVT(HEADLIGHT_OFF_REQ) \
    ADD_EVT(HEADLIGHT_OFF_CFM)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    HEADLIGHT_INTERFACE_EVT_START = INTERFACE_EVT_START(HEADLIGHT),
    HEADLIGHT_INTERFACE_EVT
};

enum {
    HEADLIGHT_REASON_UNSPEC = 0
};

class HeadlightStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    HeadlightStartReq() :
        Evt(HEADLIGHT_START_REQ) {}
};

class HeadlightStartCfm : public ErrorEvt {
public:
    HeadlightStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(HEADLIGHT_START_CFM, error, origin, reason) {}
};

class HeadlightStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    HeadlightStopReq() :
        Evt(HEADLIGHT_STOP_REQ) {}
};

class HeadlightStopCfm : public ErrorEvt {
public:
    HeadlightStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(HEADLIGHT_STOP_CFM, error, origin, reason) {}
};

class HeadlightSetReq : public MsgEvt {
public:
    enum {
        MAX_RAMP_MS = 10000,
        TIMEOUT_MS = MAX_RAMP_MS + 100
    };
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    HeadlightSetReq(HeadlightSetReqMsg const &r) :
        MsgEvt(HEADLIGHT_SET_REQ, m_msg), m_msg(r) {
        HEADLIGHT_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    HeadlightSetReq(uint32_t color, uint32_t rampMs) :
        HeadlightSetReq(HeadlightSetReqMsg(color, rampMs)) {}
    uint32_t GetColor() const { return m_msg.GetColor(); }
    uint32_t GetRampMs() const { return m_msg.GetRampMs(); }
private:
    HeadlightSetReqMsg m_msg;
};

class HeadlightSetCfm : public ErrorMsgEvt {
public:
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    HeadlightSetCfm(HeadlightSetCfmMsg const &r) :
        ErrorMsgEvt(HEADLIGHT_SET_CFM, m_msg), m_msg(r) {
        HEADLIGHT_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    HeadlightSetCfm(char const *error, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        HeadlightSetCfm(HeadlightSetCfmMsg(error, origin, reason)) {}
protected:
    HeadlightSetCfmMsg m_msg;
};

class HeadlightPatternReq : public MsgEvt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    HeadlightPatternReq(HeadlightPatternReqMsg const &r) :
        MsgEvt(HEADLIGHT_PATTERN_REQ, m_msg), m_msg(r) {
        HEADLIGHT_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    HeadlightPatternReq(uint32_t patternIndex, bool isRepeat = false) :
        HeadlightPatternReq(HeadlightPatternReqMsg(patternIndex, isRepeat)) {}
    uint32_t GetPatternIndex() const { return m_msg.GetPatternIndex(); }
    bool IsRepeat() const { return m_msg.IsRepeat(); }
private:
    HeadlightPatternReqMsg m_msg;
};

class HeadlightPatternCfm : public ErrorMsgEvt {
public:
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    HeadlightPatternCfm(HeadlightPatternCfmMsg const &r) :
        ErrorMsgEvt(HEADLIGHT_PATTERN_CFM, m_msg), m_msg(r) {
        HEADLIGHT_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    HeadlightPatternCfm(char const *error, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        HeadlightPatternCfm(HeadlightPatternCfmMsg(error, origin, reason)) {}
protected:
    HeadlightPatternCfmMsg m_msg;
};

class HeadlightOffReq : public MsgEvt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    HeadlightOffReq(HeadlightOffReqMsg const &r) :
        MsgEvt(HEADLIGHT_OFF_REQ, m_msg), m_msg(r) {
        HEADLIGHT_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    HeadlightOffReq() :
        HeadlightOffReq(HeadlightOffReqMsg()) {}
private:
    HeadlightOffReqMsg m_msg;
};

class HeadlightOffCfm : public ErrorMsgEvt {
public:
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'r'.
    HeadlightOffCfm(HeadlightOffCfmMsg const &r) :
        ErrorMsgEvt(HEADLIGHT_OFF_CFM, m_msg), m_msg(r) {
        HEADLIGHT_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    HeadlightOffCfm(char const *error, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        HeadlightOffCfm(HeadlightOffCfmMsg(error, origin, reason)) {}
protected:
    HeadlightOffCfmMsg m_msg;
};

} // namespace APP

#endif // HEADLIGHT_INTERFACE_H
