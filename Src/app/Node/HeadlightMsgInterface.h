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

#ifndef HEADLIGHT_MSG_INTERFACE_H
#define HEADLIGHT_MSG_INTERFACE_H

#include "fw_def.h"
#include "fw_msg.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

#define MSG_HEADLIGHT_REASON_INVALID_PATTERN    "Invalid headlight pattern"

namespace APP {

// This file defines messages for the "Headlight" role.

class HeadlightSetReqMsg final: public Msg {
public:
    HeadlightSetReqMsg(uint32_t color = 0, uint32_t rampMs = 1) :
        Msg("HeadlightSetReqMsg"), m_color(color), m_rampMs(rampMs) {
        m_len = sizeof(*this);
    }
    uint32_t GetColor() const { return m_color; }
    uint32_t GetRampMs() const { return m_rampMs; }
private:
    uint32_t m_color;       // Set color (888 xBGR).
    uint32_t m_rampMs;      // Ramp time from current color to set color in ms.
} __attribute__((packed));

class HeadlightSetCfmMsg final : public ErrorMsg {
public:
    HeadlightSetCfmMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("HeadlightSetCfmMsg", error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));

class HeadlightPatternReqMsg final: public Msg {
public:
    HeadlightPatternReqMsg(uint32_t patternIndex = 0, bool isRepeat = false) :
        Msg("HeadlightPatternReqMsg"), m_patternIndex(patternIndex), m_isRepeat(isRepeat) {
        m_len = sizeof(*this);
    }
    uint32_t GetPatternIndex() const { return m_patternIndex; }
    bool IsRepeat() const { return m_isRepeat; }
private:
    uint32_t m_patternIndex;
    bool m_isRepeat;
} __attribute__((packed));

class HeadlightPatternCfmMsg final : public ErrorMsg {
public:
    HeadlightPatternCfmMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("HeadlightPatternCfmMsg", error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));

class HeadlightOffReqMsg final: public Msg {
public:
    HeadlightOffReqMsg() :
        Msg("HeadlightOffReqMsg") {
        m_len = sizeof(*this);
    }
} __attribute__((packed));

class HeadlightOffCfmMsg final : public ErrorMsg {
public:
    HeadlightOffCfmMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("HeadlightOffCfmMsg", error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));

} // namespace APP

#endif // HEADLIGHT_MSG_INTERFACE_H
