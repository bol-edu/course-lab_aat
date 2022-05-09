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

#include "pricingengine_kernels.hpp"

extern "C" void pricingEngineTop(pricingEngineRegControl_t &regControl,
                                 pricingEngineRegStatus_t &regStatus,
                                 ap_uint<1024> &regCapture,
                                 pricingEngineRegStrategy_t regStrategies[NUM_SYMBOL],
                                 hls::stream<orderBookResponsePack_t> &responseStreamPack,
                                 hls::stream<orderEntryOperationPack_t> &operationStreamPack)
{
#pragma HLS INTERFACE s_axilite port=regControl bundle=control
#pragma HLS INTERFACE s_axilite port=regStatus bundle=control
#pragma HLS INTERFACE s_axilite port=regCapture bundle=control
#pragma HLS INTERFACE s_axilite port=regStrategies bundle=control
#pragma HLS INTERFACE ap_none port=regControl
#pragma HLS INTERFACE ap_none port=regStatus
#pragma HLS INTERFACE ap_memory port=regCapture
#pragma HLS INTERFACE ap_memory port=regStrategies
#pragma HLS INTERFACE axis port=responseStreamPack
#pragma HLS INTERFACE axis port=operationStreamPack
#pragma HLS INTERFACE ap_ctrl_none port=return

    static hls::stream<orderBookResponse_t> responseStreamFIFO;
    static hls::stream<orderEntryOperation_t> operationStreamFIFO;
    static PricingEngine kernel;
    static mmInterface intf;

#pragma HLS DISAGGREGATE variable=regControl
#pragma HLS DISAGGREGATE variable=regStatus
#pragma HLS STABLE variable=regStrategies
#pragma HLS DATAFLOW disable_start_propagation

    kernel.responsePull(regStatus.rxResponse,
                        responseStreamPack,
                        responseStreamFIFO);

    kernel.pricingProcess(regControl.strategy,
                          regStatus.processResponse,
                          regStatus.strategyNone,
                          regStatus.strategyPeg,
                          regStatus.strategyLimit,
                          regStatus.strategyUnknown,
                          regStrategies,
                          responseStreamFIFO,
                          operationStreamFIFO);

    kernel.operationPush(regControl.capture,
                         regStatus.txOperation,
                         regCapture,
                         operationStreamFIFO,
                         operationStreamPack);

}
