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

#ifndef ORDERENTRY_H
#define ORDERENTRY_H

#include "hls_stream.h"
#include "ap_int.h"
#include "aat_defines.hpp"
#include "aat_interfaces.hpp"

#define OE_MSG_LEN_BYTES  (256)
#define OE_MSG_WORD_BYTES (8)
#define OE_MSG_NUM_FRAME  (OE_MSG_LEN_BYTES/OE_MSG_WORD_BYTES)

#define OE_HALT           (1<<0)
#define OE_RESET_DATA     (1<<1)
#define OE_RESET_COUNT    (1<<2)
#define OE_TCP_CONNECT    (1<<3)
#define OE_TCP_GEN_SUM    (1<<4)
#define OE_CAPTURE_FREEZE (1<<31)

typedef struct orderEntryRegControl_t
{
    ap_uint<32> control;
    ap_uint<32> config;
    ap_uint<32> capture;
    ap_uint<32> destAddress;
    ap_uint<32> destPort;
    ap_uint<32> reserved05;
    ap_uint<32> reserved06;
    ap_uint<32> reserved07;
} orderEntryRegControl_t;

typedef struct orderEntryRegStatus_t
{
    ap_uint<32> status;
    ap_uint<32> rxOperation;
    ap_uint<32> processOperation;
    ap_uint<32> txData;
    ap_uint<32> txMeta;
    ap_uint<32> txOrder;
    ap_uint<32> rxData;
    ap_uint<32> rxMeta;
    ap_uint<32> rxEvent;
    ap_uint<32> txDrop;
    ap_uint<32> txStatus;
    ap_uint<32> notification;
    ap_uint<32> readRequest;
    ap_uint<32> debug;
    ap_uint<32> reserved14;
    ap_uint<32> reserved15;
} orderEntryRegStatus_t;

typedef struct connectionStatus_t
{
    ap_uint<1> connected;
    ap_uint<16> length;
    ap_uint<30> space;
    ap_uint<2> error;
    ap_uint<16> sessionID;
} connectionStatus_t;

/**
 * OrderEntry Core
 */
class OrderEntry
{
public:

    void operationPull(ap_uint<32> &regRxOperation,
                       hls::stream<orderEntryOperationPack_t> &operationStreamPack,
                       hls::stream<orderEntryOperationPack_t> &operationHostStreamPack,
                       hls::stream<orderEntryOperation_t> &operationStream);

    void operationEncode(hls::stream<orderEntryOperation_t> &operationStream,
                         hls::stream<orderEntryOperationEncode_t> &operationEncodeStream);

    void openListenPortTcp(hls::stream<ipTcpListenPortPack_t> &listenPortStream,
                           hls::stream<ipTcpListenStatusPack_t> &listenStatusStream);

    void openActivePortTcp(ap_uint<32> &regControl,
                           ap_uint<32> &regDestAddress,
                           ap_uint<32> &regDestPort,
                           ap_uint<32> &regDebug,
                           hls::stream<ipTuplePack_t> &openConnectionStream,
                           hls::stream<ipTcpConnectionStatusPack_t> &connectionStatusStream,
                           hls::stream<ipTcpCloseConnectionPack_t> &closeConnectionStream,
                           hls::stream<ipTcpTxStatusPack_t> &txStatusStream);

    void notificationHandlerTcp(ap_uint<32> &regNotification,
                                ap_uint<32> &regReadRequest,
                                hls::stream<ipTcpNotificationPack_t> &notificationStream,
                                hls::stream<ipTcpReadRequestPack_t> &readRequestStream);

    void serverProcessTcp(ap_uint<32> &regRxData,
                          ap_uint<32> &regRxMeta,
                          hls::stream<ipTcpRxMetaPack_t> &rxMetaStream,
                          hls::stream<ipTcpRxDataPack_t> &rxDataStream);

    void operationProcessTcp(ap_uint<32> &regControl,
                             ap_uint<32> &regCaptureControl,
                             ap_uint<32> &regProcessOperation,
                             ap_uint<32> &regTxOrder,
                             ap_uint<32> &regTxData,
                             ap_uint<32> &regTxMeta,
                             ap_uint<32> &regTxStatus,
                             ap_uint<32> &regTxDrop,
                             ap_uint<1024> &regCaptureBuffer,
                             hls::stream<orderEntryOperationEncode_t> &operationEncodeStream,
                             hls::stream<ipTcpTxMetaPack_t> &txMetaStream,
                             hls::stream<ipTcpTxDataPack_t> &txDataStream);


private:

    connectionStatus_t mConnectionStatus;

    // we store a template for the egress message here and populate the dynamic
    // fields such as price and quantity before we transmit, the partial sum
    // for the template message is pre-computed, a potential improvement for
    // reduced manual maintenance would be to calculate at compile time
    ap_uint<16> messageTemplateSum = 0x1b65;

    ap_uint<64> messageTemplate[OE_MSG_NUM_FRAME] =
    {
        0x383d4649582e342e,
        0x325e393d3133355e,
        0x33353d445e33343d,
        0x0000000000000000, // sequence (MSB)
        0x00005e34393d4142, // sequence (LSB)
        0x433132334e5e3530,
        0x3d58465f46494e54,
        0x4543485e35323d32,
        0x303139303832382d,
        0x0000000000000000, // timestamp
        0x5e35363d434d455e,
        0x35373d475e313432,
        0x3d49455e5e33353d,
        0x445e313d584c4e58,
        0x3132333435363738,
        0x5e31313d00000000, // orderId (MSB)
        0x0000000000005e33, // orderId (LSB)
        0x383d000000000000, // quantity (MSB)
        0x000000005e34303d, // quantity (LSB)
        0x325e34343d000000, // price (MSB)
        0x000000000000005e, // price (LSB)
        0x35343d315e35353d,
        0x584c4e585e36303d,
        0x3230313930383238,
        0x2d31303a31313a31,
        0x325e313032383d4e,
        0x5e3130373d43455a,
        0x392043393337355e,
        0x3230343d305e3937,
        0x30323d315e5e3130,
        0x3d43484b2e2e2e2e,
        0x2e2e2e2e2e2e2e2e,
    };

    ap_uint<64> byteReverse(ap_uint<64> inputData);

    ap_uint<80> uint32ToAscii(ap_uint<32> inputData);

    void bcdDigitiser(ap_uint<1> &carryIn,
                      ap_uint<4> &bcdDigit,
                      ap_uint<4> &bcdNext,
                      ap_uint<1> &carryOut);

};

#endif
