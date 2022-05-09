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

#include "orderentry_kernels.hpp"

#define NUM_TEST_SAMPLE_OE (4)

int main()
{
    orderEntryRegControl_t regControl={0};
    orderEntryRegStatus_t regStatus={0};
    ap_uint<1024> regCapture=0x0;
    ap_uint<32> loopCount;

    mmInterface intf;
    orderEntryOperation_t operation;
    orderEntryOperationPack_t operationPack;
    ipTcpTxMetaPack_t txMetaDataPack;
    ipTcpTxDataPack_t txDataPack;
    ipTcpTxStatusPack_t txStatusPack;
    ipTcpConnectionStatusPack_t openConStatusPack;
    ipTcpListenPortPack_t port;
    ipTuplePack_t connection;
    ipTcpListenStatusPack_t listenStatus_i;

    hls::stream<orderEntryOperationPack_t> operationStreamPackFIFO;
    hls::stream<orderEntryOperationPack_t> operationHostStreamPackFIFO;
    hls::stream<ipTcpListenPortPack_t> listenPort;
    hls::stream<ipTcpListenStatusPack_t> listenStatus;
    hls::stream<ipTcpNotificationPack_t> notifications;
    hls::stream<ipTcpReadRequestPack_t> readRequest;
    hls::stream<ipTcpRxMetaPack_t> rxMetaData;
    hls::stream<ipTcpRxDataPack_t> rxData;
    hls::stream<ipTuplePack_t> openConnection;
    hls::stream<ipTcpConnectionStatusPack_t> openConStatus;
    hls::stream<ipTcpCloseConnectionPack_t> closeConnection;
    hls::stream<ipTcpTxMetaPack_t> txMetaData;
    hls::stream<ipTcpTxDataPack_t> txData;
    hls::stream<ipTcpTxStatusPack_t> txStatus;

    std::cout << "OrderEntryTcp Test" << std::endl;
    std::cout << "------------------" << std::endl;

    orderEntryOperation_t orderEntryOperations[NUM_TEST_SAMPLE_OE] =
    {
        // timestamp, opCode, symbolIndex, orderId, quantity, price, direction
        {0x1111111111111111,1,0,123,800,5853400,1},
        {0x2222222222222222,1,0,234,700,5853500,1},
        {0x3333333333333333,1,0,345,600,5853600,1},
        {0x4444444444444444,1,0,456,500,5853700,1},
    };

    // configure
    regControl.control = (OE_TCP_GEN_SUM | OE_TCP_CONNECT);
    regControl.config = 0xdeadbeef;
    regControl.capture = 0x00000000;
    regControl.destAddress = 0x640aa8c0; // 192.168.10.100
    regControl.destPort = 0x17; // telnet (port 23)

    // kernel calls to connnect
    std::cout << "Setting up connection ..." << std::endl;
    for(int i=0; i<NUM_TEST_SAMPLE_OE; i++)
    {
        orderEntryTcpTop(regControl,
                         regStatus,
                         regCapture,
                         operationStreamPackFIFO,
                         operationHostStreamPackFIFO,
                         listenPort,
                         listenStatus,
                         notifications,
                         readRequest,
                         rxMetaData,
                         rxData,
                         openConnection,
                         openConStatus,
                         closeConnection,
                         txMetaData,
                         txData,
                         txStatus);

        if (!listenPort.empty())
        {
            port = listenPort.read();
            std::cout << std::dec << "listenPort: " << port.data << std::endl;
            listenStatus_i.data = 0x1;
            listenStatus.write(listenStatus_i);
        }

        if (!openConnection.empty())
        {
            connection = openConnection.read();
            std::cout << std::dec << "openConnection: " << connection.data << std::endl;

            // spoof a connection response
            txStatusPack.data.range(15,0) = 0x0001; // sessionID
            txStatusPack.data.range(31,16) = 0x0100; // length
            txStatusPack.data.range(61,32) = 0xffff; // space
            txStatusPack.data.range(63,62) = TXSTATUS_SUCCESS;
            txStatus.write(txStatusPack);
            openConStatusPack.data = 0x10001;
            openConStatus.write(openConStatusPack);
        }
    }

    // generate input orders
    std::cout << "Generating input data ..." << std::endl;
    for(int i=0; i<NUM_TEST_SAMPLE_OE; i++)
    {
        operation = orderEntryOperations[i];
        intf.orderEntryOperationPack(&operation, &operationPack);
        operationStreamPackFIFO.write(operationPack);
    }

    // kernel calls to process operations
    std::cout << "Invoking kernel execution ..." << std::endl;
    for(int i=0; i<(NUM_TEST_SAMPLE_OE*OE_MSG_NUM_FRAME); i++)
    {
        orderEntryTcpTop(regControl,
                         regStatus,
                         regCapture,
                         operationStreamPackFIFO,
                         operationHostStreamPackFIFO,
                         listenPort,
                         listenStatus,
                         notifications,
                         readRequest,
                         rxMetaData,
                         rxData,
                         openConnection,
                         openConStatus,
                         closeConnection,
                         txMetaData,
                         txData,
                         txStatus);
    }

    // drain
    std::cout << "DEBUG: TCP Meta Stream" << std::hex << std::endl;
    while(!txMetaData.empty())
    {
        txMetaDataPack = txMetaData.read();
        std::cout << txMetaDataPack.data << std::endl;
    }

    std::cout << "DEBUG: TCP Data Stream" << std::hex << std::endl;
    loopCount = 0;
    while(!txData.empty())
    {
        txDataPack = txData.read();
        std::cout << txDataPack.data << std::endl;

        // message line break after every packet
        if(0 == (++loopCount % OE_MSG_NUM_FRAME))
        {
            std::cout << std::endl;
        }
    }

    // log final status
    std::cout << "--" << std::hex << std::endl;
    std::cout << "STATUS: ";
    std::cout << "OE_STATUS=" << regStatus.status << " ";
    std::cout << "OE_RX_OP=" << regStatus.rxOperation << " ";
    std::cout << "OE_PROC_OP=" << regStatus.processOperation << " ";
    std::cout << "OE_TX_DATA=" << regStatus.txData << " ";
    std::cout << "OE_TX_META=" << regStatus.txMeta << " ";
    std::cout << "OE_TX_ORDER=" << regStatus.txOrder << " ";
    std::cout << "OE_RX_DATA=" << regStatus.rxData << " ";
    std::cout << "OE_RX_META=" << regStatus.rxMeta << " ";
    std::cout << "OE_RX_EVENT=" << regStatus.rxEvent << " ";
    std::cout << "OE_TX_DROP=" << regStatus.txDrop << " ";
    std::cout << "OE_TX_STATUS=" << regStatus.txStatus << " ";
    std::cout << "OE_DEBUG=" << regStatus.debug << " ";
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Done!" << std::endl;

    return 0;
}
