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

#ifndef LED_FRAME_H
#define LED_FRAME_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Graphics.h"
#include "LCDConf_Lin_Template.h"

using namespace QP;
using namespace FW;

namespace APP {

class LedFrame : public Region {
public:
    LedFrame();

protected:
    static QState InitialPseudoState(LedFrame * const me, QEvt const * const e);
    static QState Root(LedFrame * const me, QEvt const * const e);
        static QState Stopped(LedFrame * const me, QEvt const * const e);
        static QState Starting(LedFrame * const me, QEvt const * const e);
        static QState Stopping(LedFrame * const me, QEvt const * const e);
        static QState Started(LedFrame * const me, QEvt const * const e);
            static QState Idle(LedFrame * const me, QEvt const * const e);
            static QState Busy(LedFrame * const me, QEvt const * const e);

    uint8_t *m_frameBuf;
    uint32_t m_frameBufLen;

    // Round-up and align to 32-byte boundary to support caching in the future.
    uint8_t m_dmaBuf[ROUND_UP_32(LCD_BUF_LEN)] __attribute__((aligned(32)));
    Area m_updateArea;  // Area in frame buffer to update to LCD.
    Evt m_inEvt;        // Static event copy of a generic incoming req to be confirmed. Added more if needed.

    Timer m_stateTimer;

#define LED_FRAME_TIMER_EVT \
    ADD_EVT(STATE_TIMER)

#define LED_FRAME_INTERNAL_EVT \
    ADD_EVT(NEXT) \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        LED_FRAME_TIMER_EVT_START = TIMER_EVT_START(LED_FRAME),
        LED_FRAME_TIMER_EVT
    };

    enum {
        LED_FRAME_INTERNAL_EVT_START = INTERNAL_EVT_START(LED_FRAME),
        LED_FRAME_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // LED_FRAME_H
