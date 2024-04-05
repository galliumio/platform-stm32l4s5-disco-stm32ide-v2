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

#ifndef GPIO_IN_H
#define GPIO_IN_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class GpioIn : public Region {
public:
    class Config {
    public:
        Hsmn hsmn;
        GPIO_TypeDef *port;
        uint16_t pin;
        uint32_t pull;
        uint32_t speed;
        bool activeHigh;
        bool tracking;     // True if the HSM tracks the pin state. Interrupt handling is deferred with interrupt disabled.
                           // False if an active interrupt sends a GPIO_IN_ACTIVE_IND event directly.
    };

    // Typedef an array to allow fast lookup of a Config object from the GPIO pin number [0-15].
    typedef Config const *PinConfigMap[16];
    static PinConfigMap &GetPinConfigMap();
    static void SaveConfig(uint16_t pin, Config const *config);
    static Config const *GetConfig(uint16_t pin);
    static void GpioIntCallback(uint16_t pin);

    GpioIn();

protected:
    static QState InitialPseudoState(GpioIn * const me, QEvt const * const e);
    static QState Root(GpioIn * const me, QEvt const * const e);
        static QState Stopped(GpioIn * const me, QEvt const * const e);
        static QState Started(GpioIn * const me, QEvt const * const e);
            static QState TriggerMode(GpioIn * const me, QEvt const * const e);
            static QState TrackMode(GpioIn * const me, QEvt const * const e);
                static QState Inactive(GpioIn * const me, QEvt const * const e);
                static QState Active(GpioIn * const me, QEvt const * const e);
                    static QState PulseWait(GpioIn * const me, QEvt const * const e);
                    static QState HoldWait(GpioIn * const me, QEvt const * const e);
                    static QState Held(GpioIn * const me, QEvt const * const e);

    void InitGpio();
    void DeInitGpio();
    static void EnableGpioInt(uint16_t pin);
    static void DisableGpioInt(uint16_t pin);
    bool IsActive();

    static Config const CONFIG[];

    Config const *m_config;
    Hsmn m_client;
    bool m_filterGlitch;      // True to filter glitches.
                              // False to artificially generate active/inactive indications upon detected glitch (assuming single glitch).
    uint32_t m_holdCount;

    enum {
        PULSE_TIMEOUT_MS = 50,
        HOLD_TIMEOUT_MS = 1000
    };

    Timer m_stateTimer;
    Timer m_pulseTimer;
    Timer m_holdTimer;

#define GPIO_IN_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(PULSE_TIMER) \
    ADD_EVT(HOLD_TIMER)

#define GPIO_IN_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(TRIGGER) \
    ADD_EVT(PIN_INACTIVE) \
    ADD_EVT(PIN_ACTIVE) \
    ADD_EVT(SELF)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        GPIO_IN_TIMER_EVT_START = TIMER_EVT_START(GPIO_IN),
        GPIO_IN_TIMER_EVT
    };

    enum {
        GPIO_IN_INTERNAL_EVT_START = INTERNAL_EVT_START(GPIO_IN),
        GPIO_IN_INTERNAL_EVT
    };
};

} // namespace APP

#endif // GPIO_IN_H
