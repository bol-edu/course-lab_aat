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

#include "feedhandler_kernels.hpp"

#define NUM_PACKET           (54)
#define NUM_FRAME_PER_PACKET (13)

// TODO: templated byteReverse function for various widths in common
ap_uint<64> byteReverse(ap_uint<64> inputData)
{
    ap_uint<64> reversed = (inputData.range(7,0),
                            inputData.range(15,8),
                            inputData.range(23,16),
                            inputData.range(31,24),
                            inputData.range(39,32),
                            inputData.range(47,40),
                            inputData.range(55,48),
                            inputData.range(63,56));

    return reversed;
}

int main()
{
    feedHandlerRegControl_t regControl={0};
    feedHandlerRegStatus_t regStatus={0};
    regSymbolMapContainer_t regSymbolContainer;
    ap_uint<256> regCapture=0x0;

    mmInterface intf;
    axiWordExt_t axiw;
    orderBookOperation_t operation;
    orderBookOperationPack_t operationPack;

    hls::stream<axiWordExt_t> inputDataStream;
    hls::stream<orderBookOperationPack_t> operationStreamPack;

    std::cout << "FeedHandler Test" << std::endl;
    std::cout << "----------------" << std::endl;

    memset(&regSymbolContainer, 0, sizeof(regSymbolContainer));

    // constant fields
    axiw.strb = 0xFF;
    axiw.keep = 0xFF;

    // TODO: pull packet payloads from pcap file
    ap_uint<64> inputWords[NUM_PACKET][NUM_FRAME_PER_PACKET] =
    {
#include "input_golden.dat"
    };

    // configure
    regControl.control = 0x00000000;

    // symbol map load
    regSymbolContainer.symbols[0] = 0x11111111;
    regSymbolContainer.symbols[1] = 0x22222222;
    regSymbolContainer.symbols[2] = 0x33333333;
    regSymbolContainer.symbols[3] = 0x44444444;
    regSymbolContainer.symbols[4] = 0x55555555;
    regSymbolContainer.symbols[5] = 0x66666666;
    regSymbolContainer.symbols[6] = 0x77777777;
    regSymbolContainer.symbols[7] = 0x88888888;
    regSymbolContainer.symbols[8] = 0x99999999;
    regSymbolContainer.symbols[9] = 0x12345678; // securityID used in test messages

    // process
    for(int packet=0; packet<NUM_PACKET; packet++)
    {
        axiw.last = 0;
        for(int frame=0; frame<NUM_FRAME_PER_PACKET; frame++)
        {
            if((NUM_FRAME_PER_PACKET-1) == frame)
            {
                axiw.last = 1;
            }

            axiw.data = byteReverse(inputWords[packet][frame]);
            inputDataStream.write(axiw);
        }

        // process
        while(!inputDataStream.empty())
        {
            feedHandlerTop(regControl,
                           regStatus,
                           regSymbolContainer,
                           regCapture,
                           inputDataStream,
                           operationStreamPack);
        }
    }

    // this seems to be required to flush the pipeline and get the final response
    // without it "WARNING: Hls::stream 'udpDataFifo' contains leftover data" is reported
    for(int i; i<64; i++)
    {
        feedHandlerTop(regControl,
                       regStatus,
                       regSymbolContainer,
                       regCapture,
                       inputDataStream,
                       operationStreamPack);
    }

    // drain
    while(!operationStreamPack.empty())
    {
        operationPack = operationStreamPack.read();
        intf.orderBookOperationUnpack(&operationPack, &operation);

        std::cout << std::dec << "OPERATION: "
                  << operation.timestamp << ","
                  << operation.opCode << ","
                  << operation.symbolIndex << ","
                  << operation.orderId << ","
                  << operation.orderCount << ","
                  << operation.quantity << ","
                  << operation.price << ","
                  << operation.direction << ","
                  << operation.level << std::endl;
    }

    // log final status
    std::cout << "--" << std::hex << std::endl;
    std::cout << "STATUS: ";
    std::cout << "FH_PROCESS_WORD=" << regStatus.processWord << " ";
    std::cout << "FH_PROCESS_PACKET=" << regStatus.processPacket << " ";
    std::cout << "FH_PROCESS_BINARY=" << regStatus.processBinary << " ";
    std::cout << "FH_PROCESS_FIX=" << regStatus.processFix << " ";
    std::cout << "FH_TX_OP=" << regStatus.txOperation << " ";
    std::cout << "FH_RX_EVENT=" << regStatus.rxEvent << " ";
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Done!" << std::endl;

    return 0;
}
