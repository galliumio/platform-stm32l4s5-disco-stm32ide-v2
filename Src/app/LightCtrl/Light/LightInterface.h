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

#ifndef LIGHT_INTERFACE_H
#define LIGHT_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"

#define LIGHT_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("LightInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define LIGHT_INTERFACE_EVT \
    ADD_EVT(LIGHT_START_REQ) \
    ADD_EVT(LIGHT_START_CFM) \
    ADD_EVT(LIGHT_STOP_REQ) \
    ADD_EVT(LIGHT_STOP_CFM) \
    ADD_EVT(LIGHT_EXCEPTION_IND) \
    ADD_EVT(LIGHT_SET_REQ) \
    ADD_EVT(LIGHT_SET_CFM) \
    ADD_EVT(LIGHT_PATTERN_REQ) \
    ADD_EVT(LIGHT_PATTERN_CFM) \
    ADD_EVT(LIGHT_OFF_REQ) \
    ADD_EVT(LIGHT_OFF_CFM)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    LIGHT_INTERFACE_EVT_START = INTERFACE_EVT_START(LIGHT),
    LIGHT_INTERFACE_EVT
};

enum {
    LIGHT_REASON_UNSPEC = 0,
    LIGHT_REASON_INVALID_PATTERN,
};

// ### IMPORTANT ###
// The following enum MUST match the LIGHT_PATTERN_SET definition in LightPattern.cpp.
enum class LightPatternIdx {
    BREATHING_WHITE,
    BREATHING_RED,
    ALERT_RED,
    ALERT_BLUE,
    ALERT_RED_BLUE,
    ALERT_AMBER_WHITE,
    STROBE_WHITE,
    // Add more patterns here.
    INVALID,
    COUNT = INVALID
};

class LightStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    LightStartReq(Hsmn ws2812Hsmn) :
        Evt(LIGHT_START_REQ), m_ws2812Hsmn(ws2812Hsmn) {}
    Hsmn GetWs2812Hsmn() const { return m_ws2812Hsmn; }
private:
    Hsmn m_ws2812Hsmn;
};

class LightStartCfm : public ErrorEvt {
public:
    LightStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_START_CFM, error, origin, reason) {}
};

class LightStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200
    };
    LightStopReq() :
        Evt(LIGHT_STOP_REQ) {}
};

class LightStopCfm : public ErrorEvt {
public:
    LightStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_STOP_CFM, error, origin, reason) {}
};

class LightExceptionInd : public ErrorEvt {
public:
    LightExceptionInd(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_EXCEPTION_IND, error, origin, reason) {}
};

class LightSetReq : public Evt {
public:
    enum {
        MAX_RAMP_MS = 5000,
        TIMEOUT_MS = MAX_RAMP_MS + 200
    };
    LightSetReq(uint32_t color, uint32_t rampMs = 1) :
        Evt(LIGHT_SET_REQ), m_color(color), m_rampMs(rampMs) {}
    uint32_t GetColor() const { return m_color; }
    uint32_t GetRampMs() const { return m_rampMs; }
private:
    uint32_t m_color;       // Set color (888 xBGR).
    uint32_t m_rampMs;      // Ramp time from current color to set color in ms.
};

class LightSetCfm : public ErrorEvt {
public:
    LightSetCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_SET_CFM, error, origin, reason) {}
};

class LightPatternReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200
    };

    LightPatternReq(LightPatternIdx patternIndex, bool isRepeat = true, uint32_t startInterval = 0) :
        Evt(LIGHT_PATTERN_REQ), m_patternIndex(patternIndex),
        m_isRepeat(isRepeat), m_startInterval(startInterval) {
        LIGHT_INTERFACE_ASSERT(patternIndex < LightPatternIdx::INVALID);
    }
    LightPatternIdx GetPatternIndex() const { return m_patternIndex; }
    bool IsRepeat() const { return m_isRepeat; }
    uint32_t GetStartInterval() const { return m_startInterval; }
private:
    LightPatternIdx m_patternIndex;
    bool m_isRepeat;
    uint32_t m_startInterval;   // Starting interval index (modulo interval count).
};

class LightPatternCfm : public ErrorEvt {
public:
    LightPatternCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_PATTERN_CFM, error, origin, reason) {}
};

class LightOffReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200
    };
    LightOffReq() :
        Evt(LIGHT_OFF_REQ) {}
};

class LightOffCfm : public ErrorEvt {
public:
    LightOffCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LIGHT_OFF_CFM, error, origin, reason) {}
};

} // namespace APP

#endif // LIGHT_INTERFACE_H
