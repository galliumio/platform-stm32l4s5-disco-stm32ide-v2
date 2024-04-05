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

#ifndef TEST_PINS_H
#define TEST_PINS_H

#include "fw_macro.h"
#include "Pin.h"

#define TEST_PINS_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("TestPins.h", (int_t)__LINE__))

#define TEST_PIN_SET(x_)        APP::TestPins::GetInstance().Set(APP::TestPins::x_)
#define TEST_PIN_CLEAR(x_)      APP::TestPins::GetInstance().Clear(APP::TestPins::x_)
#define TEST_PIN_TOGGLE(x_)     APP::TestPins::GetInstance().Toggle(APP::TestPins::x_)

namespace APP {

// Passive object to represent a single or a set of GPIO pins.
class TestPins
{
public:
    enum Tp {
        TP_0,
        TP_1,
        TP_COUNT
    };
    static TestPins &GetInstance() {
        static TestPins t;
        return t;
    }
    // If a single test pin is shared by multiple threads, external critical section may be required.
    void Set(Tp tp) {
        TEST_PINS_ASSERT(tp < TP_COUNT);
        m_pins[tp].Set();
    }
    void Clear(Tp tp) {
        TEST_PINS_ASSERT(tp < TP_COUNT);
        m_pins[tp].Clear();
    }
    void Toggle(Tp tp) {
        TEST_PINS_ASSERT(tp < TP_COUNT);
        m_pins[tp].Toggle();
    }

protected:
    TestPins() {
        TEST_PINS_ASSERT(TP_COUNT == 2);
        m_pins[0].Init(GPIOA, GPIO_PIN_0);
        m_pins[1].Init(GPIOA, GPIO_PIN_1);
    }

    Pin m_pins[TP_COUNT];
};

}

#endif // TEST_PINS_H
