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

#ifndef UART_OUT_INTERFACE_H
#define UART_OUT_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "fw_pipe.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

// UART_OUT_WRITE_REQ is of type Evt (used by Log::Write() in fw_log.cpp).
#define UART_OUT_INTERFACE_EVT \
    ADD_EVT(UART_OUT_START_REQ) \
    ADD_EVT(UART_OUT_START_CFM) \
    ADD_EVT(UART_OUT_STOP_REQ) \
    ADD_EVT(UART_OUT_STOP_CFM) \
    ADD_EVT(UART_OUT_FAIL_IND) \
    ADD_EVT(UART_OUT_WRITE_REQ) \
    ADD_EVT(UART_OUT_WRITE_CFM) \
    ADD_EVT(UART_OUT_EMPTY_IND)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    UART_OUT_INTERFACE_EVT_START = INTERFACE_EVT_START(UART_OUT),
    UART_OUT_INTERFACE_EVT
};

enum {
    UART_OUT_REASON_UNSPEC = 0,
};

class UartOutStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    UartOutStartReq(Fifo *fifo, Hsmn client) :
        Evt(UART_OUT_START_REQ), m_fifo(fifo), m_client(client) {}
    Fifo *GetFifo() const { return m_fifo; }
    Hsmn GetClient() const { return m_client; }
private:
    Fifo *m_fifo;
    Hsmn m_client;
};

class UartOutStartCfm : public ErrorEvt {
public:
    UartOutStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(UART_OUT_START_CFM, error, origin, reason) {}
};

class UartOutStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 2100       // It has to be greater than UartOut::ACTIVE_TIMEOUT_MS since
                                // UartOutStopReq may be deferred in the Active state.
    };
    UartOutStopReq() :
        Evt(UART_OUT_STOP_REQ) {}
};

class UartOutStopCfm : public ErrorEvt {
public:
    UartOutStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(UART_OUT_STOP_CFM, error, origin, reason) {}
};

class UartOutFailInd : public ErrorEvt {
public:
    UartOutFailInd(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(UART_OUT_FAIL_IND, error, origin, reason) {}
};

class UartOutWriteCfm : public ErrorEvt {
public:
    UartOutWriteCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(UART_OUT_WRITE_CFM, error, origin, reason) {}
};

class UartOutEmptyInd : public Evt {
public:
    UartOutEmptyInd() :
        Evt(UART_OUT_EMPTY_IND) {}
    // For forwarding or sending reminder (to keep 'from' and 'seq' in the original event).
    UartOutEmptyInd(Hsmn to, Hsmn from = HSM_UNDEF, Sequence seq = 0) :
        Evt(UART_OUT_EMPTY_IND, to, from, seq) {}
};

} // namespace APP

#endif // UART_OUT_INTERFACE_H
