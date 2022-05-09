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

#ifndef ORDERBOOK_KERNELS_H
#define ORDERBOOK_KERNELS_H

#include "orderbook.hpp"

extern "C" void orderBookTop(orderBookRegControl_t &regControl,
                             orderBookRegStatus_t &regStatus,
                             ap_uint<1024> &regCapture,
                             hls::stream<orderBookOperationPack_t> &operationStreamPack,
                             hls::stream<orderBookResponsePack_t> &responseStreamPack,
                             hls::stream<orderBookResponsePack_t> &dataMoveStreamPack);

extern "C" void orderBookDataMoverTop(orderBookDataMoverRegControl_t &regControl,
                                      orderBookDataMoverRegStatus_t &regStatus,
                                      ap_uint<1024> *ringBufferTx,
                                      ap_uint<256> *ringBufferRx,
                                      hls::stream<orderBookResponsePack_t> &responseStreamPack,
                                      hls::stream<orderEntryOperationPack_t> &operationStreamPack);

#endif
