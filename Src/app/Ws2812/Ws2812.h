/*******************************************************************************
 * Copyright (C) 2022 Gallium Studio LLC (Lawrence Lo). All rights reserved.
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

#ifndef WS2812_H
#define WS2812_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "Pin.h"
#include "app_hsmn.h"
#include "Ws2812Interface.h"

using namespace QP;
using namespace FW;

namespace APP {

class Ws2812 : public Active {
public:
    typedef KeyValue<Hsmn, TIM_HandleTypeDef *> HsmnHal;
    typedef Map<Hsmn, TIM_HandleTypeDef *> HsmnHalMap;
    static HsmnHalMap &GetHsmnHalMap();
    static void SaveHal(Hsmn hsmn, TIM_HandleTypeDef *hal);
    static TIM_HandleTypeDef *Hsm2Hal(Hsmn hsmn);
    static Hsmn Hal2Hsm(TIM_HandleTypeDef *hal);
    typedef KeyValue<Hsmn, DMA_HandleTypeDef *> HsmnDma;
    typedef Map<Hsmn, DMA_HandleTypeDef *> HsmnDmaMap;
    static HsmnDmaMap &GetHsmnDmaMap();
    static void SaveDma(Hsmn hsmn, DMA_HandleTypeDef *dma);
    static DMA_HandleTypeDef *Hsm2Dma(Hsmn hsmn);
    static Hsmn Dma2Hsm(DMA_HandleTypeDef *dma);

    Ws2812(Hsmn hsmn, char const *name);

    static void DmaCompleteCallback(TIM_HandleTypeDef *hal);

protected:
    static QState InitialPseudoState(Ws2812 * const me, QEvt const * const e);
    static QState Root(Ws2812 * const me, QEvt const * const e);
        static QState Stopped(Ws2812 * const me, QEvt const * const e);
        static QState Starting(Ws2812 * const me, QEvt const * const e);
        static QState Stopping(Ws2812 * const me, QEvt const * const e);
        static QState Started(Ws2812 * const me, QEvt const * const e);
            static QState Idle(Ws2812 * const me, QEvt const * const e);
            static QState Busy(Ws2812 * const me, QEvt const * const e);
                static QState Normal(Ws2812 * const me, QEvt const * const e);
                static QState SetPending(Ws2812 * const me, QEvt const * const e);
                static QState StopPending(Ws2812 * const me, QEvt const * const e);
                    static QState SetDoneWait(Ws2812 * const me, QEvt const * const e);
                    static QState ClearDoneWait(Ws2812 * const me, QEvt const * const e);

    enum {
        // No. of PWM cycles per single-color level
        CYCLES_PER_LEVEL = 8,
        // No. of PWM cycles per pixel (RGB color levels)
        CYCLES_PER_PIXEL = CYCLES_PER_LEVEL * 3,
        MAX_LED_COUNT = Ws2812SetReq::MAX_LED_COUNT,
        // A period of low signal level before starting a new data transfer.
        RESET_CYCLES = 80,
        // After data transfer completes, the last PWM cycle repeats until DMA_DONE is processed (calling StopDma()).
        // Appending a single placeholder cycle of low signal level essentially stops PWM immediately after last data bit.
        POST_CYCLES = 1,
        // Size of PWM data buffer.
        PWM_DATA_LEN = RESET_CYCLES + (MAX_LED_COUNT * CYCLES_PER_PIXEL) + POST_CYCLES
    };

    void InitGpio();
    void DeInitGpio();
    void InitPwmTim();
    void DeInitPwmTim();
    void InitPwmDma();
    void DeInitPwmDma();
    void StartDma();
    void StopDma();
    void ClearColorBuf() {
        memset(m_colorBuf, 0, sizeof(m_colorBuf));
    }
    void UpdateColorBuf(Ws2812SetReq const &req);
    void Level2DutyCycle(uint8_t level, uint32_t *buf, uint32_t bufLen);
    void InitPwmData() {
        memset(m_pwmData, 0, sizeof(m_pwmData));
    }
    void GenPwmData();

    class Config {
    public:
        // Key
        Hsmn hsmn;
        // Data pin.
        GPIO_TypeDef *gpioPort;
        uint16_t gpioPin;
        uint32_t gpioAf;
        TIM_TypeDef *pwmTim;
        uint32_t pwmCh;
        bool pwmComplementary;
        DMA_Channel_TypeDef *dmaCh;     // Similar to stream in F4.
        uint32_t dmaReq;                // Similar to channel in F4.
        IRQn_Type dmaIrq;
        uint32_t dmaPrio;
    };
    static Config const CONFIG[];
    Config const *m_config;
    TIM_HandleTypeDef m_hal;
    DMA_HandleTypeDef m_dma;
    Pin m_pin;
    Evt m_inEvt;                        // Static event copy of a generic incoming req to be confirmed. Added more if needed.
    uint32_t m_ledCount;                // Actual no. of LEDs used.
    uint32_t m_colorBuf[MAX_LED_COUNT]; // Buffered color of all LEDs.
    // Data buffer storing PWM duty cycles for DMA.
    // Must be aligned and rounded-up to cache-line size (32B or 8DW) if cache is used (e.g. STM32H7).
    uint32_t m_pwmData[ROUND_UP_8(PWM_DATA_LEN)] __attribute__((aligned(32)));

    Timer m_stateTimer;
    Timer m_busyTimer;

    enum {
        BUSY_TIMEOUT_MS = 50,
    };

#define WS2812_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(BUSY_TIMER)

#define WS2812_INTERNAL_EVT \
    ADD_EVT(START) \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(DMA_DONE) \
    ADD_EVT(CONTINUE) \
    ADD_EVT(CLEAR_WAIT) \
    ADD_EVT(FINISHED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        WS2812_TIMER_EVT_START = TIMER_EVT_START(WS2812),
        WS2812_TIMER_EVT
    };

    enum {
        WS2812_INTERNAL_EVT_START = INTERNAL_EVT_START(WS2812),
        WS2812_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // WS2812_H
