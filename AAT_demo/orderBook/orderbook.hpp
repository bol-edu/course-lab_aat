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

#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "hls_stream.h"
#include "ap_int.h"
#include "aat_defines.hpp"
#include "aat_interfaces.hpp"

#define OB_DM_RING_BUF_LEN (65536)

// OrderBook control
#define OB_DM_FWD_ENABLE  (1<<3)
#define OB_RESET_COUNT    (1<<2)
#define OB_RESET_DATA     (1<<1)
#define OB_HALT           (1<<0)
#define OB_CAPTURE_FREEZE (1<<31)

// OrderBookDataMover control
#define OB_DM_RTT_RESET  (1<<2)
#define OB_DM_RTT_ENABLE (1<<1)
#define OB_DM_HALT       (1<<0)

typedef struct orderBookRegControl_t
{
    ap_uint<32> control;
    ap_uint<32> config;
    ap_uint<32> capture;
    ap_uint<32> reserved03;
    ap_uint<32> reserved04;
    ap_uint<32> reserved05;
    ap_uint<32> reserved06;
    ap_uint<32> reserved07;
} orderBookRegControl_t;

typedef struct orderBookRegStatus_t
{
    ap_uint<32> status;
    ap_uint<32> rxOperation;
    ap_uint<32> processOperation;
    ap_uint<32> invalidOperation;
    ap_uint<32> generateResponse;
    ap_uint<32> txResponse;
    ap_uint<32> addOperation;
    ap_uint<32> modifyOperation;
    ap_uint<32> deleteOperation;
    ap_uint<32> transactOperation;
    ap_uint<32> haltOperation;
    ap_uint<32> timestampError;
    ap_uint<32> operationError;
    ap_uint<32> symbolError;
    ap_uint<32> directionError;
    ap_uint<32> levelError;
    ap_uint<32> rxEvent;
    ap_uint<32> reserved17;
    ap_uint<32> reserved18;
    ap_uint<32> reserved19;
    ap_uint<32> reserved20;
    ap_uint<32> reserved21;
    ap_uint<32> reserved22;
    ap_uint<32> reserved23;
} orderBookRegStatus_t;

typedef struct orderBookDataMoverRegControl_t
{
    ap_uint<32> control;
    ap_uint<32> indexTxHead;
    ap_uint<32> indexRxTail;
    ap_uint<32> rxThrottleRate;
    ap_uint<32> reserved04;
    ap_uint<32> reserved05;
    ap_uint<32> reserved06;
    ap_uint<32> reserved07;
} orderBookDataMoverRegControl_t;

typedef struct orderBookDataMoverRegStatus_t
{
    ap_uint<32> status;
    ap_uint<32> indexTxTail;
    ap_uint<32> txResponse;
    ap_uint<32> indexRxHead;
    ap_uint<32> rxOperation;
    ap_uint<32> latencyMin;
    ap_uint<32> latencyMax;
    ap_uint<32> latencySum;
    ap_uint<32> latencyCount;
    ap_uint<32> cyclesPre;
    ap_uint<32> cyclesPost;
    ap_uint<32> rxThrottleCount;
    ap_uint<32> rxThrottleEvent;
    ap_uint<32> reserved13;
    ap_uint<32> reserved14;
    ap_uint<32> reserved15;
} orderBookDataMoverRegStatus_t;

/**
 * OrderBook Core
 */
class OrderBook
{
public:

    void operationPull(ap_uint<32> &regRxOperation,
                       hls::stream<orderBookOperationPack_t> &operationStreamPack,
                       hls::stream<orderBookOperation_t> &operationStream);

    void operationProcess(ap_uint<32> &regProcessOperation,
                          ap_uint<32> &regInvalidOperation,
                          ap_uint<32> &regGenerateResponse,
                          ap_uint<32> &regAddOperation,
                          ap_uint<32> &regModifyOperation,
                          ap_uint<32> &regDeleteOperation,
                          ap_uint<32> &regTransactOperation,
                          ap_uint<32> &regHaltOperation,
                          ap_uint<32> &regTimestampError,
                          ap_uint<32> &regOperationError,
                          ap_uint<32> &regSymbolError,
                          ap_uint<32> &regDirectionError,
                          ap_uint<32> &regLevelError,
                          hls::stream<orderBookOperation_t> &operationStream,
                          hls::stream<orderBookResponse_t> &responseStream);

    unsigned int queryPriceLevel(ap_uint<8> symbolIndex,
                                 ap_uint<32> price,
                                 ap_uint<8> direction,
                                 ap_int<8> level);

    void operationAdd(ap_uint<8> symbolIndex,
                      ap_uint<32> orderCount,
                      ap_uint<32> quantity,
                      ap_uint<32> price,
                      ap_uint<8> direction,
                      ap_int<8> level);

    void operationModify(ap_uint<8> symbolIndex,
                         ap_uint<32> orderCount,
                         ap_uint<32> quantity,
                         ap_uint<32> price,
                         ap_uint<8> direction,
                         ap_int<8> level);

    void operationDelete(ap_uint<8> symbolIndex,
                         ap_uint<32> orderCount,
                         ap_uint<32> quantity,
                         ap_uint<32> price,
                         ap_uint<8> direction,
                         ap_int<8> level);

    void operationTransactVisible(ap_uint<8> symbolIndex,
                                  ap_uint<32> orderCount,
                                  ap_uint<32> quantity,
                                  ap_uint<32> price,
                                  ap_uint<8> direction,
                                  ap_int<8> level);

    void operationTransactHidden(ap_uint<8> symbolIndex,
                                 ap_uint<32> orderCount,
                                 ap_uint<32> quantity,
                                 ap_uint<32> price,
                                 ap_uint<8> direction,
                                 ap_int<8> level);

    void operationHalt(void);

    void responseDump(ap_uint<1024> responseDump[1],
                      hls::stream<orderBookResponsePack_t> &responseStreamPack);

    void responsePush(ap_uint<32> &regControl,
                      ap_uint<32> &regCaptureControl,
                      ap_uint<32> &regTxResponse,
                      ap_uint<1024> &regCaptureBuffer,
                      hls::stream<orderBookResponse_t> &responseStream,
                      hls::stream<orderBookResponsePack_t> &responseStreamPack,
                      hls::stream<orderBookResponsePack_t> &dataMoveStreamPack);

    void responseMove(ap_uint<32> &regControl,
                      ap_uint<32> &regIndexHead,
                      ap_uint<32> &regIndexTail,
                      ap_uint<32> &regTxResponse,
                      ap_uint<32> &regCyclesPre,
                      ap_uint<1024> ringBuffer[OB_DM_RING_BUF_LEN],
                      hls::stream<orderBookResponsePack_t> &responseStreamPack);

    void operationMove(ap_uint<32> &regControl,
                       ap_uint<32> &regIndexTail,
                       ap_uint<32> &regRxThrottleRate,
                       ap_uint<32> &regIndexHead,
                       ap_uint<32> &regRxOperation,
                       ap_uint<32> &regLatencyMin,
                       ap_uint<32> &regLatencyMax,
                       ap_uint<32> &regLatencySum,
                       ap_uint<32> &regLatencyCount,
                       ap_uint<32> &regCyclesPost,
                       ap_uint<32> &rxThrottleCount,
                       ap_uint<32> &rxThrottleEvent,
                       ap_uint<256> ringBuffer[OB_DM_RING_BUF_LEN],
                       hls::stream<orderEntryOperationPack_t> &operationStreamPack);

private:

    // TODO: DCA-1273 flat or array for book data storage?
    ap_uint<160> orderBookBidCount[NUM_SYMBOL]={0};
    ap_uint<160> orderBookBidPrice[NUM_SYMBOL]={0};
    ap_uint<160> orderBookBidQuantity[NUM_SYMBOL]={0};
    ap_uint<160> orderBookAskCount[NUM_SYMBOL]={0};
    ap_uint<160> orderBookAskPrice[NUM_SYMBOL]={0};
    ap_uint<160> orderBookAskQuantity[NUM_SYMBOL]={0};

};

#endif
