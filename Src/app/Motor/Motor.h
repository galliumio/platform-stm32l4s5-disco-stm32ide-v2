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

#ifndef MOTOR_H
#define MOTOR_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "MotorInterface.h"
#include "Pin.h"

using namespace QP;
using namespace FW;

namespace APP {

class Motor : public Active {
public:
    Motor();

protected:
    static QState InitialPseudoState(Motor * const me, QEvt const * const e);
    static QState Root(Motor * const me, QEvt const * const e);
        static QState Stopped(Motor * const me, QEvt const * const e);
        static QState Starting(Motor * const me, QEvt const * const e);
        static QState Stopping(Motor * const me, QEvt const * const e);
        static QState Started(Motor * const me, QEvt const * const e);
            static QState Idle(Motor * const me, QEvt const * const e);
            static QState Running(Motor * const me, QEvt const * const e);
                static QState Accel(Motor * const me, QEvt const * const e);
                static QState Decel(Motor * const me, QEvt const * const e);
                    static QState Slowing(Motor * const me, QEvt const * const e);
                    static QState Reversing(Motor * const me, QEvt const * const e);
                    static QState Halting(Motor * const me, QEvt const * const e);
                static QState Const(Motor * const me, QEvt const * const e);
                    static QState Normal(Motor * const me, QEvt const * const e);
                    static QState IdleWait(Motor * const me, QEvt const * const e);

    // GPIO control functions
    void InitGpio();
    void DeInitGpio();
    void ConfigPwm(uint32_t levelPermil, bool activeHigh = true);
    void StartPwm();
    void StopPwm();
    // Controls motor direction and speed.
    bool IsValid(MotorDir dir) {
        return dir < MotorDir::INVALID;
    }
    void SetDirection();
    void SetSpeed();
    void Disable();
    uint16_t CalStep(uint16_t rate);

    enum {
        SPEED_TIMEOUT_MS = 50,
        IDLE_WAIT_TIMEOUT_MS = 500,
    };

    enum {
        MAX_SPEED = 1000,
        START_SPEED = 100,
        BREAK_DECEL = 2000,   // Breaking deceleration (PWM level/s). 2000 means full speed to full stop in 0.5s.
    };

    // (IN1, IN2) pins control:
    // (0, 0) or (0, 1) to break.
    // (0, 1) Clockwise.
    // (1, 0) Counter Clockwise.
    // IN1, IN2 and EN (enable) pins
    typedef struct {
        Hsmn hsmn;
        GPIO_TypeDef *in1Port;
        uint16_t in1Pin;
        GPIO_TypeDef *in2Port;
        uint16_t in2Pin;
        GPIO_TypeDef *enPort;
        uint16_t enPin;
        // PWM timer config.
        uint32_t pwmAf;         // GPIO alternate function for timer.
        TIM_TypeDef *pwmTimer;
        uint32_t pwmChannel;
        bool pwmComplementary;
    } Config;
    static Config const CONFIG[];

    Config const *m_config;
    TIM_HandleTypeDef *m_hal;
    Pin m_in1Pin;
    Pin m_in2Pin;
    Pin m_enPin;
    Evt m_inEvt;                    // Static event copy of a generic incoming request event to be confirmed. Add more if needed.
    // @todo Remove.
    //Msg m_inMsg;                    // Saved base of a generic incoming request message to be confirmed.
    MsgBaseEvt m_inMsg;             // Static event copy of the base of an incoming request message to be confirmed.
    MotorDir m_dir;
    uint16_t m_setSpeed;            // Target speed.
    uint16_t m_revSpeed;            // Speed after reversing.
    uint16_t m_revAccel;            // Acceleration after reversing.
    uint16_t m_speed;               // PWM duty-cycle permil level (0-1000)
    uint16_t m_step;                // Increment or decrement in PWM level in each timer interval.
    Timer m_stateTimer;
    Timer m_speedTimer;
    Timer m_idleWaitTimer;

#define MOTOR_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(SPEED_TIMER) \
    ADD_EVT(IDLE_WAIT_TIMER)

#define MOTOR_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(MOTOR_IDLE) \
    ADD_EVT(SPEED_UP) \
    ADD_EVT(SLOW_DOWN) \
    ADD_EVT(HALT) \
    ADD_EVT(REVERSE) \
    ADD_EVT(STABLE)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        MOTOR_TIMER_EVT_START = TIMER_EVT_START(MOTOR),
        MOTOR_TIMER_EVT
    };

    enum {
        MOTOR_INTERNAL_EVT_START = INTERNAL_EVT_START(MOTOR),
        MOTOR_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // MOTOR_H
