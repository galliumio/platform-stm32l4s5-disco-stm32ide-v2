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

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include "fw_assert.h"

#define GRAPHICS_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("Graphics.h", (int)__LINE__))

namespace APP {

// x is horizontal and y is vertical, with origin at the top-left corner.
class Point {
public:
    Point(uint32_t const x, uint32_t const y) : m_x(x), m_y(y) {}
    uint32_t X() const { return m_x; }
    uint32_t Y() const { return m_y; }
    // use built-in memberwise constructor and assignment operator
private:
    uint32_t m_x;
    uint32_t m_y;
};

class Area {
public:
    // (x, y) is the starting point at the upper left corner.
    Area(uint32_t x = 0, uint32_t y = 0, uint32_t width = 0, uint32_t height = 0) :
        m_x(x), m_y(y), m_width(width), m_height(height) {}
    uint32_t GetX() const { return m_x; }
    uint32_t GetY() const { return m_y; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    void Clear() {
        m_x = 0;
        m_y = 0;
        m_width = 0;
        m_height = 0;
    }
    bool IsEmpty() const {
        return (m_width == 0) || (m_height == 0);
    }
    Area &operator+=(Area const &a) {
        if (!a.IsEmpty()) {
            if (IsEmpty()) {
                *this = a;
            } else {
                m_x = LESS(m_x, a.GetX());
                m_y = LESS(m_y, a.GetY());
                uint32_t end = m_x + m_width;
                uint32_t aEnd = a.GetX() + a.GetWidth();
                m_width = GREATER(end, aEnd) - m_x;
                end = m_y + m_height;
                aEnd = a.GetY() + a.GetHeight();
                m_height = GREATER(end, aEnd) - m_y;
            }
        }
        return *this;
    }
    bool operator==(Area const &a) const {
        return (m_x == a.m_x) && (m_y == a.m_y) &&
               (m_width == a.m_width) && (m_height == a.m_height);
    }
    // @todo Add IsInclude().
private:
    uint32_t m_x;
    uint32_t m_y;
    uint32_t m_width;
    uint32_t m_height;
};

}

#endif // GRAPHICS_H
