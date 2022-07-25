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

#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>
#include "fw_macro.h"
#include "fw_assert.h"

#define COLORS_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("Colors.h", (int)__LINE__))

namespace APP {

// Helper class for color format (888 xBGR).
class Color888Bgr {
public:
    Color888Bgr(uint32_t color) : m_color(color) {}
    void Set(uint32_t color) { m_color = color; }
    void Set(uint8_t red, uint8_t green, uint8_t blue) {
        m_color = BYTE_TO_LONG(0, blue, green, red);
    }
    uint32_t Get() const { return m_color; }
    uint8_t Red() const { return BYTE_0(m_color); }
    uint8_t Green() const { return BYTE_1(m_color); }
    uint8_t Blue() const { return BYTE_2(m_color); }
    bool operator==(Color888Bgr const &c) {
        return m_color == c.Get();
    }
    bool operator!=(Color888Bgr const &c) {
        return m_color != c.Get();
    }
private:
    uint32_t m_color;   // (888 xBGR).
};

}

#endif // GRAPHICS_H
