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

#include <fstream>
#include <iomanip>
#include <iostream>

#include "orderbook_kernels.hpp"

int main()
{
    orderBookRegControl_t regControl={0};
    orderBookRegStatus_t regStatus={0};
    ap_uint<1024> regCapture=0x0;
    ap_uint<32> rangeIndexHigh, rangeIndexLow;
    ap_uint<32> bidCount[5], bidPrice[5], bidQuantity[5];
    ap_uint<32> askCount[5], askPrice[5], askQuantity[5];

    mmInterface intf;
    orderBookOperation_t operation;
    orderBookOperationPack_t operationPack;
    orderBookResponsePack_t responsePack;
    orderBookResponse_t response;

    hls::stream<orderBookOperationPack_t> operationStreamPackFIFO;
    hls::stream<orderBookResponsePack_t> responseStreamPackFIFO;
    hls::stream<orderBookResponsePack_t> dataMoveStreamPackFIFO;

    std::cout << "OrderBook Test" << std::endl;
    std::cout << "--------------" << std::endl;

    orderBookOperation_t inputOperations[] =
    {
        // timestamp, opCode, symbolIndex, orderId, orderCount, quantity, price, direction, level
        // 0-9 = init
        {1571145019318770688,1,0,101,1,730,10000,0,0},
        {1571145019328685056,1,0,102,1,710,10200,1,0},
        {1571145019329758976,1,0,103,9,640,9900,0,1},
        {1571145019330797312,1,0,104,5,890,10300,1,1},
        {1571145019331833344,1,0,105,2,640,9800,0,2},
        {1571145019332884480,1,0,106,3,20,10400,1,2},
        {1571145019333912832,1,0,107,6,440,9700,0,3},
        {1571145019334959616,1,0,108,1,430,10500,1,3},
        {1571145019336062720,1,0,109,5,660,9600,0,4},
        {1571145019337140480,1,0,110,6,470,10600,1,4},
        // 10-29 = walk up
        {1571145019338322432,0,0,111,5,260,10050,0,0},
        {1571145019339423232,1,0,112,1,710,10250,1,0},
        {1571145019340597504,0,0,113,2,890,10100,0,0},
        {1571145019341809152,2,0,114,5,890,10300,1,0},
        {1571145019342878720,0,0,115,7,50,10700,1,4},
        {1571145019344016640,0,0,116,3,770,10150,0,0},
        {1571145019345140736,1,0,117,5,890,10350,1,0},
        {1571145019346270208,0,0,118,6,290,10200,0,0},
        {1571145019347402496,2,0,119,3,20,10400,1,0},
        {1571145019348440064,0,0,120,3,270,10800,1,4},
        {1571145019349566976,0,0,121,5,870,10250,0,0},
        {1571145019350644992,1,0,122,3,20,10450,1,0},
        {1571145019351792640,0,0,123,4,730,10300,0,0},
        {1571145019352936960,2,0,124,1,430,10500,1,0},
        {1571145019354059264,0,0,125,5,360,10900,1,4},
        {1571145019355169280,0,0,126,7,330,10350,0,0},
        {1571145019356219648,1,0,127,1,430,10550,1,0},
        {1571145019357370112,0,0,128,4,910,10400,0,0},
        {1571145019358504704,2,0,129,6,470,10600,1,0},
        {1571145019359561472,0,0,130,3,830,11000,1,4},
        // 30-53 = walk down
        {1571145019360711424,2,0,131,7,330,10350,0,0},
        {1571145019361763584,0,0,132,8,130,10100,0,4},
        {1571145019362966528,0,0,133,2,220,10550,1,0},
        {1571145019364120064,2,0,134,4,730,10300,0,0},
        {1571145019365148928,0,0,135,3,50,10000,0,4},
        {1571145019366260992,0,0,136,7,710,10500,1,0},
        {1571145019367428096,2,0,137,5,870,10250,0,0},
        {1571145019368484608,0,0,138,8,280,9900,0,4},
        {1571145019369615360,0,0,139,5,740,10450,1,0},
        {1571145019370785280,2,0,140,6,290,10200,0,0},
        {1571145019371944448,0,0,141,7,570,9800,0,4},
        {1571145019373060096,0,0,142,1,780,10400,1,0},
        {1571145019374203904,2,0,143,8,130,10100,0,0},
        {1571145019375249920,0,0,144,4,450,9700,0,4},
        {1571145019376353280,0,0,145,5,630,10350,1,0},
        {1571145019377526784,2,0,146,3,50,10000,0,0},
        {1571145019378587648,0,0,147,1,130,9600,0,4},
        {1571145019379707136,0,0,148,5,530,10300,1,0},
        {1571145019380940288,2,0,149,8,280,9900,0,0},
        {1571145019381969664,0,0,150,9,480,9500,0,4},
        {1571145019383081472,0,0,151,3,770,10250,1,0},
        {1571145019384279040,2,0,152,7,570,9800,0,0},
        {1571145019385332224,0,0,153,3,970,9400,0,4},
        {1571145019386460416,0,0,154,7,440,10200,1,0},
    };

    for(int i=0; i<NUM_TEST_SAMPLE; i++)
    {
        operation = inputOperations[i];
        intf.orderBookOperationPack(&operation, &operationPack);
        operationStreamPackFIFO.write(operationPack);
    }

    // configure
    regControl.control = 0x00000000;
    regControl.config  = 0xdeadbeef;
    regControl.capture = 0x00000000;

    // kernel call to process operations
    while(!operationStreamPackFIFO.empty())
    {
        orderBookTop(regControl,
                     regStatus,
                     regCapture,
                     operationStreamPackFIFO,
                     responseStreamPackFIFO,
                     dataMoveStreamPackFIFO);
    }

    // drain response stream
    while(!responseStreamPackFIFO.empty())
    {
        responsePack = responseStreamPackFIFO.read();
        intf.orderBookResponseUnpack(&responsePack, &response);

        std::cout << "BOOK_RESPONSE[" << response.symbolIndex << "]: BID";

        for(int i=0; i<NUM_LEVEL; i++)
        {
            rangeIndexLow = i * 32;
            rangeIndexHigh = rangeIndexLow + 31;

            bidCount[i] = response.bidCount.range(rangeIndexHigh,rangeIndexLow);
            bidPrice[i] = response.bidPrice.range(rangeIndexHigh,rangeIndexLow);
            bidQuantity[i] = response.bidQuantity.range(rangeIndexHigh,rangeIndexLow);

            askCount[i] = response.askCount.range(rangeIndexHigh,rangeIndexLow);
            askPrice[i] = response.askPrice.range(rangeIndexHigh,rangeIndexLow);
            askQuantity[i] = response.askQuantity.range(rangeIndexHigh,rangeIndexLow);
        }

        for(int j=NUM_LEVEL-1; j>=0; j--)
        {
            if(0==bidQuantity[j] && 0==bidPrice[j])
            {
                std::cout << " | NULL       ";
            }
            else
            {
                std::cout << " | "
                          << bidQuantity[j]
                          << "["
                          << bidCount[j]
                          << "]"
                          << "@"
                          << bidPrice[j];
            }
        }

        std::cout << " || ";

        for(int j=0; j<NUM_LEVEL; j++)
        {
            if(0==askQuantity[j] && 0==askPrice[j])
            {
                std::cout << "NULL         | ";
            }
            else
            {
                std::cout << askQuantity[j]
                          << "["
                          << askCount[j]
                          << "]"
                          << "@"
                          << askPrice[j]
                          << " | ";
            }
        }

        std::cout << "ASK"
                  << std::endl;
    }

    // log final status
    std::cout << "--" << std::hex << std::endl;
    std::cout << "STATUS: ";
    std::cout << "OB_STATUS=" << regStatus.status << " ";
    std::cout << "OB_RX_OP=" << regStatus.rxOperation << " ";
    std::cout << "OB_PROC_OP=" << regStatus.processOperation << " ";
    std::cout << "OB_INVALID_OP=" << regStatus.invalidOperation << " ";
    std::cout << "OB_GEN_RESP=" << regStatus.generateResponse << " ";
    std::cout << "OB_TX_RESP=" << regStatus.txResponse << " ";
    std::cout << "OB_ADD_OP=" << regStatus.addOperation << " ";
    std::cout << "OB_MODIFY_OP=" << regStatus.modifyOperation << " ";
    std::cout << "OB_DELETE_OP=" << regStatus.deleteOperation << " ";
    std::cout << "OB_TRANSACT_OP=" << regStatus.transactOperation << " ";
    std::cout << "OB_HALT_OP=" << regStatus.haltOperation << " ";
    std::cout << "OB_TIMESTAMP_ERR=" << regStatus.timestampError << " ";
    std::cout << "OB_OPCODE_ERR=" << regStatus.operationError << " ";
    std::cout << "OB_SYMBOL_ERR=" << regStatus.symbolError << " ";
    std::cout << "OB_DIRECTION_ERR=" << regStatus.directionError << " ";
    std::cout << "OB_LEVEL_ERR=" << regStatus.levelError << " ";
    std::cout << "OB_RX_EVENT=" << regStatus.rxEvent << " ";
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Done!" << std::endl;

    return 0;
}
