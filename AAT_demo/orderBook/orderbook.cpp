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
#include "orderbook.hpp"

/**
 * OrderBook Core
 */
void OrderBook::operationPull(ap_uint<32> &regRxOperation,
                              hls::stream<orderBookOperationPack_t> &operationStreamPack,
                              hls::stream<orderBookOperation_t> &operationStream)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    orderBookOperationPack_t operationPack;
    orderBookOperation_t operation, operationExpected;

    static ap_uint<32> countRxOperation=0;

    if(!operationStreamPack.empty())
    {
        operationPack = operationStreamPack.read();
        intf.orderBookOperationUnpack(&operationPack, &operation);
        operationStream.write(operation);
        ++countRxOperation;
    }

    regRxOperation = countRxOperation;

    return;
}

void OrderBook::operationProcess(ap_uint<32> &regProcessOperation,
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
                                 hls::stream<orderBookResponse_t> &responseStream)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    orderBookOperation_t operation;
    orderBookResponse_t response;
    ap_uint<64> timestamp;
    ap_uint<32> orderId, orderCount, quantity, price;
    ap_uint<8> opCode, symbolIndex, direction;
    ap_int<8> level;

    static ap_uint<32> countProcessOperation=0;
    static ap_uint<32> countInvalidOperation=0;
    static ap_uint<32> countGenerateResponse=0;
    static ap_uint<32> countAddOperation=0;
    static ap_uint<32> countModifyOperation=0;
    static ap_uint<32> countDeleteOperation=0;
    static ap_uint<32> countTransactOperation=0;
    static ap_uint<32> countHaltOperation=0;
    static ap_uint<32> countTimestampError=0;
    static ap_uint<32> countOperationError=0;
    static ap_uint<32> countSymbolError=0;
    static ap_uint<32> countDirectionError=0;
    static ap_uint<32> countLevelError=0;

    if(!operationStream.empty())
    {
        operation = operationStream.read();
        ++countProcessOperation;

        timestamp = operation.timestamp;
        opCode = operation.opCode;
        symbolIndex = operation.symbolIndex;
        orderId = operation.orderId;
        orderCount = operation.orderCount;
        quantity = operation.quantity;
        price = operation.price;
        direction = operation.direction;
        level= operation.level;

        if(ORDERBOOK_ADD == opCode)
        {
            operationAdd(symbolIndex, orderCount, quantity, price, direction, level);
            ++countAddOperation;
        }
        else if(ORDERBOOK_MODIFY == opCode)
        {
            operationModify(symbolIndex, orderCount, quantity, price, direction, level);
            ++countModifyOperation;
        }
        else if(ORDERBOOK_DELETE == opCode)
        {
            operationDelete(symbolIndex, orderCount, quantity, price, direction, level);
            ++countDeleteOperation;
        }
        else if(ORDERBOOK_TRANSACT_VISIBLE == opCode)
        {
            operationTransactVisible(symbolIndex, orderCount, quantity, price, direction, level);
            ++countTransactOperation;
        }
        else if(ORDERBOOK_TRANSACT_HIDDEN == opCode)
        {
            operationTransactHidden(symbolIndex, orderCount, quantity, price, direction, level);
            ++countTransactOperation;
        }
        else if(ORDERBOOK_HALT == opCode)
        {
            operationHalt();
            ++countHaltOperation;
        }
        else
        {
            ++countInvalidOperation;
        }

        // generate a response for every operation, downstream filter can decide whether to publish
        // TODO: 56b timestamp to pack within 1024b total, to increase to 64b support may split
        //       bid/ask into separate response messages as book operation should hit one side only
        response.timestamp = timestamp.range(55,0);
        response.symbolIndex = symbolIndex;
        response.bidCount = orderBookBidCount[symbolIndex];
        response.bidPrice = orderBookBidPrice[symbolIndex];
        response.bidQuantity = orderBookBidQuantity[symbolIndex];
        response.askCount = orderBookAskCount[symbolIndex];
        response.askPrice = orderBookAskPrice[symbolIndex];
        response.askQuantity = orderBookAskQuantity[symbolIndex];

        responseStream.write(response);
        ++countGenerateResponse;
    }

    regProcessOperation = countProcessOperation;
    regInvalidOperation = countInvalidOperation;
    regGenerateResponse = countGenerateResponse;
    regAddOperation = countAddOperation;
    regModifyOperation = countModifyOperation;
    regDeleteOperation = countDeleteOperation;
    regTransactOperation = countTransactOperation;
    regHaltOperation = countHaltOperation;
    regTimestampError = countTimestampError;
    regOperationError = countOperationError;
    regSymbolError = countSymbolError;
    regDirectionError = countDirectionError;
    regLevelError = countLevelError;

    return;
}

unsigned int OrderBook::queryPriceLevel(ap_uint<8> symbolIndex,
                                        ap_uint<32> price,
                                        ap_uint<8> direction,
                                        ap_int<8> level)
{
#pragma HLS INLINE

    ap_uint<8> priceLevel;

    if(LEVEL_UNSPECIFIED == level)
    {
        // TODO
        priceLevel = 0;
    }
    else
    {
        // market by price, use level as directed by opcode
        priceLevel = level;
    }

    return priceLevel;
}

void OrderBook::operationAdd(ap_uint<8> symbolIndex,
                             ap_uint<32> orderCount,
                             ap_uint<32> quantity,
                             ap_uint<32> price,
                             ap_uint<8> direction,
                             ap_int<8> level)
{
#pragma HLS INLINE

    ap_uint<8> priceLevel;

    priceLevel = queryPriceLevel(symbolIndex, price, direction, level);

    // TODO: remove code duplication, set a pointer to either bid side or ask side data storage, still deciding whether
    //       to store book data in flat wide vectors or use arrays, this was originally array based but switched to data
    //       slicing during throughput investigations, arrays will offer better readability and scale better to
    //       different book depths (may need to use UNPACK directives)
    if(ORDER_BID == direction)
    {
        switch(level)
        {
            case(0):
            {
                orderBookBidCount[symbolIndex].range(159,32) = orderBookBidCount[symbolIndex].range(127,0);
                orderBookBidCount[symbolIndex].range(31,0) = orderCount;
                orderBookBidPrice[symbolIndex].range(159,32) = orderBookBidPrice[symbolIndex].range(127,0);
                orderBookBidPrice[symbolIndex].range(31,0) = price;
                orderBookBidQuantity[symbolIndex].range(159,32) = orderBookBidQuantity[symbolIndex].range(127,0);
                orderBookBidQuantity[symbolIndex].range(31,0) = quantity;
                break;
            }
            case(1):
            {
                orderBookBidCount[symbolIndex].range(159,64) = orderBookBidCount[symbolIndex].range(127,32);
                orderBookBidCount[symbolIndex].range(63,32) = orderCount;
                orderBookBidPrice[symbolIndex].range(159,64) = orderBookBidPrice[symbolIndex].range(127,32);
                orderBookBidPrice[symbolIndex].range(63,32) = price;
                orderBookBidQuantity[symbolIndex].range(159,64) = orderBookBidQuantity[symbolIndex].range(127,32);
                orderBookBidQuantity[symbolIndex].range(63,32) = quantity;
                break;
            }
            case(2):
            {
                orderBookBidCount[symbolIndex].range(159,96) = orderBookBidCount[symbolIndex].range(127,64);
                orderBookBidCount[symbolIndex].range(95,64) = orderCount;
                orderBookBidPrice[symbolIndex].range(159,96) = orderBookBidPrice[symbolIndex].range(127,64);
                orderBookBidPrice[symbolIndex].range(95,64) = price;
                orderBookBidQuantity[symbolIndex].range(159,96) = orderBookBidQuantity[symbolIndex].range(127,64);
                orderBookBidQuantity[symbolIndex].range(95,64) = quantity;
                break;
            }
            case(3):
            {
                orderBookBidCount[symbolIndex].range(159,128) = orderBookBidCount[symbolIndex].range(127,96);
                orderBookBidCount[symbolIndex].range(127,96) = orderCount;
                orderBookBidPrice[symbolIndex].range(159,128) = orderBookBidPrice[symbolIndex].range(127,96);
                orderBookBidPrice[symbolIndex].range(127,96) = price;
                orderBookBidQuantity[symbolIndex].range(159,128) = orderBookBidQuantity[symbolIndex].range(127,96);
                orderBookBidQuantity[symbolIndex].range(127,96) = quantity;
                break;
            }
            case(4):
            {
                orderBookBidCount[symbolIndex].range(159,128) = orderCount;
                orderBookBidPrice[symbolIndex].range(159,128) = price;
                orderBookBidQuantity[symbolIndex].range(159,128) = quantity;
                break;
            }
            default:
            {
                // TODO: error handling
                KDEBUG("ERROR: Unsupported price level received")
                break;
            }
        }
    }
    else if(ORDER_ASK == direction)
    {
        switch(level)
        {
            case(0):
            {
                orderBookAskCount[symbolIndex].range(159,32) = orderBookAskCount[symbolIndex].range(127,0);
                orderBookAskCount[symbolIndex].range(31,0) = orderCount;
                orderBookAskPrice[symbolIndex].range(159,32) = orderBookAskPrice[symbolIndex].range(127,0);
                orderBookAskPrice[symbolIndex].range(31,0) = price;
                orderBookAskQuantity[symbolIndex].range(159,32) = orderBookAskQuantity[symbolIndex].range(127,0);
                orderBookAskQuantity[symbolIndex].range(31,0) = quantity;
                break;
            }
            case(1):
            {
                orderBookAskCount[symbolIndex].range(159,64) = orderBookAskCount[symbolIndex].range(127,32);
                orderBookAskCount[symbolIndex].range(63,32) = orderCount;
                orderBookAskPrice[symbolIndex].range(159,64) = orderBookAskPrice[symbolIndex].range(127,32);
                orderBookAskPrice[symbolIndex].range(63,32) = price;
                orderBookAskQuantity[symbolIndex].range(159,64) = orderBookAskQuantity[symbolIndex].range(127,32);
                orderBookAskQuantity[symbolIndex].range(63,32) = quantity;
                break;
            }
            case(2):
            {
                orderBookAskCount[symbolIndex].range(159,96) = orderBookAskCount[symbolIndex].range(127,64);
                orderBookAskCount[symbolIndex].range(95,64) = orderCount;
                orderBookAskPrice[symbolIndex].range(159,96) = orderBookAskPrice[symbolIndex].range(127,64);
                orderBookAskPrice[symbolIndex].range(95,64) = price;
                orderBookAskQuantity[symbolIndex].range(159,96) = orderBookAskQuantity[symbolIndex].range(127,64);
                orderBookAskQuantity[symbolIndex].range(95,64) = quantity;
                break;
            }
            case(3):
            {
                orderBookAskCount[symbolIndex].range(159,128) = orderBookAskCount[symbolIndex].range(127,96);
                orderBookAskCount[symbolIndex].range(127,96) = orderCount;
                orderBookAskPrice[symbolIndex].range(159,128) = orderBookAskPrice[symbolIndex].range(127,96);
                orderBookAskPrice[symbolIndex].range(127,96) = price;
                orderBookAskQuantity[symbolIndex].range(159,128) = orderBookAskQuantity[symbolIndex].range(127,96);
                orderBookAskQuantity[symbolIndex].range(127,96) = quantity;
                break;
            }
            case(4):
            {
                orderBookAskCount[symbolIndex].range(159,128) = orderCount;
                orderBookAskPrice[symbolIndex].range(159,128) = price;
                orderBookAskQuantity[symbolIndex].range(159,128) = quantity;
                break;
            }
            default:
            {
                // TODO: error handling
                KDEBUG("ERROR: Unsupported price level received");
                break;
            }
        }
    }
    else
    {
        // TODO: error handling
        KDEBUG("ERROR: Unsupported direction received");
    }

    return;
}

void OrderBook::operationModify(ap_uint<8> symbolIndex,
                                ap_uint<32> orderCount,
                                ap_uint<32> quantity,
                                ap_uint<32> price,
                                ap_uint<8> direction,
                                ap_int<8> level)
{
#pragma HLS INLINE

    ap_uint<8> priceLevel;

    priceLevel = queryPriceLevel(symbolIndex, price, direction, level);

    // TODO: remove code duplication, set a pointer to either bid side or ask side data storage
    if(ORDER_BID == direction)
    {
        switch(level)
        {
            case(0):
            {
                orderBookBidCount[symbolIndex].range(31,0) = orderCount;
                orderBookBidPrice[symbolIndex].range(31,0) = price;
                orderBookBidQuantity[symbolIndex].range(31,0) = quantity;
                break;
            }
            case(1):
            {
                orderBookBidCount[symbolIndex].range(63,32) = orderCount;
                orderBookBidPrice[symbolIndex].range(63,32) = price;
                orderBookBidQuantity[symbolIndex].range(63,32) = quantity;
                break;
            }
            case(2):
            {
                orderBookBidCount[symbolIndex].range(95,64) = orderCount;
                orderBookBidPrice[symbolIndex].range(95,64) = price;
                orderBookBidQuantity[symbolIndex].range(95,64) = quantity;
                break;
            }
            case(3):
            {
                orderBookBidCount[symbolIndex].range(127,96) = orderCount;
                orderBookBidPrice[symbolIndex].range(127,96) = price;
                orderBookBidQuantity[symbolIndex].range(127,96) = quantity;
                break;
            }
            case(4):
            {
                orderBookBidCount[symbolIndex].range(159,128) = orderCount;
                orderBookBidPrice[symbolIndex].range(159,128) = price;
                orderBookBidQuantity[symbolIndex].range(159,128) = quantity;
                break;
            }
            default:
            {
                // TODO: error handling
                KDEBUG("ERROR: Unsupported price level received");
                break;
            }
        }
    }
    else if(ORDER_ASK == direction)
    {
        switch(level)
        {
            case(0):
            {
                orderBookAskCount[symbolIndex].range(31,0) = orderCount;
                orderBookAskPrice[symbolIndex].range(31,0) = price;
                orderBookAskQuantity[symbolIndex].range(31,0) = quantity;
                break;
            }
            case(1):
            {
                orderBookAskCount[symbolIndex].range(63,32) = orderCount;
                orderBookAskPrice[symbolIndex].range(63,32) = price;
                orderBookAskQuantity[symbolIndex].range(63,32) = quantity;
                break;
            }
            case(2):
            {
                orderBookAskCount[symbolIndex].range(95,64) = orderCount;
                orderBookAskPrice[symbolIndex].range(95,64) = price;
                orderBookAskQuantity[symbolIndex].range(95,64) = quantity;
                break;
            }
            case(3):
            {
                orderBookAskCount[symbolIndex].range(127,96) = orderCount;
                orderBookAskPrice[symbolIndex].range(127,96) = price;
                orderBookAskQuantity[symbolIndex].range(127,96) = quantity;
                break;
            }
            case(4):
            {
                orderBookAskCount[symbolIndex].range(159,128) = orderCount;
                orderBookAskPrice[symbolIndex].range(159,128) = price;
                orderBookAskQuantity[symbolIndex].range(159,128) = quantity;
                break;
            }
            default:
            {
                // TODO: error handling
                KDEBUG("ERROR: Unsupported price level received");
                break;
            }
        }
    }
    else
    {
        // TODO: error handling
        KDEBUG("ERROR: Unsupported direction received");
    }

    return;
}

void OrderBook::operationDelete(ap_uint<8> symbolIndex,
                                ap_uint<32> orderCount,
                                ap_uint<32> quantity,
                                ap_uint<32> price,
                                ap_uint<8> direction,
                                ap_int<8> level)
{
#pragma HLS INLINE

    ap_uint<8> priceLevel;

    priceLevel = queryPriceLevel(symbolIndex, price, direction, level);

    if(ORDER_BID == direction)
    {
        switch(level)
        {
            case(0):
            {
                orderBookBidCount[symbolIndex].range(127,0) = orderBookBidCount[symbolIndex].range(159,32);
                orderBookBidCount[symbolIndex].range(159,128) = 0;
                orderBookBidPrice[symbolIndex].range(127,0) = orderBookBidPrice[symbolIndex].range(159,32);
                orderBookBidPrice[symbolIndex].range(159,128) = 0;
                orderBookBidQuantity[symbolIndex].range(127,0) = orderBookBidQuantity[symbolIndex].range(159,32);
                orderBookBidQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            case(1):
            {
                orderBookBidCount[symbolIndex].range(127,32) = orderBookBidCount[symbolIndex].range(159,64);
                orderBookBidCount[symbolIndex].range(159,128) = 0;
                orderBookBidPrice[symbolIndex].range(127,32) = orderBookBidPrice[symbolIndex].range(159,64);
                orderBookBidPrice[symbolIndex].range(159,128) = 0;
                orderBookBidQuantity[symbolIndex].range(127,32) = orderBookBidQuantity[symbolIndex].range(159,64);
                orderBookBidQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            case(2):
            {
                orderBookBidCount[symbolIndex].range(127,64) = orderBookBidCount[symbolIndex].range(159,96);
                orderBookBidCount[symbolIndex].range(159,128) = 0;
                orderBookBidPrice[symbolIndex].range(127,64) = orderBookBidPrice[symbolIndex].range(159,96);
                orderBookBidPrice[symbolIndex].range(159,128) = 0;
                orderBookBidQuantity[symbolIndex].range(127,64) = orderBookBidQuantity[symbolIndex].range(159,96);
                orderBookBidQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            case(3):
            {
                orderBookBidCount[symbolIndex].range(127,96) = orderBookBidCount[symbolIndex].range(159,128);
                orderBookBidCount[symbolIndex].range(159,128) = 0;
                orderBookBidPrice[symbolIndex].range(127,96) = orderBookBidPrice[symbolIndex].range(159,128);
                orderBookBidPrice[symbolIndex].range(159,128) = 0;
                orderBookBidQuantity[symbolIndex].range(127,96) = orderBookBidQuantity[symbolIndex].range(159,128);
                orderBookBidQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            case(4):
            {
                orderBookBidCount[symbolIndex].range(159,128) = 0;
                orderBookBidPrice[symbolIndex].range(159,128) = 0;
                orderBookBidQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            default:
            {
                // TODO: error handling
                KDEBUG("ERROR: Unsupported price level received");
                break;
            }
        }
    }
    else if(ORDER_ASK == direction)
    {
        switch(level)
        {
            case(0):
            {
                orderBookAskCount[symbolIndex].range(127,0) = orderBookAskCount[symbolIndex].range(159,32);
                orderBookAskCount[symbolIndex].range(159,128) = 0;
                orderBookAskPrice[symbolIndex].range(127,0) = orderBookAskPrice[symbolIndex].range(159,32);
                orderBookAskPrice[symbolIndex].range(159,128) = 0;
                orderBookAskQuantity[symbolIndex].range(127,0) = orderBookAskQuantity[symbolIndex].range(159,32);
                orderBookAskQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            case(1):
            {
                orderBookAskCount[symbolIndex].range(127,32) = orderBookAskCount[symbolIndex].range(159,64);
                orderBookAskCount[symbolIndex].range(159,128) = 0;
                orderBookAskPrice[symbolIndex].range(127,32) = orderBookAskPrice[symbolIndex].range(159,64);
                orderBookAskPrice[symbolIndex].range(159,128) = 0;
                orderBookAskQuantity[symbolIndex].range(127,32) = orderBookAskQuantity[symbolIndex].range(159,64);
                orderBookAskQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            case(2):
            {
                orderBookAskCount[symbolIndex].range(127,64) = orderBookAskCount[symbolIndex].range(159,96);
                orderBookAskCount[symbolIndex].range(159,128) = 0;
                orderBookAskPrice[symbolIndex].range(127,64) = orderBookAskPrice[symbolIndex].range(159,96);
                orderBookAskPrice[symbolIndex].range(159,128) = 0;
                orderBookAskQuantity[symbolIndex].range(127,64) = orderBookAskQuantity[symbolIndex].range(159,96);
                orderBookAskQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            case(3):
            {
                orderBookAskCount[symbolIndex].range(127,96) = orderBookAskCount[symbolIndex].range(159,128);
                orderBookAskCount[symbolIndex].range(159,128) = 0;
                orderBookAskPrice[symbolIndex].range(127,96) = orderBookAskPrice[symbolIndex].range(159,128);
                orderBookAskPrice[symbolIndex].range(159,128) = 0;
                orderBookAskQuantity[symbolIndex].range(127,96) = orderBookAskQuantity[symbolIndex].range(159,128);
                orderBookAskQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            case(4):
            {
                orderBookAskCount[symbolIndex].range(159,128) = 0;
                orderBookAskPrice[symbolIndex].range(159,128) = 0;
                orderBookAskQuantity[symbolIndex].range(159,128) = 0;
                break;
            }
            default:
            {
                // TODO: error handling
                KDEBUG("ERROR: Unsupported price level received");
                break;
            }
        }
    }
    else
    {
        // TODO: error handling
        KDEBUG("ERROR: Unsupported direction received");
    }

    return;
}

void OrderBook::operationTransactVisible(ap_uint<8> symbolIndex,
                                         ap_uint<32> orderCount,
                                         ap_uint<32> quantity,
                                         ap_uint<32> price,
                                         ap_uint<8> direction,
                                         ap_int<8> level)
{
    KDEBUG("TODO: operationTransactVisible");

    return;
}

void OrderBook::operationTransactHidden(ap_uint<8> symbolIndex,
                                        ap_uint<32> orderCount,
                                        ap_uint<32> quantity,
                                        ap_uint<32> price,
                                        ap_uint<8> direction,
                                        ap_int<8> level)
{
    KDEBUG("TODO: operationTransactHidden");

    return;
}

void OrderBook::operationHalt(void)
{
    KDEBUG("TODO: operationHalt");

    return;
}

void OrderBook::responsePush(ap_uint<32> &regControl,
                             ap_uint<32> &regCaptureControl,
                             ap_uint<32> &regTxResponse,
                             ap_uint<1024> &regCaptureBuffer,
                             hls::stream<orderBookResponse_t> &responseStream,
                             hls::stream<orderBookResponsePack_t> &responseStreamPack,
                             hls::stream<orderBookResponsePack_t> &dataMoveStreamPack)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    orderBookResponse_t response;
    orderBookResponsePack_t responsePack;

    static ap_uint<32> countTxResponse=0;

    if(!responseStream.empty())
    {
        response = responseStream.read();

        intf.orderBookResponsePack(&response, &responsePack);
        responseStreamPack.write(responsePack);
        ++countTxResponse;

        if(OB_DM_FWD_ENABLE & regControl)
        {
            dataMoveStreamPack.write(responsePack);
        }

        // check if host has capture freeze control enabled before updating
        // TODO: filter capture by user supplied symbol
        if(0 == (OB_CAPTURE_FREEZE & regCaptureControl))
        {
            regCaptureBuffer = responsePack.data;
        }
    }

    regTxResponse = countTxResponse;

    return;
}

void OrderBook::responseDump(ap_uint<1024> responseDump[1],
                             hls::stream<orderBookResponsePack_t> &responseStreamPack)
{
#pragma HLS PIPELINE II=1 style=flp

    orderBookResponsePack_t responsePack;

    if(!responseStreamPack.empty())
    {
        responsePack = responseStreamPack.read();
        responseDump[0] = responsePack.data;
    }

    return;
}

void OrderBook::responseMove(ap_uint<32> &regControl,
                             ap_uint<32> &regIndexHead,
                             ap_uint<32> &regIndexTail,
                             ap_uint<32> &regTxResponse,
                             ap_uint<32> &regCyclesPre,
                             ap_uint<1024> ringBuffer[OB_DM_RING_BUF_LEN],
                             hls::stream<orderBookResponsePack_t> &responseStreamPack)
{
#pragma HLS PIPELINE II=1 style=flp

    orderBookResponsePack_t responsePack;

    static ap_uint<32> countCycles=0;
    static ap_uint<16> countIndexTail=0;
    static ap_uint<32> countTxResponse=0;

    // TODO: move local counter to class?
    if(OB_DM_RTT_RESET & regControl)
    {
        countCycles = 0;
    }
    else
    {
        if(OB_DM_RTT_ENABLE & regControl)
        {
            ++countCycles;
        }
    }

    if(!responseStreamPack.empty())
    {
        responsePack = responseStreamPack.read();

        // inject local timestamp for host rtt latency measurement
        if(OB_DM_RTT_ENABLE & regControl)
        {
            responsePack.data.range(1023,968) = countCycles;
        }

        // write to ring buffer, advance tail pointer
        ringBuffer[countIndexTail++] = responsePack.data;
        ++countTxResponse;
    }

    regCyclesPre = countCycles;
    regIndexTail = countIndexTail;
    regTxResponse = countTxResponse;

    return;
}

void OrderBook::operationMove(ap_uint<32> &regControl,
                              ap_uint<32> &regIndexTail,
                              ap_uint<32> &regRxThrottleRate,
                              ap_uint<32> &regIndexHead,
                              ap_uint<32> &regRxOperation,
                              ap_uint<32> &regLatencyMin,
                              ap_uint<32> &regLatencyMax,
                              ap_uint<32> &regLatencySum,
                              ap_uint<32> &regLatencyCount,
                              ap_uint<32> &regCyclesPost,
                              ap_uint<32> &regRxThrottleCount,
                              ap_uint<32> &regRxThrottleEvent,
                              ap_uint<256> ringBuffer[OB_DM_RING_BUF_LEN],
                              hls::stream<orderEntryOperationPack_t> &operationStreamPack)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    orderEntryOperationPack_t operationPack;
    orderEntryOperation_t operation;
    ap_uint<32> latencyDiff;

    static ap_uint<32> countCycles=0;
    static ap_uint<16> countIndexHead=0;
    static ap_uint<32> countRxOperation=0;
    static ap_uint<32> countRxThrottle=0;
    static ap_uint<32> countRxBackpressure=0;

    static ap_uint<32> latencyMin=0xffffffff;
    static ap_uint<32> latencyMax=0;
    static ap_uint<32> latencySum=0;
    static ap_uint<32> latencyCount=0;

    // TODO: move local counter to class?
    if(OB_DM_RTT_RESET & regControl)
    {
        countCycles = 0;
        latencyMin = 0xffffffff;
        latencyMax = 0;
        latencySum = 0;
        latencyCount = 0;
    }
    else
    {
        if(OB_DM_RTT_ENABLE & regControl)
        {
            ++countCycles;
        }
    }

    // use rate throttle to account for software stalls where data can back up
    // in H2C ring buffer, OrderEntry operations expand to minumum of 32 frames
    // in OrderEntry send, without a throttle this expansion can saturate that
    // kernel and result in deadlock, alternative approach would be to buffer
    // operations downstream before the send but data is already buffered here,
    // simple addition of dispatch rate seems like the logical approach
    if(0 == countRxThrottle)
    {
        // TODO: buffer wrap checks needed?
        if(countIndexHead != regIndexTail)
        {
            // read from ring buffer, advance head pointer
            operationPack.data = ringBuffer[countIndexHead++];
            ++countRxOperation;

            // debug control to disable forwarding operations
            // TODO: remove?
            if(OB_DM_HALT & regControl)
            {
                // nop
                countRxThrottle = 0;
            }
            else
            {
                // forward to OrderEntry
                operationStreamPack.write(operationPack);

                // reset rate throttle, must decrement to 0 before next dispatch
                countRxThrottle = regRxThrottleRate;
            }

            // rtt latency measurement
            if(OB_DM_RTT_ENABLE & regControl)
            {
                // could calculate a rolling average here but as this is a
                // debug feature the approach adopted is to keep the capture
                // as simple as possible and defer to software to co-ordinate
                // using reset and enable control provided

                // software should ensure enable is cleared before readback to
                // ensure synchronistion across registers and also that capture
                // duration does not result in latencySum register overflow
                // (hardware could add status flag for latter)
                intf.orderEntryOperationUnpack(&operationPack, &operation);
                latencyDiff = (countCycles - operation.timestamp);
                latencySum += latencyDiff;
                ++latencyCount;

                if(latencyDiff < latencyMin)
                {
                    latencyMin = latencyDiff;
                }

                if(latencyDiff > latencyMax)
                {
                    latencyMax = latencyDiff;
                }
            }
        }
    }
    else
    {
        --countRxThrottle;
        if((countIndexHead != regIndexTail) && (0 == countRxThrottle))
        {
            // update received from host while under throttle
            ++countRxBackpressure;
        }
    }

    regIndexHead = countIndexHead;
    regRxOperation = countRxOperation;
    regLatencyMin = latencyMin;
    regLatencyMax = latencyMax;
    regLatencySum = latencySum;
    regLatencyCount = latencyCount;
    regCyclesPost = countCycles;
    regRxThrottleCount = countRxThrottle;
    regRxThrottleEvent = countRxBackpressure;

    return;
}
