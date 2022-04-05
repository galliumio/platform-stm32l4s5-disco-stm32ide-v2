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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "MotorInterface.h"
#include "Motor.h"

// Must enable one of the following.
//#define PWM_ON_EN
#define PWM_ON_IN2

#if (defined(PWM_ON_IN2) && defined(PWM_ON_EN))
#error PWM_ON_IN2 and PWM_ON_EN cannot be both defined
#endif
#if (!defined(PWM_ON_IN2) && !defined(PWM_ON_EN))
#error PWM_ON_IN2 or PWM_ON_EN must be defined
#endif

FW_DEFINE_THIS_FILE("Motor.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "MOTOR_TIMER_EVT_START",
    MOTOR_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "MOTOR_INTERNAL_EVT_START",
    MOTOR_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "MOTOR_INTERFACE_EVT_START",
    MOTOR_INTERFACE_EVT
};


// Define GPIO output pin configurations.
Motor::Config const Motor::CONFIG[] = {
#ifdef PWM_ON_EN
    { MOTOR, GPIOB, GPIO_PIN_2, GPIOA, GPIO_PIN_4, GPIOB, GPIO_PIN_1, GPIO_AF2_TIM3, TIM3, TIM_CHANNEL_4, false },
#endif
#ifdef PWM_ON_IN2
    { MOTOR, GPIOB, GPIO_PIN_2, GPIOB, GPIO_PIN_1, GPIOA, GPIO_PIN_4, GPIO_AF2_TIM3, TIM3, TIM_CHANNEL_4, false },
#endif
};

void Motor::InitGpio() {
    // Clock has been initialized by System via Periph.
    m_in1Pin.Init(m_config->in1Port, m_config->in1Pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, 0, GPIO_SPEED_FREQ_MEDIUM);
#ifdef PWM_ON_EN
    m_in2Pin.Init(m_config->in2Port, m_config->in2Pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, 0, GPIO_SPEED_FREQ_MEDIUM);
    m_enPin.Init(m_config->enPort, m_config->enPin, GPIO_MODE_AF_PP, GPIO_PULLUP, m_config->pwmAf, GPIO_SPEED_FREQ_MEDIUM);
#endif
#ifdef PWM_ON_IN2
    m_in2Pin.Init(m_config->in2Port, m_config->in2Pin, GPIO_MODE_AF_PP, GPIO_PULLUP, m_config->pwmAf, GPIO_SPEED_FREQ_MEDIUM);
    m_enPin.Init(m_config->enPort, m_config->enPin, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, 0, GPIO_SPEED_FREQ_MEDIUM);
#endif
}

void Motor::DeInitGpio() {
    StopPwm();
    m_in1Pin.DeInit();
    m_in2Pin.DeInit();
    m_enPin.DeInit();
}

void Motor::ConfigPwm(uint32_t levelPermil, bool activeHigh) {
    FW_ASSERT(levelPermil <= 1000);
    FW_ASSERT(m_hal);
    if (!activeHigh) {
        levelPermil = 1000 - levelPermil;
    }
    // Base PWM timer has been initialized by System via Periph.
    StopPwm();
    TIM_OC_InitTypeDef timConfig;
    memset(&timConfig, 0, sizeof(timConfig));
    timConfig.OCMode       = TIM_OCMODE_PWM1;
    timConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
    timConfig.OCFastMode   = TIM_OCFAST_DISABLE;
    timConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    timConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    timConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
    timConfig.Pulse        = (m_hal->Init.Period + 1) * levelPermil / 1000;
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    HAL_StatusTypeDef status = HAL_TIM_PWM_ConfigChannel(m_hal, &timConfig, m_config->pwmChannel);
    FW_ASSERT(status== HAL_OK);
    StartPwm();
    QF_CRIT_EXIT(crit);
}

void Motor::StartPwm() {
    FW_ASSERT(m_hal);
    HAL_StatusTypeDef status;
    if (m_config->pwmComplementary) {
        status = HAL_TIMEx_PWMN_Start(m_hal, m_config->pwmChannel);
    } else {
        status = HAL_TIM_PWM_Start(m_hal, m_config->pwmChannel);
    }
    FW_ASSERT(status == HAL_OK);
}


void Motor::StopPwm() {
    FW_ASSERT(m_hal);
    HAL_StatusTypeDef status;
    if (m_config->pwmComplementary) {
        status = HAL_TIMEx_PWMN_Stop(m_hal, m_config->pwmChannel);
    } else {
        status = HAL_TIM_PWM_Stop(m_hal, m_config->pwmChannel);
    }
    FW_ASSERT(status == HAL_OK);
}

void Motor::SetDirection() {
#ifdef PWM_ON_EN
    StopPwm();
    if (m_dir == MotorDir::FORWARD) {
        m_in1Pin.Clear();
        m_in2Pin.Set();
    } else {
        m_in1Pin.Set();
        m_in2Pin.Clear();
    }
#endif
#ifdef PWM_ON_IN2
    m_enPin.Clear();
    if (m_dir == MotorDir::FORWARD) {
        m_in1Pin.Clear();
        ConfigPwm(0, true);
    } else {
        m_in1Pin.Set();
        ConfigPwm(0, false);
    }
    m_enPin.Set();
#endif
}

void Motor::SetSpeed() {
#ifdef PWM_ON_EN
    ConfigPwm(m_speed, true);
#endif
#ifdef PWM_ON_IN2
    ConfigPwm(m_speed, m_dir == MotorDir::FORWARD);
#endif
}

void Motor::Disable() {
#ifdef PWM_ON_EN
    StopPwm();
    m_in1Pin.Clear();
    m_in2Pin.Clear();
#endif
#ifdef PWM_ON_IN2
    m_enPin.Clear();
    m_in1Pin.Clear();
    StopPwm();
#endif
}

// @param rate - Rate of change of speed in PWM permil level.
// @retrun Step size in PWM level per timeout period with a minimum of 1.
uint16_t Motor::CalStep(uint16_t rate) {
    FW_ASSERT((SPEED_TIMEOUT_MS > 0) && (SPEED_TIMEOUT_MS < 1000));
    uint16_t step = rate / (1000 / SPEED_TIMEOUT_MS);
    return (step > 0) ? step : 1;
}

Motor::Motor() :
    Active((QStateHandler)&Motor::InitialPseudoState, MOTOR, "MOTOR"),
    m_hal(NULL), m_inEvt(QEvt::STATIC_EVT), m_inMsg(QEvt::STATIC_EVT),
    m_dir(MotorDir::FORWARD), m_setSpeed(0), m_revSpeed(0), m_revAccel(0), m_speed(0), m_step(0),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_speedTimer(GetHsmn(), SPEED_TIMER),
    m_idleWaitTimer(GetHsmn(), IDLE_WAIT_TIMER) {
    SET_EVT_NAME(MOTOR);
    uint32_t i;
    for (i = 0; i < ARRAY_COUNT(CONFIG); i++) {
        if (CONFIG[i].hsmn == GetHsmn()) {
            m_config = &CONFIG[i];
            break;
        }
    }
    FW_ASSERT(i < ARRAY_COUNT(CONFIG));
    // Note - m_hal points to the PWM timer HAL owned by Periph, which is initialized by System upon startup.
    //        It will be retrieved from Periph upon MOTOR_START_REQ when it is guaranteed to be ready.
}

QState Motor::InitialPseudoState(Motor * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Motor::Root);
}

QState Motor::Root(Motor * const me, QEvt const * const e) {
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
            return Q_TRAN(&Motor::Stopped);
        }
        case MOTOR_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new MotorStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case MOTOR_RUN_REQ: {
            EVENT(e);
            auto const &req = static_cast<MotorRunReq const &>(*e);
            me->SendCfmMsg(new MotorRunCfm(MotorRunCfmMsg(MSG_ERROR_STATE, req.GetMsgTo())), req);
            return Q_HANDLED();
        }
        case MOTOR_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Motor::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Motor::Stopped(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case MOTOR_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new MotorStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case MOTOR_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&Motor::Starting);
        }
    }
    return Q_SUPER(&Motor::Root);
}

QState Motor::Starting(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = MotorStartReq::TIMEOUT_MS;
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
                me->SendCfm(new MotorStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new MotorStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Motor::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new MotorStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Motor::Started);
        }
    }
    return Q_SUPER(&Motor::Root);
}

QState Motor::Stopping(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = MotorStopReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            // For testing, send DONE immediately. Do not use Raise() in entry action.
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
        case MOTOR_STOP_REQ: {
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
            return Q_TRAN(&Motor::Stopped);
        }
    }
    return Q_SUPER(&Motor::Root);
}

QState Motor::Started(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_hal = Periph::GetHal(me->m_config->pwmTimer);
            me->InitGpio();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->Disable();
            me->DeInitGpio();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Motor::Idle);
        }
        case MOTOR_RUN_REQ: {
            EVENT(e);
            auto const &req = static_cast<MotorRunReq const &>(*e);
            me->SendCfmMsg(new MotorRunCfm(MotorRunCfmMsg(MSG_ERROR_PARAM, req.GetMsgTo())), req);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Root);
}

QState Motor::Idle(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_dir = MotorDir::FORWARD;
            me->m_setSpeed = 0;
            me->m_revSpeed = 0;
            me->m_speed = 0;
            me->m_step = 0;
            me->SetDirection();
            me->SetSpeed();
            return Q_HANDLED();
        }
        case MOTOR_RUN_REQ: {
            EVENT(e);
            MotorRunReq const &req = static_cast<MotorRunReq const &>(*e);
            if (me->IsValid(req.GetDir())) {
                me->m_setSpeed = LESS(req.GetSpeed(), (uint32_t)MAX_SPEED);
                me->m_dir = req.GetDir();
                me->m_step = me->CalStep(req.GetAccel());
                me->SetDirection();
                me->m_inMsg = req;
                return Q_TRAN(&Motor::Accel);
            }
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Started);
}

QState Motor::Running(Motor * const me, QEvt const * const e) {
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
            return Q_TRAN(&Motor::Accel);
        }
        case MOTOR_IDLE: {
            return Q_TRAN(&Motor::Idle);
        }
        case SPEED_UP: {
            return Q_TRAN(&Motor::Accel);
        }
        case SLOW_DOWN: {
            return Q_TRAN(&Motor::Slowing);
        }
        case REVERSE: {
            return Q_TRAN(&Motor::Reversing);
        }
        case HALT: {
            return Q_TRAN(&Motor::Halting);
        }
        case STABLE: {
            return Q_TRAN(&Motor::Const);
        }
        case MOTOR_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            me->m_setSpeed = 0;
            me->m_step = me->CalStep(BREAK_DECEL);
            me->Raise(new Evt(HALT));
            // Sends cfm to previous run request if exists.
            FW_ASSERT((!me->m_inMsg.InUse()) || (me->m_inMsg.sig == MOTOR_RUN_REQ));
            me->SendCfmMsg(new MotorRunCfm(MotorRunCfmMsg(MSG_ERROR_ABORT, me->m_inMsg.GetMsgTo())), me->m_inMsg);
            return Q_HANDLED();
        }
        case MOTOR_RUN_REQ: {
            EVENT(e);
            MotorRunReq const &req = static_cast<MotorRunReq const &>(*e);
            if (me->IsValid(req.GetDir())) {
                if (req.GetDir() != me->m_dir) {
                    me->m_revSpeed = LESS(req.GetSpeed(), (uint32_t)MAX_SPEED);
                    me->m_revAccel = req.GetAccel();
                    me->m_setSpeed = 0;
                    me->m_step = me->CalStep(req.GetDecel());
                    me->Raise(new Evt(REVERSE));
                } else {
                    me->m_setSpeed = LESS(req.GetSpeed(), (uint32_t)MAX_SPEED);
                    if (me->m_speed  < me->m_setSpeed) {
                        me->m_step = me->CalStep(req.GetAccel());
                        me->Raise(new Evt(SPEED_UP));
                    } else if (me->m_speed > me->m_setSpeed) {
                        me->m_step = me->CalStep(req.GetDecel());
                        me->Raise(new Evt(SLOW_DOWN));
                    } else {
                        me->Raise(new Evt(STABLE));
                    }
                }
                me->SendCfmMsg(new MotorRunCfm(MotorRunCfmMsg(MSG_ERROR_ABORT, me->m_inMsg.GetMsgTo())), me->m_inMsg);
                me->m_inMsg = req;
                return Q_HANDLED();
            }
            break;
        }
    }
    return Q_SUPER(&Motor::Started);
}

QState Motor::Accel(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_speedTimer.Start(SPEED_TIMEOUT_MS, Timer::PERIODIC);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_speedTimer.Stop();
            return Q_HANDLED();
        }
        case SPEED_TIMER: {
            EVENT(e);
            if(me->m_speed < me->m_setSpeed) {
                if ((me->m_setSpeed - me->m_speed) <= me->m_step) {
                    me->m_speed = me->m_setSpeed;
                } else {
                    if (me->m_speed < START_SPEED) {
                        me->m_speed = LESS(me->m_setSpeed, (uint32_t)START_SPEED);
                    } else {
                        me->m_speed += me->m_step;
                    }
                }
                me->m_speed += me->m_step,
                me->m_speed = LESS(me->m_speed, me->m_setSpeed);
                LOG("level(%c) = %d (target = %d)", me->m_dir == MotorDir::FORWARD ? 'f' : 'b',
                    me->m_speed, me->m_setSpeed);
                me->SetSpeed();
            } else {
                me->Raise(new Evt(STABLE));
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Running);
}

QState Motor::Decel(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_speedTimer.Start(SPEED_TIMEOUT_MS, Timer::PERIODIC);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_speedTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Motor::Slowing);
        }
        case SPEED_TIMER: {
            EVENT(e);
            if (me->m_speed > me->m_setSpeed) {
                if (((me->m_speed - me->m_setSpeed) <= me->m_step) ||
                    (me->m_speed < START_SPEED)) {
                    me->m_speed = me->m_setSpeed;
                } else {
                    me->m_speed -= me->m_step;
                }
                LOG("level(%c) = %d (target = %d)", me->m_dir == MotorDir::FORWARD ? 'f' : 'b',
                    me->m_speed, me->m_setSpeed);
                me->SetSpeed();
            } else {
                me->Raise(new Evt(STABLE));
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Running);
}

QState Motor::Slowing(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Decel);
}

QState Motor::Reversing(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            FW_ASSERT(me->m_setSpeed == 0);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case STABLE: {
            EVENT(e);
            FW_ASSERT(me->m_speed == 0);
            me->m_setSpeed = me->m_revSpeed;
            me->m_dir = (me->m_dir == MotorDir::FORWARD) ? MotorDir::BACKWARD : MotorDir::FORWARD;
            me->m_step = me->CalStep(me->m_revAccel);
            me->SetDirection();
            me->Raise(new Evt(SPEED_UP));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Decel);
}

QState Motor::Halting(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case MOTOR_STOP_REQ:
        case MOTOR_RUN_REQ:{
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Decel);
}

QState Motor::Const(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            FW_ASSERT(me->m_speed == me->m_setSpeed);
            me->SendCfmMsg(new MotorRunCfm(MotorRunCfmMsg(MSG_ERROR_SUCCESS)), me->m_inMsg);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            if (me->m_speed > 0) {
                return Q_TRAN(&Motor::Normal);
            } else {
                return Q_TRAN(&Motor::IdleWait);
            }
        }
    }
    return Q_SUPER(&Motor::Running);
}

QState Motor::Normal(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Const);
}

QState Motor::IdleWait(Motor * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_idleWaitTimer.Start(IDLE_WAIT_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_idleWaitTimer.Stop();
            return Q_HANDLED();
        }
        case IDLE_WAIT_TIMER: {
            EVENT(e);
            me->Raise(new Evt(MOTOR_IDLE));
            return Q_HANDLED();
        }
        case MOTOR_STOP_REQ:
        case MOTOR_RUN_REQ:{
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Motor::Const);
}

/*
QState Motor::MyState(Motor * const me, QEvt const * const e) {
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
            return Q_TRAN(&Motor::SubState);
        }
    }
    return Q_SUPER(&Motor::SuperState);
}
*/

} // namespace APP
