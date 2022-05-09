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

#include "feedhandler_kernels.hpp"

extern "C" void feedHandlerTop(feedHandlerRegControl_t &regControl,
                               feedHandlerRegStatus_t &regStatus,
                               regSymbolMapContainer_t &regSymbolMap,
                               ap_uint<256> &regCapture,
                               hls::stream<axiWordExt_t> &inputDataStream,
                               hls::stream<orderBookOperationPack_t> &operationStreamPack)
{
#pragma HLS INTERFACE s_axilite port=regControl bundle=control
#pragma HLS INTERFACE s_axilite port=regStatus bundle=control
#pragma HLS INTERFACE s_axilite port=regSymbolMap bundle=control
#pragma HLS INTERFACE s_axilite port=regCapture bundle=control
#pragma HLS INTERFACE ap_none port=regControl
#pragma HLS INTERFACE ap_none port=regStatus
#pragma HLS INTERFACE ap_none port=regSymbolMap
#pragma HLS INTERFACE ap_none port=regCapture
#pragma HLS INTERFACE axis port=inputDataStream depth=32
#pragma HLS INTERFACE axis port=operationStreamPack depth=32
#pragma HLS INTERFACE ap_ctrl_none port=return

    static hls::stream<axiWord_t> mdpDataFifo;
    static hls::stream<axiWord_t> fixMsgFifo;
    static hls::stream<templateId_t> templateIdFifo;
    static hls::stream<securityId_t> securityIdFifo;
    static hls::stream<orderBookOperation_t> operationFifo;

#pragma HLS STREAM variable=mdpDataFifo
#pragma HLS STREAM variable=fixMsgFifo
#pragma HLS STREAM variable=templateIdFifo
#pragma HLS STREAM variable=securityIdFifo
#pragma HLS STREAM variable=operationFifo

    static FeedHandler kernel;

#pragma HLS DISAGGREGATE variable=regControl
#pragma HLS DISAGGREGATE variable=regStatus
#pragma HLS STABLE variable=regSymbolMap
#pragma HLS DATAFLOW disable_start_propagation

    kernel.udpPacketHandler(regStatus.processWord,
                            regStatus.processPacket,
                            inputDataStream,
                            mdpDataFifo);

    kernel.binaryPacketHandler(regStatus.processBinary,
                               mdpDataFifo,
                               fixMsgFifo,
                               templateIdFifo);

    kernel.fixDecoderTop(regStatus.processFix,
                         fixMsgFifo,
                         templateIdFifo,
                         securityIdFifo,
                         operationFifo);

    kernel.symbolLookup(regControl.capture,
                        regStatus.txOperation,
                        regSymbolMap.symbols,
                        regCapture,
                        securityIdFifo,
                        operationFifo,
                        operationStreamPack);
}
