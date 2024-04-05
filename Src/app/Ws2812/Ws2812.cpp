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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "TestPins.h"
#include "Colors.h"
#include "Ws2812Interface.h"
#include "Ws2812.h"

FW_DEFINE_THIS_FILE("Ws2812.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "WS2812_TIMER_EVT_START",
    WS2812_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "WS2812_INTERNAL_EVT_START",
    WS2812_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "WS2812_INTERFACE_EVT_START",
    WS2812_INTERFACE_EVT
};

Ws2812::HsmnHalMap &Ws2812::GetHsmnHalMap() {
    static HsmnHal hsmnHalStor[WS2812_COUNT];
    static HsmnHalMap hsmnHalMap(hsmnHalStor, ARRAY_COUNT(hsmnHalStor), HsmnHal(HSM_UNDEF, NULL));
    return hsmnHalMap;
}

// No need to have critical section since mapping is not changed after global/static object construction.
void Ws2812::SaveHal(Hsmn hsmn, TIM_HandleTypeDef *hal) {
    GetHsmnHalMap().Save(HsmnHal(hsmn, hal));
}

// No need to have critical section since mapping is not changed after global/static object construction.
TIM_HandleTypeDef *Ws2812::Hsm2Hal(Hsmn hsmn) {
    FW_ASSERT(hsmn != HSM_UNDEF);
    HsmnHal *kv = GetHsmnHalMap().GetByKey(hsmn);
    FW_ASSERT(kv);
    TIM_HandleTypeDef *hal = kv->GetValue();
    FW_ASSERT(hal);
    return hal;
}

// No need to have critical section since mapping is not changed after global/static object construction.
Hsmn Ws2812::Hal2Hsm(TIM_HandleTypeDef *hal) {
    FW_ASSERT(hal);
    HsmnHal *kv = GetHsmnHalMap().GetFirstByValue(hal);
    FW_ASSERT(kv);
    Hsmn hsmn = kv->GetKey();
    FW_ASSERT(hsmn != HSM_UNDEF);
    return hsmn;
}

Ws2812::HsmnDmaMap &Ws2812::GetHsmnDmaMap() {
    static HsmnDma hsmnDmaStor[WS2812_COUNT];
    static HsmnDmaMap hsmnDmaMap(hsmnDmaStor, ARRAY_COUNT(hsmnDmaStor), HsmnDma(HSM_UNDEF, NULL));
    return hsmnDmaMap;
}

// No need to have critical section since mapping is not changed after global/static object construction.
void Ws2812::SaveDma(Hsmn hsmn, DMA_HandleTypeDef *dma) {
    GetHsmnDmaMap().Save(HsmnDma(hsmn, dma));
}

// No need to have critical section since mapping is not changed after global/static object construction.
DMA_HandleTypeDef *Ws2812::Hsm2Dma(Hsmn hsmn) {
    FW_ASSERT(hsmn != HSM_UNDEF);
    HsmnDma *kv = GetHsmnDmaMap().GetByKey(hsmn);
    FW_ASSERT(kv);
    DMA_HandleTypeDef *dma = kv->GetValue();
    FW_ASSERT(dma);
    return dma;
}

// No need to have critical section since mapping is not changed after global/static object construction.
Hsmn Ws2812::Dma2Hsm(DMA_HandleTypeDef *dma) {
    FW_ASSERT(dma);
    HsmnDma *kv = GetHsmnDmaMap().GetFirstByValue(dma);
    FW_ASSERT(kv);
    Hsmn hsmn = kv->GetKey();
    FW_ASSERT(hsmn != HSM_UNDEF);
    return hsmn;
}

void Ws2812::DmaCompleteCallback(TIM_HandleTypeDef *hal) {
    Hsmn hsmn = Hal2Hsm(hal);
    Evt *evt = new Evt(Ws2812::DMA_DONE, hsmn, HSM_UNDEF, 0);
    Fw::Post(evt);
}

// TIMx configuration:
// APB1CLK = HCLK -> TIM1CLK = HCLK = SystemCoreClock (See "clock tree" and "timer clock" in ref manual.)
#define TIM_CLK             (SystemCoreClock)   // 120MHz
#define TIM_COUNTER_CLK     (120000000)         // 120MHz
#define TIM_PWM_FREQ        (800000)            // 800kHz (1.25us)
#define TIM_PWM_PRESCALER   ((TIM_CLK / TIM_COUNTER_CLK) - 1)
#define TIM_PWM_PERIOD      ((TIM_COUNTER_CLK / TIM_PWM_FREQ) - 1)
#define DUTY_CYCLE(_x)      (((TIM_PWM_PERIOD + 1) * (_x)) / 100)
#define B0                  DUTY_CYCLE(32)      // Bit '0'.
#define B1                  DUTY_CYCLE(64)      // Bit '1'

Ws2812::Config const Ws2812::CONFIG[] = {
    // Primary.
    { WS2812, GPIOA, GPIO_PIN_15, GPIO_AF1_TIM2, TIM2, TIM_CHANNEL_1, false, DMA2_Channel5, DMA_REQUEST_TIM2_CH1, DMA2_Channel5_IRQn, DMA2_CHANNEL5_PRIO},
    // Add more WS2812 control line here.
};


void Ws2812::InitGpio() {
    // Enables pull-down to ensure pin stays low when DMA is inactive. Otherwise data pin was at > 1V between DMA transfers.
    m_pin.Init(m_config->gpioPort, m_config->gpioPin, GPIO_MODE_AF_PP, GPIO_PULLDOWN, m_config->gpioAf);
}

void Ws2812::DeInitGpio() {
    m_pin.DeInit();
}

void Ws2812::InitPwmTim() {
    // Initialize TIM for WS2812 PWM.
    switch((uint32_t)(m_config->pwmTim)) {
        case TIM1_BASE: __HAL_RCC_TIM1_CLK_ENABLE(); break;
        case TIM2_BASE: __HAL_RCC_TIM2_CLK_ENABLE(); break;
        case TIM8_BASE: __HAL_RCC_TIM8_CLK_ENABLE(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }

    //m_config->pwmTim->CR1 |= TIM_CR1_OPM;
    HAL_StatusTypeDef status;
    m_hal.Instance = m_config->pwmTim;
    m_hal.Init.Prescaler = TIM_PWM_PRESCALER;
    m_hal.Init.Period = TIM_PWM_PERIOD;
    m_hal.Init.ClockDivision = 0;
    m_hal.Init.CounterMode = TIM_COUNTERMODE_UP;
    m_hal.Init.RepetitionCounter = 0;
    status = HAL_TIM_PWM_Init(&m_hal);
    FW_ASSERT(status == HAL_OK);

    TIM_OC_InitTypeDef timOc;
    memset(&timOc, 0, sizeof(timOc));
    timOc.OCMode       = TIM_OCMODE_PWM1;
    timOc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    timOc.Pulse        = 0;
    timOc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    timOc.OCFastMode   = TIM_OCFAST_DISABLE;
    timOc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    timOc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    status = HAL_TIM_PWM_ConfigChannel(&m_hal, &timOc, m_config->pwmCh);
    FW_ASSERT(status == HAL_OK);
    Log::Debug(Log::TYPE_LOG, GetHsmn(), "PWM Period=%d, Pulse=%d", m_hal.Init.Period, timOc.Pulse);
}

void Ws2812::DeInitPwmTim() {

    HAL_TIM_PWM_DeInit(&m_hal);
    switch((uint32_t)(m_config->pwmTim)) {
        case TIM1_BASE: __HAL_RCC_TIM1_CLK_DISABLE(); break;
        case TIM2_BASE: __HAL_RCC_TIM2_CLK_DISABLE(); break;
        case TIM8_BASE: __HAL_RCC_TIM8_CLK_DISABLE(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }
}

void Ws2812::InitPwmDma() {
    m_dma.Instance                 = m_config->dmaCh;
    m_dma.Init.Request             = m_config->dmaReq;
    m_dma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    m_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    m_dma.Init.MemInc              = DMA_MINC_ENABLE;
    m_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    m_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    m_dma.Init.Mode                = DMA_NORMAL;  // DMA_CIRCULAR;
    m_dma.Init.Priority            = DMA_PRIORITY_VERY_HIGH;

    uint16_t index;
    switch(m_config->pwmCh) {
        case TIM_CHANNEL_1: index = TIM_DMA_ID_CC1; break;
        case TIM_CHANNEL_2: index = TIM_DMA_ID_CC2; break;
        case TIM_CHANNEL_3: index = TIM_DMA_ID_CC3; break;
        case TIM_CHANNEL_4: index = TIM_DMA_ID_CC4; break;
        default: FW_ASSERT(0);
    }
    __HAL_LINKDMA(&m_hal, hdma[index], m_dma);
    //FW_CRIT_ENTRY();
    HAL_StatusTypeDef status = HAL_DMA_Init(&m_dma);
    //FW_CRIT_EXIT();
    FW_ASSERT(status == HAL_OK);

    NVIC_SetPriority(m_config->dmaIrq, m_config->dmaPrio);
    NVIC_EnableIRQ(m_config->dmaIrq);
}

void Ws2812::DeInitPwmDma() {
    //FW_CRIT_ENTRY();
    HAL_StatusTypeDef status = HAL_DMA_DeInit(&m_dma);
    //FW_CRIT_EXIT();
    FW_ASSERT(status == HAL_OK);
}

void Ws2812::StartDma() {
    uint16_t cycles = RESET_CYCLES + (m_ledCount * CYCLES_PER_PIXEL) + POST_CYCLES;
    FW_ASSERT(cycles <= ARRAY_COUNT(m_pwmData));
    HAL_StatusTypeDef status = HAL_TIM_PWM_Start_DMA(&m_hal, m_config->pwmCh, m_pwmData, cycles);
    FW_ASSERT(status == HAL_OK);
}

void Ws2812::StopDma() {
    HAL_TIM_PWM_Stop_DMA(&m_hal, m_config->pwmCh);
}

void Ws2812::UpdateColorBuf(Ws2812SetReq const &req) {
    auto me = this; // For log.
    auto colors = req.GetColors();
    FW_ASSERT(colors);
    for (uint32_t i = 0; i < req.GetCount() ; i++) {
        uint32_t idx = colors[i].GetKey();
        if (idx < m_ledCount) {
            FW_ASSERT(idx < ARRAY_COUNT(m_colorBuf));
            INFO("LED[%d] = %x", idx, colors[i].GetValue());
            m_colorBuf[idx] = colors[i].GetValue();
        } else {
            WARNING("UpdateColorBuf - Invalid LED idx %lu", idx);
        }
    }
}

void Ws2812::Level2DutyCycle(uint8_t level, uint32_t *buf, uint32_t bufLen) {
    FW_ASSERT(buf && (bufLen == CYCLES_PER_LEVEL));
    for (uint32_t i = 0; i < CYCLES_PER_LEVEL; i++) {
        buf[i] = (level & 0x80) ? B1 : B0;
        level <<= 1;
    }
}

void Ws2812::GenPwmData() {
    // Skips over the initial reset cycles which remain at 0.
    uint32_t *buf = m_pwmData + RESET_CYCLES;
    FW_ASSERT(CYCLES_PER_PIXEL == (3 * CYCLES_PER_LEVEL));
    FW_ASSERT((buf + (m_ledCount * CYCLES_PER_PIXEL) + POST_CYCLES) <= (m_pwmData + ARRAY_COUNT(m_pwmData)));
    uint32_t i;
    for (i = 0; i < m_ledCount; i++) {
        Color888Bgr color(m_colorBuf[i]);
        // The order of colors sent to WS2812 is green, red and blue.
        Level2DutyCycle(color.Green(), buf, CYCLES_PER_LEVEL);
        Level2DutyCycle(color.Red(), buf + CYCLES_PER_LEVEL, CYCLES_PER_LEVEL);
        Level2DutyCycle(color.Blue(), buf + 2*CYCLES_PER_LEVEL, CYCLES_PER_LEVEL);
        buf += CYCLES_PER_PIXEL;
    }
    // Clears the post-cycles after the last data bit to stops PWM immediately.
    for (i = 0; i < POST_CYCLES; i++) {
        *buf++ = 0;
    }
}

Ws2812::Ws2812(Hsmn hsmn, char const *name) :
    Active((QStateHandler)&Ws2812::InitialPseudoState, hsmn, name),
    m_config(NULL), m_inEvt(QEvt::STATIC_EVT), m_ledCount(0),
    m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER),
    m_busyTimer(GetHsm().GetHsmn(), BUSY_TIMER) {
    SET_EVT_NAME(WS2812);
    FW_ASSERT((hsmn >= WS2812) && (hsmn <= WS2812_LAST));
    memset(&m_hal, 0, sizeof(m_hal));
    memset(&m_dma, 0, sizeof(m_dma));
    uint32_t i;
    for (i = 0; i < ARRAY_COUNT(CONFIG); i++) {
        if (CONFIG[i].hsmn == hsmn) {
            m_config = &CONFIG[i];
            break;
        }
    }
    FW_ASSERT(i < ARRAY_COUNT(CONFIG));
    // Save hsmn to HAL mapping.
    SaveHal(hsmn, &m_hal);
    SaveDma(hsmn, &m_dma);
    ClearColorBuf();
    InitPwmData();
}

QState Ws2812::InitialPseudoState(Ws2812 * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Ws2812::Root);
}

QState Ws2812::Root(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Ws2812::Stopped);
        }
        case WS2812_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new Ws2812StartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case WS2812_SET_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new Ws2812SetCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case WS2812_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Ws2812::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Ws2812::Stopped(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WS2812_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new Ws2812StopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case WS2812_START_REQ: {
            EVENT(e);
            Ws2812StartReq const &req = static_cast<Ws2812StartReq const &>(*e);
            if (req.GetLedCount() <= MAX_LED_COUNT) {
                me->m_inEvt = req;
                me->m_ledCount = req.GetLedCount();
                me->Raise(new Evt(START));
            } else {
                me->SendCfm(new Ws2812StartCfm(ERROR_PARAM, me->GetHsmn()), req);
            }
            return Q_HANDLED();
        }
        case START: {
            EVENT(e);
            return Q_TRAN(&Ws2812::Starting);
        }
    }
    return Q_SUPER(&Ws2812::Root);
}

QState Ws2812::Starting(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = Ws2812StartReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            // Since there is no other initialization, send DONE immediately. Do not use Raise() in entry action.
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new Ws2812StartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new Ws2812StartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Ws2812::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new Ws2812StartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Ws2812::Started);
        }
    }
    return Q_SUPER(&Ws2812::Root);
}

QState Ws2812::Stopping(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = Ws2812StopReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            // Since there is no other cleanup, send DONE immediately. Do not use Raise() in entry action.
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case WS2812_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Ws2812::Stopped);
        }
    }
    return Q_SUPER(&Ws2812::Root);
}

QState Ws2812::Started(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->InitGpio();
            me->InitPwmTim();
            me->InitPwmDma();
            me->InitPwmData();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->DeInitPwmDma();
            me->DeInitPwmTim();
            me->DeInitGpio();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            me->ClearColorBuf();
            return Q_TRAN(&Ws2812::Busy);
        }
        case FINISHED: {
            EVENT(e);
            return Q_TRAN(&Ws2812::Stopping);
        }
    }
    return Q_SUPER(&Ws2812::Root);
}

QState Ws2812::Idle(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WS2812_SET_REQ: {
            EVENT(e);
            Ws2812SetReq const &req = static_cast<Ws2812SetReq const &>(*e);
            me->UpdateColorBuf(req);
            me->SendCfm(new Ws2812SetCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&Ws2812::Busy);
        }
        case WS2812_STOP_REQ: {
            EVENT(e);
            me->ClearColorBuf();
            me->Defer(e);
            return Q_TRAN(&Ws2812::ClearDoneWait);
        }
    }
    return Q_SUPER(&Ws2812::Started);
}

QState Ws2812::Busy(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->GenPwmData();
            me->StartDma();
            me->m_busyTimer.Start(BUSY_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->StopDma();
            me->m_busyTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Ws2812::Normal);
        }
        case BUSY_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            return Q_HANDLED();
        }
        case WS2812_SET_REQ: {
            EVENT(e);
            Ws2812SetReq const &req = static_cast<Ws2812SetReq const &>(*e);
            me->UpdateColorBuf(req);
            me->SendCfm(new Ws2812SetCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&Ws2812::SetPending);
        }
        case WS2812_STOP_REQ: {
            EVENT(e);
            me->ClearColorBuf();
            me->Defer(e);
            return Q_TRAN(&Ws2812::StopPending);
        }
        case CLEAR_WAIT: {
            EVENT(e);
            return Q_TRAN(&Ws2812::ClearDoneWait);
        }
        case CONTINUE: {
            EVENT(e);
            return Q_TRAN(&Ws2812::Busy);
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Ws2812::Idle);
        }
    }
    return Q_SUPER(&Ws2812::Started);
}

QState Ws2812::Normal(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DMA_DONE: {
            EVENT(e);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Ws2812::Busy);
}

QState Ws2812::SetPending(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DMA_DONE: {
            EVENT(e);
            me->Raise(new Evt(CONTINUE));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Ws2812::Busy);
}

QState Ws2812::StopPending(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->Recall();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Ws2812::SetDoneWait);
        }
        case WS2812_SET_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new Ws2812SetCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case WS2812_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Ws2812::Busy);
}

QState Ws2812::SetDoneWait(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DMA_DONE: {
            EVENT(e);
            me->Raise(new Evt(CONTINUE));
            me->Raise(new Evt(CLEAR_WAIT));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Ws2812::StopPending);
}

QState Ws2812::ClearDoneWait(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DMA_DONE: {
            EVENT(e);
            me->Raise(new Evt(FINISHED));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Ws2812::StopPending);
}

/*
QState Ws2812::MyState(Ws2812 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Ws2812::SubState);
        }
    }
    return Q_SUPER(&Ws2812::SuperState);
}
*/

} // namespace APP
