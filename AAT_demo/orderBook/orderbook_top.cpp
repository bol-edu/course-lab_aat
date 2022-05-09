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

extern "C" void orderBookTop(orderBookRegControl_t &regControl,
                             orderBookRegStatus_t &regStatus,
                             ap_uint<1024> &regCapture,
                             hls::stream<orderBookOperationPack_t> &operationStreamPack,
                             hls::stream<orderBookResponsePack_t> &responseStreamPack,
                             hls::stream<orderBookResponsePack_t> &dataMoveStreamPack){
								 
#pragma HLS INTERFACE s_axilite port=regControl bundle=control
#pragma HLS INTERFACE s_axilite port=regStatus bundle=control
#pragma HLS INTERFACE s_axilite port=regCapture bundle=control
#pragma HLS INTERFACE ap_none port=regControl
#pragma HLS INTERFACE ap_none port=regStatus
#pragma HLS INTERFACE ap_none port=regCapture
#pragma HLS INTERFACE axis port=operationStreamPack
#pragma HLS INTERFACE axis port=responseStreamPack
#pragma HLS INTERFACE axis port=dataMoveStreamPack
#pragma HLS INTERFACE ap_ctrl_none port=return

    static hls::stream<orderBookOperation_t> operationStreamFIFO;
    static hls::stream<orderBookResponse_t> responseStreamFIFO;

    static OrderBook kernel;

#pragma HLS DISAGGREGATE variable=regControl
#pragma HLS DISAGGREGATE variable=regStatus
#pragma HLS DATAFLOW disable_start_propagation

    kernel.operationPull(regStatus.rxOperation,
                         operationStreamPack,
                         operationStreamFIFO);

    kernel.operationProcess(regStatus.processOperation,
                            regStatus.invalidOperation,
                            regStatus.generateResponse,
                            regStatus.addOperation,
                            regStatus.modifyOperation,
                            regStatus.deleteOperation,
                            regStatus.transactOperation,
                            regStatus.haltOperation,
                            regStatus.timestampError,
                            regStatus.operationError,
                            regStatus.symbolError,
                            regStatus.directionError,
                            regStatus.levelError,
                            operationStreamFIFO,
                            responseStreamFIFO);

    kernel.responsePush(regControl.control,
                        regControl.capture,
                        regStatus.txResponse,
                        regCapture,
                        responseStreamFIFO,
                        responseStreamPack,
                        dataMoveStreamPack);

}
