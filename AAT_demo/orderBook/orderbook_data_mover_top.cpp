/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include "orderbook_kernels.hpp"

extern "C" void orderBookDataMoverTop(orderBookDataMoverRegControl_t &regControl,
                                      orderBookDataMoverRegStatus_t &regStatus,
                                      ap_uint<1024> *ringBufferTx,
                                      ap_uint<256> *ringBufferRx,
                                      hls::stream<orderBookResponsePack_t> &responseStreamPack,
                                      hls::stream<orderEntryOperationPack_t> &operationStreamPack)
{
#pragma HLS INTERFACE m_axi port=ringBufferTx offset=slave
#pragma HLS INTERFACE m_axi port=ringBufferRx offset=slave
#pragma HLS INTERFACE s_axilite port=regControl
#pragma HLS INTERFACE s_axilite port=regStatus
#pragma HLS INTERFACE ap_none port=regControl
#pragma HLS INTERFACE ap_none port=regStatus
#pragma HLS INTERFACE axis port=responseStreamPack
#pragma HLS INTERFACE axis port=operationStreamPack
#pragma HLS INTERFACE s_axilite port=return

    static OrderBook kernel;

#pragma HLS DISAGGREGATE variable=regControl
#pragma HLS DISAGGREGATE variable=regStatus

    // this ap_ctrl_hs kernel should run continuously when started, to avoid
    // an inner loop we can use auto_restart functionality, this is a control
    // bit set from host software as part of kernel startup sequence

    kernel.responseMove(regControl.control,
                        regControl.indexTxHead,
                        regStatus.indexTxTail,
                        regStatus.txResponse,
                        regStatus.cyclesPre,
                        ringBufferTx,
                        responseStreamPack);

    kernel.operationMove(regControl.control,
                         regControl.indexRxTail,
                         regControl.rxThrottleRate,
                         regStatus.indexRxHead,
                         regStatus.rxOperation,
                         regStatus.latencyMin,
                         regStatus.latencyMax,
                         regStatus.latencySum,
                         regStatus.latencyCount,
                         regStatus.cyclesPost,
                         regStatus.rxThrottleCount,
                         regStatus.rxThrottleEvent,
                         ringBufferRx,
                         operationStreamPack);

}
