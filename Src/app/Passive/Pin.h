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

#ifndef PIN_H
#define PIN_H

#include "qpcpp.h"
#include "periph.h"

namespace APP {

// Passive object to represent a single or a set of GPIO pins.
class Pin
{
public:
    Pin() : m_port(NULL), m_pin(0) {}
    ~Pin() {}
    void Init(GPIO_TypeDef *port,  uint16_t pin, uint32_t mode = GPIO_MODE_OUTPUT_PP, uint32_t pull = GPIO_PULLUP,
              uint32_t af = 0, uint32_t speed = GPIO_SPEED_FREQ_VERY_HIGH);
    void DeInit();
    // Critical section is not required for the following functions since they use atomic pin-wise set/clear registers.
    // However if a single pin is shared by multiple threads, external critical section may be required.
    // Single-pin operations.
    void Set()   const  { HAL_GPIO_WritePin(m_port, m_pin, GPIO_PIN_SET); }
    void Clear() const  { HAL_GPIO_WritePin(m_port, m_pin, GPIO_PIN_RESET); }
    void Toggle() const { HAL_GPIO_TogglePin(m_port, m_pin); }
    bool IsSet() const { return HAL_GPIO_ReadPin(m_port, m_pin) == GPIO_PIN_SET; }
    // Multi-pin operations.
    void Write(uint32_t data) const { m_port->ODR = (m_port->ODR & ~m_pin) | (data & m_pin); }
    uint32_t Read() const { return m_port->IDR & m_pin; }

protected:
    GPIO_TypeDef * m_port;
    uint16_t       m_pin;
};

}

#endif // PIN_H
