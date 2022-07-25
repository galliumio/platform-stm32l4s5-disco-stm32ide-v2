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

#ifndef WS2812_INTERFACE_H
#define WS2812_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "fw_kv.h"
#include "fw_inline.h"
#include "app_hsmn.h"

#define WS2812_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("Ws2812Interface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define WS2812_INTERFACE_EVT \
    ADD_EVT(WS2812_START_REQ) \
    ADD_EVT(WS2812_START_CFM) \
    ADD_EVT( WS2812_STOP_REQ) \
    ADD_EVT(WS2812_STOP_CFM) \
    ADD_EVT(WS2812_SET_REQ) \
    ADD_EVT(WS2812_SET_CFM)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    WS2812_INTERFACE_EVT_START = INTERFACE_EVT_START(WS2812),
    WS2812_INTERFACE_EVT
};

enum {
    WS2812_REASON_UNSPEC = 0,
};

class Ws2812StartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200
    };
    Ws2812StartReq(uint32_t ledCount) :
        Evt(WS2812_START_REQ), m_ledCount(ledCount) {}
    uint32_t GetLedCount() const { return m_ledCount; }
private:
    uint32_t m_ledCount;
};

class Ws2812StartCfm : public ErrorEvt {
public:
    Ws2812StartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WS2812_START_CFM, error, origin, reason) {}
};

class Ws2812StopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200
    };
    Ws2812StopReq() :
        Evt(WS2812_STOP_REQ) {}
};

class Ws2812StopCfm : public ErrorEvt {
public:
    Ws2812StopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WS2812_STOP_CFM, error, origin, reason) {}
};

class Ws2812SetReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200,
        MAX_LED_COUNT = 16      // When this is increased, we may need to increase event pool size in fw.h.
                                // Each LedColor object takes 8 bytes.
    };

    // Key is LED index. Value is color (888 xBGR).
    using LedColor = KeyValue<uint32_t, uint32_t>;

    Ws2812SetReq(LedColor const *colors, uint32_t count) :
        Evt(WS2812_SET_REQ), m_count(LESS(count, MAX_LED_COUNT)) {
        ArrayCopy(m_colors, colors, m_count);
    }
    LedColor const *GetColors() const { return m_colors; }
    uint32_t GetCount() const { return m_count; }
private:
    // A list of index-color pair to specify the color to be set to each LED in the list.
    LedColor m_colors[MAX_LED_COUNT];
    uint32_t m_count;
};

class Ws2812SetCfm : public ErrorEvt {
public:
    Ws2812SetCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WS2812_SET_CFM, error, origin, reason) {}
};

} // namespace APP

#endif // WS2812_INTERFACE_H
