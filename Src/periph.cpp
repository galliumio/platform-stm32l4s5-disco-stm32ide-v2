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

#include "qpcpp.h"
#include "fw_assert.h"
#include "periph.h"

FW_DEFINE_THIS_FILE("periph.cpp")

namespace APP {

TimHal Periph::m_timHalStor[MAX_TIM_COUNT];
TimHalMap Periph::m_timHalMap(m_timHalStor, ARRAY_COUNT(m_timHalStor), TimHal(NULL, NULL));

TIM_HandleTypeDef Periph::m_tim1Hal;
TIM_HandleTypeDef Periph::m_tim2Hal;
TIM_HandleTypeDef Periph::m_tim3Hal;

// Add more HAL handles here.

// Setup common peripherals for normal power mode.
// These common peripherals are shared among different HW blocks and cannot be setup individually
// USART1 - TX PB.6 DMA2 Channel 6 DMA_REQUEST_USART1_TX (This avoids conflict with I2C2)
//          RX PB.7 DMA2 Channel 7 DMA_REQUEST_USART1_RX (This avoids conflict with I2C2)

// (Adafruit Ili9341 SPI1 - SCK PA.5, MISO PA.6, MOSI PA.7, CS PA.2 D/CX PA.15
//                  Reset - PD.14 (Not connected)
//                  TX DMA2 Channel 4 DMA_REQUEST_SPI1_TX
//                  RX DMA2 Channel 3 DMA_REQUEST_SPI1_RX)
// External Ili9341 SPI1 - SCK PA.5, MISO PA.6, MOSI PA.7, CS PB.8 D/CX PA.3
//                  Reset - PD.14
// 2nd External Ili9341 - Shares all pins with first LCD except CS on PB.9
//                  TX DMA2 Channel 4 DMA_REQUEST_SPI1_TX
//                  RX DMA2 Channel 3 DMA_REQUEST_SPI1_RX)
//
// WIFI SPI3 - SCK PC.10, MISO PC.11 MOSI PC.12, CS PE.0 (DMA not used.)
// WIFI WAKEUP - PB.13
// WIFI RESET - PE.8
// WIFI CMD DATA RDY - PE.1
// Sensor I2C2 - SCL PB.10, SDA PB.11
//             - TX DMA1 Channel 4 DMA_REQUEST_I2C2_TX (not used)
//             - RX DMA1 Channel 5 DMA_REQUEST_I2C2_RX (not used)
// Sensor ACCEL GYRO INT - PD.11
// Sensor MAG DRDY - PC.8
// Sensor HUMID TEMP DRDY - PD.15
// Sensor PRESS INT - PD.10
// LED0 - PB.14 PWM TIM1 Channel 2
// User Button - PC.13
// Train Button A (Yellow) - PC.5
// Train Button B (White) - PC.4
// Hall-effect Sensor (GPIO) - PC.3
// Motor (PWM on IN1 and IN2)
// Motor IN1 (PWM) - PB.4 TIM3 Channel 1
// Motor IN2 (PWM) - PB.1 TIM3 Channel 4
// WS2812 (PWM) PA.15 TIM2 Channel 1
//              DMA2 Channel 5 DMA_REQUEST_TIM2_CH1
// Future ESP8266 (UART) TX PA.0 (Test Pin)
//                       RX PA.1 (Test Pin)
//
// TIM1 configuration:
// APB1CLK = HCLK -> TIM1CLK = HCLK = SystemCoreClock (See "clock tree" and "timer clock" in ref manual.)
#define TIM1CLK             (SystemCoreClock)   // 120MHz
#define TIM1_COUNTER_CLK    (20000000)          // 20MHz
#define TIM1_PWM_FREQ       (500)               // 500Hz, was 20kHz (20000)

// TIM3 configuration:
// APB1CLK = HCLK -> TIM1CLK = HCLK = SystemCoreClock (See "clock tree" and "timer clock" in ref manual.)
#define TIM3CLK             (SystemCoreClock)   // 120MHz
#define TIM3_COUNTER_CLK    (20000000)          // 20MHz
#define TIM3_PWM_FREQ       (15000)             // 15kHz

void Periph::SetupNormal() {
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();
    __HAL_RCC_CRC_CLK_ENABLE();     // Required by emwin.
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_DMAMUX1_CLK_ENABLE();

    // Initialize TIM1 for PWM (shared by LED0...).
    HAL_StatusTypeDef status;
    memset(&m_tim1Hal, 0, sizeof(m_tim1Hal));
    m_tim1Hal.Instance = TIM1;
    m_tim1Hal.Init.Prescaler = (TIM1CLK / TIM1_COUNTER_CLK) - 1;
    m_tim1Hal.Init.Period = (TIM1_COUNTER_CLK / TIM1_PWM_FREQ) - 1;
    m_tim1Hal.Init.ClockDivision = 0;
    m_tim1Hal.Init.CounterMode = TIM_COUNTERMODE_UP;
    m_tim1Hal.Init.RepetitionCounter = 0;
    status = HAL_TIM_PWM_Init(&m_tim1Hal);
    FW_ASSERT(status == HAL_OK);
    // Add timHandle to map.
    SetHal(TIM1, &m_tim1Hal);

    // Initialize TIM3 for PWM (shared by Motor...).
    memset(&m_tim3Hal, 0, sizeof(m_tim3Hal));
    m_tim3Hal.Instance = TIM3;
    m_tim3Hal.Init.Prescaler = (TIM3CLK / TIM3_COUNTER_CLK) - 1;
    m_tim3Hal.Init.Period = (TIM3_COUNTER_CLK / TIM3_PWM_FREQ) - 1;
    m_tim3Hal.Init.ClockDivision = 0;
    m_tim3Hal.Init.CounterMode = TIM_COUNTERMODE_UP;
    m_tim3Hal.Init.RepetitionCounter = 0;
    status = HAL_TIM_PWM_Init(&m_tim3Hal);
    FW_ASSERT(status == HAL_OK);
    // Add timHandle to map.
    SetHal(TIM3, &m_tim3Hal);
}

// Setup common peripherals for low power mode.
void Periph::SetupLowPower() {
    // TBD.
}

// Reset common peripherals to startup state.
void Periph::Reset() {
    HAL_TIM_PWM_DeInit(&m_tim1Hal);
    __HAL_RCC_DMAMUX1_CLK_DISABLE();
    __HAL_RCC_TIM3_CLK_DISABLE();
    __HAL_RCC_TIM1_CLK_DISABLE();
    __HAL_RCC_CRC_CLK_DISABLE();     // Required by emwin.
    __HAL_RCC_DMA2_CLK_DISABLE();
    __HAL_RCC_DMA1_CLK_DISABLE();
    __GPIOE_CLK_DISABLE();
    __GPIOD_CLK_DISABLE();
    __GPIOC_CLK_DISABLE();
    __GPIOB_CLK_DISABLE();
    __GPIOA_CLK_DISABLE();
    // TBD.
}

} // namespace APP
