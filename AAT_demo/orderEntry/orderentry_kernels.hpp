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

#ifndef ORDERENTRY_KERNELS_H
#define ORDERENTRY_KERNELS_H

#include "orderentry.hpp"

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
                                 hls::stream<ipTcpTxStatusPack_t> &txStatusStreamPack);

#endif
