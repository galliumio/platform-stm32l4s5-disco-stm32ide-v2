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
#include "GpioInInterface.h"
#include "GpioIn.h"

FW_DEFINE_THIS_FILE("GpioIn.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "GPIO_IN_TIMER_EVT_START",
    GPIO_IN_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "GPIO_IN_INTERNAL_EVT_START",
    GPIO_IN_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "GPIO_IN_INTERFACE_EVT_START",
    GPIO_IN_INTERFACE_EVT
};

// The order below must match that in app_hsmn.h.
static char const * const hsmName[] = {
    "USER_BTN",
    "ACCEL_GYRO_INT",
    "MAG_DRDY",
    "HUMID_TEMP_DRDY",
    "PRESS_INT",
    "BTN_A",
    "BTN_B",
    "HALL_SENSOR"
    // Add more regions here.
};

static Hsmn &GetCurrHsmn() {
    static Hsmn hsmn = GPIO_IN;
    FW_ASSERT(hsmn <= GPIO_IN_LAST);
    return hsmn;
}

static char const * GetCurrName() {
    uint16_t inst = GetCurrHsmn() - GPIO_IN;
    FW_ASSERT(inst < ARRAY_COUNT(hsmName));
    return hsmName[inst];
}

static void IncCurrHsmn() {
    Hsmn &currHsmn = GetCurrHsmn();
    ++currHsmn;
    FW_ASSERT(currHsmn > 0);
}

static uint16_t GetInst(Hsmn hsmn) {
    uint16_t inst = hsmn - GPIO_IN;
    FW_ASSERT(inst < GPIO_IN_COUNT);
    return inst;
}

GpioIn::Config const GpioIn::CONFIG[] = {
    { USER_BTN,        GPIOC, GPIO_PIN_13, GPIO_NOPULL,   GPIO_SPEED_FREQ_LOW,  false, true },
    { ACCEL_GYRO_INT,  GPIOD, GPIO_PIN_11, GPIO_NOPULL,   GPIO_SPEED_FREQ_HIGH, true, true },
    { MAG_DRDY,        GPIOC, GPIO_PIN_8,  GPIO_NOPULL,   GPIO_SPEED_FREQ_HIGH, true, true },
    { HUMID_TEMP_DRDY, GPIOD, GPIO_PIN_15, GPIO_NOPULL,   GPIO_SPEED_FREQ_HIGH, true, true },
    { PRESS_INT,       GPIOD, GPIO_PIN_10, GPIO_NOPULL,   GPIO_SPEED_FREQ_HIGH, true, true },
    { BTN_A,           GPIOC, GPIO_PIN_5,  GPIO_PULLDOWN, GPIO_SPEED_FREQ_LOW,  true, true },
    { BTN_B,           GPIOC, GPIO_PIN_4,  GPIO_PULLDOWN, GPIO_SPEED_FREQ_LOW,  true, true },
    { HALL_SENSOR,     GPIOC, GPIO_PIN_3,  GPIO_NOPULL,   GPIO_SPEED_FREQ_LOW, false, true },
};

GpioIn::PinConfigMap &GpioIn::GetPinConfigMap() {
    static PinConfigMap map;
    // map is guaranteed to be zero-initialized which corresponds to null pointers.
    return map;
}

// pin is a bitmask representing the pin (GPIO_PIN_0 to GPIO_PIN_15).
void GpioIn::SaveConfig(uint16_t pin, Config const *config) {
    auto &map = GetPinConfigMap();
    FW_ASSERT(!config || (config->pin == pin));
    FW_ASSERT(pin);
    // QF_LOG2 is 1-based, so subtract 1 to get 0-based index.
    uint8_t idx = QF_LOG2(pin) - 1;
    FW_ASSERT(idx < ARRAY_COUNT(map));
    map[idx] = config;
}

// Returns NULL if pin is unused.
GpioIn::Config const *GpioIn::GetConfig(uint16_t pin) {
    auto &map = GetPinConfigMap();
    FW_ASSERT(pin);
    // QF_LOG2 is 1-based, so subtract 1 to get 0-based index.
    uint8_t idx = QF_LOG2(pin) - 1;
    FW_ASSERT(idx < ARRAY_COUNT(map));
    return map[idx];
}

void GpioIn::InitGpio() {
    FW_ASSERT(m_config->port);
    switch((uint32_t)m_config->port) {
        case GPIOA_BASE: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case GPIOB_BASE: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case GPIOC_BASE: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case GPIOD_BASE: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        case GPIOE_BASE: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
        case GPIOF_BASE: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
        case GPIOG_BASE: __HAL_RCC_GPIOG_CLK_ENABLE(); break;
        case GPIOH_BASE: __HAL_RCC_GPIOH_CLK_ENABLE(); break;
        default: FW_ASSERT(0); break;
    }
    GPIO_InitTypeDef gpioInit;
    gpioInit.Pin = m_config->pin;
    if (m_config->tracking) {
        gpioInit.Mode = GPIO_MODE_IT_RISING_FALLING;
    } else {
        gpioInit.Mode = (m_config->activeHigh ? GPIO_MODE_IT_RISING : GPIO_MODE_IT_FALLING);
    }
    gpioInit.Pull = m_config->pull;
    gpioInit.Speed = m_config->speed;
    HAL_GPIO_Init(m_config->port, &gpioInit);

    IRQn_Type irq;
    uint32_t prio = QF_AWARE_ISR_CMSIS_PRI;
    switch(m_config->pin) {
        case GPIO_PIN_0: irq = EXTI0_IRQn; prio = EXTI0_PRIO; break;
        case GPIO_PIN_1: irq = EXTI1_IRQn; prio = EXTI1_PRIO; break;
        case GPIO_PIN_2: irq = EXTI2_IRQn; prio = EXTI2_PRIO; break;
        case GPIO_PIN_3: irq = EXTI3_IRQn; prio = EXTI3_PRIO; break;
        case GPIO_PIN_4: irq = EXTI4_IRQn; prio = EXTI4_PRIO; break;
        case GPIO_PIN_5:
        case GPIO_PIN_6:
        case GPIO_PIN_7:
        case GPIO_PIN_8:
        case GPIO_PIN_9: irq = EXTI9_5_IRQn; prio = EXTI9_5_PRIO; break;
        case GPIO_PIN_10:
        case GPIO_PIN_11:
        case GPIO_PIN_12:
        case GPIO_PIN_13:
        case GPIO_PIN_14:
        case GPIO_PIN_15: irq = EXTI15_10_IRQn; prio = EXTI15_10_PRIO; break;
        default: FW_ASSERT(0); irq = (IRQn_Type)0; break;   // To avoid warning.
    }
    NVIC_SetPriority(irq, prio);
    NVIC_EnableIRQ(irq);
}

void GpioIn::EnableGpioInt(uint16_t pin) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    EXTI->IMR1 = BIT_SET(EXTI->IMR1, pin, 0);
    QF_CRIT_EXIT(crit);
}

void GpioIn::DisableGpioInt(uint16_t pin) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    EXTI->IMR1 = BIT_CLR(EXTI->IMR1, pin, 0);
    QF_CRIT_EXIT(crit);
}

void GpioIn::GpioIntCallback(uint16_t pin) {
    static Sequence counter = 0;
    GpioIn::Config const *config = GpioIn::GetConfig(pin);
    FW_ASSERT(config);
    Evt *evt = new Evt(GpioIn::TRIGGER, config->hsmn, HSM_UNDEF, counter++);
    Fw::Post(evt);
    if (config->tracking) {
        DisableGpioInt(pin);
    }
}

bool GpioIn::IsActive() {
    return (HAL_GPIO_ReadPin(m_config->port, m_config->pin) == (m_config->activeHigh ? GPIO_PIN_SET : GPIO_PIN_RESET));
}

GpioIn::GpioIn() :
    Region((QStateHandler)&GpioIn::InitialPseudoState, GetCurrHsmn(), GetCurrName()),
    m_config(NULL), m_client(HSM_UNDEF), m_filterGlitch(true), m_holdCount(0),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_pulseTimer(GetHsmn(), PULSE_TIMER),
    m_holdTimer(GetHsmn(), HOLD_TIMER) {
    SET_EVT_NAME(GPIO_IN);
    uint32_t i;
    for (i = 0; i < ARRAY_COUNT(CONFIG); i++) {
        if (CONFIG[i].hsmn == GetHsmn()) {
            m_config = &CONFIG[i];
            break;
        }
    }
    FW_ASSERT(i < ARRAY_COUNT(CONFIG));
    // Save config to pin mapping.
    SaveConfig(m_config->pin, m_config);
    IncCurrHsmn();
}

QState GpioIn::InitialPseudoState(GpioIn * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&GpioIn::Root);
}

QState GpioIn::Root(GpioIn * const me, QEvt const * const e) {
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
            return Q_TRAN(&GpioIn::Stopped);
        }
        case GPIO_IN_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GpioInStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState GpioIn::Stopped(GpioIn * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case GPIO_IN_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GpioInStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case GPIO_IN_START_REQ: {
            EVENT(e);
            GpioInStartReq const &req = static_cast<GpioInStartReq const &>(*e);
            me->m_client = req.GetFrom();
            me->m_filterGlitch = req.IsFilterGlitch();
            me->SendCfm(new GpioInStartCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&GpioIn::Started);
        }
    }
    return Q_SUPER(&GpioIn::Root);
}

QState GpioIn::Started(GpioIn * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("Instance = %d", GetInst(me->GetHsmn()));
            me->InitGpio();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            DisableGpioInt(me->m_config->pin);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            if (me->m_config->tracking) {
                return Q_TRAN(&GpioIn::TrackMode);
            }
            return Q_TRAN(&GpioIn::TriggerMode);
        }
        case GPIO_IN_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GpioInStopCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&GpioIn::Stopped);
        }
    }
    return Q_SUPER(&GpioIn::Root);
}

QState GpioIn::TriggerMode(GpioIn * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case TRIGGER: {
            EVENT(e);
            me->Send(new GpioInActiveInd(), me->m_client);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GpioIn::Started);
}

QState GpioIn::TrackMode(GpioIn * const me, QEvt const * const e) {
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
            if (me->IsActive()) {
                return Q_TRAN(&GpioIn::Active);
            }
            return Q_TRAN(&GpioIn::Inactive);
        }
    }
    return Q_SUPER(&GpioIn::Started);
}

QState GpioIn::Inactive(GpioIn * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->Send(new GpioInInactiveInd(), me->m_client);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case TRIGGER: {
            EVENT(e);
            EnableGpioInt(me->m_config->pin);
            if (me->IsActive()) {
                me->Raise(new Evt(PIN_ACTIVE));
            } else if (!me->m_filterGlitch) {
                me->Raise(new Evt(SELF));
            }
            return Q_HANDLED();
        }
        case PIN_ACTIVE: {
            return Q_TRAN(&GpioIn::Active);
        }
        case SELF: {
            return Q_TRAN(&GpioIn::Inactive);
        }
    }
    return Q_SUPER(&GpioIn::TrackMode);;
}

QState GpioIn::Active(GpioIn * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_holdCount = 0;
            me->Send(new GpioInActiveInd(), me->m_client);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&GpioIn::PulseWait);
        }
        case TRIGGER: {
            EVENT(e);
            EnableGpioInt(me->m_config->pin);
            if (!me->IsActive()) {
                me->Raise(new Evt(PIN_INACTIVE));
            } else if (!me->m_filterGlitch) {
                me->Raise(new Evt(SELF));
            }
            return Q_HANDLED();
        }
        case HOLD_TIMER: {
            return Q_TRAN(&GpioIn::Held);
        }
        case PIN_INACTIVE: {
            return Q_TRAN(&GpioIn::Inactive);
        }
        case SELF: {
            return Q_TRAN(&GpioIn::Active);
        }
    }
    return Q_SUPER(&GpioIn::TrackMode);
}

QState GpioIn::PulseWait(GpioIn * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_pulseTimer.Start(PULSE_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_pulseTimer.Stop();
            return Q_HANDLED();
        }
        case PULSE_TIMER: {
            return Q_TRAN(&GpioIn::HoldWait);
        }
    }
    return Q_SUPER(&GpioIn::Active);
}


QState GpioIn::HoldWait(GpioIn * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_holdTimer.Start(HOLD_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_holdTimer.Stop();
            return Q_HANDLED();
        }
        case PIN_INACTIVE: {
            me->Send(new GpioInPulseInd(), me->m_client);
            return Q_TRAN(&GpioIn::Inactive);
        }
    }
    return Q_SUPER(&GpioIn::Active);
}

QState GpioIn::Held(GpioIn * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->Send(new GpioInHoldInd(++me->m_holdCount), me->m_client);
            me->m_holdTimer.Start(HOLD_TIMEOUT_MS);
            // For test only to trigger hardfault in debugger.
            //*(uint32_t *)(0xD0000000) = 0x1234;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_holdTimer.Stop();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&GpioIn::Active);
}

/*
QState GpioIn::MyState(GpioIn * const me, QEvt const * const e) {
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
            return Q_TRAN(&GpioIn::SubState);
        }
    }
    return Q_SUPER(&GpioIn::SuperState);
}
*/

} // namespace APP
