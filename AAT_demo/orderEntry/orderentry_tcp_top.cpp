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

#include "orderentry_kernels.hpp"

extern "C" void orderEntryTcpTop(orderEntryRegControl_t &regControl,
                                 orderEntryRegStatus_t &regStatus,
                                 ap_uint<1024> &regCapture,
                                 hls::stream<orderEntryOperationPack_t> &operationStreamPack,
                                 hls::stream<orderEntryOperationPack_t> &operationHostStreamPack,
                                 hls::stream<ipTcpListenPortPack_t> &listenPortStreamPack,
                                 hls::stream<ipTcpListenStatusPack_t> &listenStatusStreamPack,
                                 hls::stream<ipTcpNotificationPack_t> &notificationStreamPack,
                                 hls::stream<ipTcpReadRequestPack_t> &readRequestStreamPack,
                                 hls::stream<ipTcpRxMetaPack_t> &rxMetaStreamPack,
                                 hls::stream<ipTcpRxDataPack_t> &rxDataStreamPack,
                                 hls::stream<ipTuplePack_t> &openConnectionStreamPack,
                                 hls::stream<ipTcpConnectionStatusPack_t> &connectionStatusStreamPack,
                                 hls::stream<ipTcpCloseConnectionPack_t> &closeConnectionStreamPack,
                                 hls::stream<ipTcpTxMetaPack_t> &txMetaStreamPack,
                                 hls::stream<ipTcpTxDataPack_t> &txDataStreamPack,
                                 hls::stream<ipTcpTxStatusPack_t> &txStatusStreamPack)
{
#pragma HLS INTERFACE s_axilite port=regControl bundle=control
#pragma HLS INTERFACE s_axilite port=regStatus bundle=control
#pragma HLS INTERFACE s_axilite port=regCapture bundle=control
#pragma HLS INTERFACE ap_none port=regControl
#pragma HLS INTERFACE ap_none port=regStatus
#pragma HLS INTERFACE ap_none port=regCapture
#pragma HLS INTERFACE axis register port=operationStreamPack
#pragma HLS INTERFACE axis register port=operationHostStreamPack
#pragma HLS INTERFACE axis register port=listenPortStreamPack
#pragma HLS INTERFACE axis register port=listenStatusStreamPack
#pragma HLS INTERFACE axis register port=notificationStreamPack
#pragma HLS INTERFACE axis register port=readRequestStreamPack
#pragma HLS INTERFACE axis register port=rxMetaStreamPack
#pragma HLS INTERFACE axis register port=rxDataStreamPack
#pragma HLS INTERFACE axis register port=openConnectionStreamPack
#pragma HLS INTERFACE axis register port=connectionStatusStreamPack
#pragma HLS INTERFACE axis register port=closeConnectionStreamPack 
#pragma HLS INTERFACE axis register port=txMetaStreamPack depth=32
#pragma HLS INTERFACE axis register port=txDataStreamPack depth=32
#pragma HLS INTERFACE axis register port=txStatusStreamPack
#pragma HLS INTERFACE ap_ctrl_none port=return

    static hls::stream<orderEntryOperation_t> operationStreamFIFO;
    static hls::stream<orderEntryOperationEncode_t> operationEncodeStreamFIFO;
    static hls::stream<ipTcpTxStatus_t> txStatusStreamFIFO;
    static OrderEntry kernel;

#pragma HLS DISAGGREGATE variable=regControl
#pragma HLS DISAGGREGATE variable=regStatus
#pragma HLS DATAFLOW disable_start_propagation

    kernel.openListenPortTcp(listenPortStreamPack,
                             listenStatusStreamPack);

    kernel.openActivePortTcp(regControl.control,
                             regControl.destAddress,
                             regControl.destPort,
                             regStatus.debug,
                             openConnectionStreamPack,
                             connectionStatusStreamPack,
                             closeConnectionStreamPack,
                             txStatusStreamPack);

    kernel.operationPull(regStatus.rxOperation,
                         operationStreamPack,
                         operationHostStreamPack,
                         operationStreamFIFO);

    kernel.operationEncode(operationStreamFIFO,
                           operationEncodeStreamFIFO);

    kernel.operationProcessTcp(regControl.control,
                               regControl.capture,
                               regStatus.processOperation,
                               regStatus.txOrder,
                               regStatus.txData,
                               regStatus.txMeta,
                               regStatus.txStatus,
                               regStatus.txDrop,
                               regCapture,
                               operationEncodeStreamFIFO,
                               txMetaStreamPack,
                               txDataStreamPack);

    kernel.serverProcessTcp(regStatus.rxData,
                            regStatus.rxMeta,
                            rxMetaStreamPack,
                            rxDataStreamPack);

    kernel.notificationHandlerTcp(regStatus.notification,
                                  regStatus.readRequest,
                                  notificationStreamPack,
                                  readRequestStreamPack);

}
