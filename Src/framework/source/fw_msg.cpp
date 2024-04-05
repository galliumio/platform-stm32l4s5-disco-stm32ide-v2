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
#include "fw_msg.h"
#include "fw_assert.h"

FW_DEFINE_THIS_FILE("fw_msg.cpp")

using namespace QP;

namespace FW {


char const * ToMsgError(Error evtError) {
    switch(evtError) {
        case ERROR_SUCCESS:     return MSG_ERROR_SUCCESS;
        case ERROR_PENDING:     return MSG_ERROR_PENDING;
        case ERROR_UNSPEC:      return MSG_ERROR_UNSPEC;
        case ERROR_ABORTED:     return MSG_ERROR_ABORTED;
        case ERROR_TIMEOUT:     return MSG_ERROR_TIMEOUT;
        case ERROR_HAL:         return MSG_ERROR_HAL;
        case ERROR_HARDWARE:    return MSG_ERROR_HARDWARE;
        case ERROR_HSMN:        return MSG_ERROR_HSMN;
        case ERROR_STATE:       return MSG_ERROR_STATE;
        case ERROR_UNAVAIL:     return MSG_ERROR_UNAVAIL;
        case ERROR_PARAM:       return MSG_ERROR_PARAM;
        case ERROR_NETWORK:     return MSG_ERROR_NETWORK;
        case ERROR_AUTH:        return MSG_ERROR_AUTH;
        default:                break;
    }
    return MSG_ERROR_UNSPEC;
}

Error ToEvtError(char const *msgError) {
    if (STRING_EQUAL(msgError, MSG_ERROR_SUCCESS)) {
        return ERROR_SUCCESS;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_PENDING)) {
        return ERROR_PENDING;
    }    
    if (STRING_EQUAL(msgError, MSG_ERROR_UNSPEC)) {
        return ERROR_UNSPEC;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_ABORTED)) {
        return ERROR_ABORTED;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_TIMEOUT)) {
        return ERROR_TIMEOUT;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_HAL)) {
        return ERROR_HAL;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_HARDWARE)) {
        return ERROR_HARDWARE;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_HSMN)) {
        return ERROR_HSMN;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_STATE)) {
        return ERROR_STATE;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_UNAVAIL)) {
        return ERROR_UNAVAIL;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_PARAM)) {
        return ERROR_PARAM;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_NETWORK)) {
        return ERROR_NETWORK;
    }
    if (STRING_EQUAL(msgError, MSG_ERROR_AUTH)) {
        return ERROR_AUTH;
    }
    return ERROR_UNSPEC;
}

} // namespace FW
