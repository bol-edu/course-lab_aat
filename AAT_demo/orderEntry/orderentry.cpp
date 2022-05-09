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
#include "orderentry.hpp"

/**
 * OrderEntry Core
 */

void OrderEntry::operationPull(ap_uint<32> &regRxOperation,
                               hls::stream<orderEntryOperationPack_t> &operationStreamPack,
                               hls::stream<orderEntryOperationPack_t> &operationHostStreamPack,
                               hls::stream<orderEntryOperation_t> &operationStream)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    orderEntryOperationPack_t operationPack;
    orderEntryOperation_t operation;

    static ap_uint<32> countRxOperation=0;

    // priority to direct path from PricingEngine then host offload path
    // TODO: add register control to enable/disable these different paths?
    if(!operationStreamPack.empty())
    {
        operationPack = operationStreamPack.read();
        ++countRxOperation;
        intf.orderEntryOperationUnpack(&operationPack, &operation);
        operationStream.write(operation);
    }
    else if(!operationHostStreamPack.empty())
    {
        operationPack = operationHostStreamPack.read();
        ++countRxOperation;
        intf.orderEntryOperationUnpack(&operationPack, &operation);
        operationStream.write(operation);
    }

    regRxOperation = countRxOperation;

    return;
}

void OrderEntry::operationEncode(hls::stream<orderEntryOperation_t> &operationStream,
                                 hls::stream<orderEntryOperationEncode_t> &operationEncodeStream)
{
#pragma HLS PIPELINE II=1 style=flp

    orderEntryOperation_t operation;
    orderEntryOperationEncode_t operationEncode;
    ap_uint<80> orderIdEncode, quantityEncode, priceEncode;

    if(!operationStream.empty())
    {
        operation = operationStream.read();

        orderIdEncode = uint32ToAscii(operation.orderId);
        quantityEncode = uint32ToAscii(operation.quantity);
        priceEncode = uint32ToAscii(operation.price);

        operationEncode.timestamp = operation.timestamp;
        operationEncode.opCode = operation.opCode;
        operationEncode.symbolIndex = operation.symbolIndex;
        operationEncode.orderId = orderIdEncode;
        operationEncode.quantity = quantityEncode;
        operationEncode.price = priceEncode;
        operationEncode.direction = operation.direction;

        operationEncodeStream.write(operationEncode);
    }

    return;
}

void OrderEntry::openListenPortTcp(hls::stream<ipTcpListenPortPack_t> &listenPortStream,
                                   hls::stream<ipTcpListenStatusPack_t> &listenStatusStream)
{
#pragma HLS PIPELINE II=1 style=flp

    ipTcpListenPortPack_t listenPortPack;
    ipTcpListenStatusPack_t listenStatusPack;
    bool listenDone = false;

    static ap_uint<2> state=0;
#pragma HLS RESET variable=state

    switch(state)
    {
        case 0:
        {
            // TODO: remove hard coded listen port
            listenPortPack.data = 7;
            listenPortPack.keep = 0x3;
            listenPortPack.last = 1;
            listenPortStream.write(listenPortPack);
            state = 1;
            break;
        }
        case 1:
        {
            if(!listenStatusStream.empty())
            {
                listenStatusPack = listenStatusStream.read();
                if(1 == listenStatusPack.data)
                {
                    state = 2;
                }
                else
                {
                    state = 0;
                }
            }
            break;
        }
        case 2:
        {
            // IDLE
            break;
        }
    } // switch
}

void OrderEntry::openActivePortTcp(ap_uint<32> &regControl,
                                   ap_uint<32> &regDestAddress,
                                   ap_uint<32> &regDestPort,
                                   ap_uint<32> &regDebug,
                                   hls::stream<ipTuplePack_t> &openConnectionStream,
                                   hls::stream<ipTcpConnectionStatusPack_t> &connectionStatusStream,
                                   hls::stream<ipTcpCloseConnectionPack_t> &closeConnectionStream,
                                   hls::stream<ipTcpTxStatusPack_t> &txStatusStream)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    ipTuple_t tuple;
    ipTuplePack_t tuplePack;
    ipTcpTxStatus_t txStatus;
    ipTcpTxStatusPack_t txStatusPack;
    ipTcpCloseConnectionPack_t closeConnectionPack;
    ipTcpConnectionStatusPack_t connectionStatusPack;

    enum stateType {IDLE, INIT_CON, WAIT_CON, ACTIVE_CON};
    static stateType state=IDLE;
    static ipTcpConnectionStatus_t connectionStatus;
    static ap_uint<1>  statusConnected=0;
    static ap_uint<16> statusLength=0;
    static ap_uint<30> statusSpace=0;
    static ap_uint<2>  statusError=0;
    static ap_uint<16> statusSessionID=0;

    static ap_uint<32> countDebug=0;

    if(!txStatusStream.empty())
    {
        txStatusPack = txStatusStream.read();
        intf.ipTcpTxStatusStreamUnpack(&txStatusPack, &txStatus);
        statusLength = txStatus.length;
        statusSpace = txStatus.space;
        statusError = txStatus.error;
    }

    switch(state)
    {
        case IDLE:
        {
            if(OE_TCP_CONNECT & regControl)
            {
                countDebug = (countDebug | 0x00000001);
                state = INIT_CON;
            }
            break;
        }
        case INIT_CON:
        {
            countDebug = (countDebug | 0x00000020);
            tuple.address = regDestAddress;
            tuple.port = regDestPort;
            intf.ipTuplePack(&tuple, &tuplePack);
            tuplePack.last = 1;
            tuplePack.keep = 0x3F;
            openConnectionStream.write(tuplePack);

            state = WAIT_CON;
            break;
        }
        case WAIT_CON:
        {
            countDebug = (countDebug | 0x00000300);
            if(!connectionStatusStream.empty())
            {
                countDebug = (countDebug | 0x00004000);
                connectionStatusPack = connectionStatusStream.read();
                intf.ipTcpConnectionStatusUnpack(&connectionStatusPack, &connectionStatus);
                if(connectionStatus.success)
                {
                    countDebug = (countDebug | 0x00050000);
                    state = ACTIVE_CON;
                    statusConnected = 0x1;
                    statusLength = 0x0;
                    statusSpace = 0xffff;
                    statusError = TXSTATUS_SUCCESS;
                    statusSessionID = connectionStatus.sessionID;
                }
            }
            // This code added to allow reconnect or disconnect to get out of WAIT_CON state
            // Note 0x007 instead of 0x006 to show this path was taken.
            if(0 == (OE_TCP_CONNECT & regControl))
            {
                countDebug = (countDebug | 0x00700000);
                state = IDLE;
                statusConnected = 0x0;
                statusLength = 0x0;
                statusSpace = 0x0;
                statusError = TXSTATUS_CLOSED;
            }
            break;
        }
        case ACTIVE_CON:
        {
            countDebug = (countDebug | 0x00600000);
            if(0 == (OE_TCP_CONNECT & regControl))
            {
                countDebug = (countDebug | 0x07000000);
                closeConnectionPack.data = connectionStatus.sessionID;
                closeConnectionPack.keep = 0x3;
                closeConnectionPack.last = 1;
                closeConnectionStream.write(closeConnectionPack);
                state = IDLE;
                statusConnected = 0x0;
                statusLength = 0x0;
                statusSpace = 0x0;
                statusError = TXSTATUS_CLOSED;
            }
            break;
        }
        default:
        {
            countDebug = (countDebug | 0x80000000);
            state = IDLE;
            break;
        }
    }

    // single point of update for private struct
    mConnectionStatus.connected = statusConnected;
    mConnectionStatus.length = statusLength;
    mConnectionStatus.space = statusSpace;
    mConnectionStatus.error = statusError;
    mConnectionStatus.sessionID = statusSessionID;

    regDebug = countDebug;
}

void OrderEntry::notificationHandlerTcp(ap_uint<32> &regNotification,
                                        ap_uint<32> &regReadRequest,
                                        hls::stream<ipTcpNotificationPack_t> &notificationStream,
                                        hls::stream<ipTcpReadRequestPack_t> &readRequestStream)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    ipTcpNotification_t notification;
    ipTcpNotificationPack_t notificationPack;
    ipTcpReadRequest_t readRequest;
    ipTcpReadRequestPack_t readRequestPack;

    static ap_uint<32> countNotification=0;
    static ap_uint<32> countReadRequest=0;

    if(!notificationStream.empty())
    {
        notificationPack = notificationStream.read();
        ++countNotification;
        intf.ipTcpNotificationUnpack(&notificationPack, &notification);
        if(notification.length != 0)
        {
            readRequest.sessionID = notification.sessionID;
            readRequest.length = notification.length;
            intf.ipTcpReadRequestPack(&readRequest, &readRequestPack);
            readRequestPack.last = 1;
            readRequestPack.keep = 0x3F;
            readRequestStream.write(readRequestPack);
            ++countReadRequest;
        }
    }

    regNotification = countNotification;
    regReadRequest = countReadRequest;
}

void OrderEntry::serverProcessTcp(ap_uint<32> &regRxData,
                                  ap_uint<32> &regRxMeta,
                                  hls::stream<ipTcpRxMetaPack_t> &rxMetaStream,
                                  hls::stream<ipTcpRxDataPack_t> &rxDataStream)
{
#pragma HLS PIPELINE II=1 style=flp

    ipTcpRxMetaPack_t rxMetaPack;
    ap_uint<16> sessionID;
    ap_axiu<64,0,0,0> currWord;

    static ap_uint<1> state=0;

    static ap_uint<32> countRxData=0;
    static ap_uint<32> countRxMeta=0;

    switch(state)
    {
        case 0:
        {
            if(!rxMetaStream.empty())
            {
                rxMetaPack = rxMetaStream.read();
                ++countRxMeta;
                sessionID = rxMetaPack.data;
                state = 1;
            }
            break;
        }
        case 1:
        {
            if(!rxDataStream.empty())
            {
                currWord = rxDataStream.read();
                ++countRxData;
                if(currWord.last)
                {
                    state = 0;
                }
            }
            break;
        }
    }

    regRxData = countRxData;
    regRxMeta = countRxMeta;
}

void OrderEntry::operationProcessTcp(ap_uint<32> &regControl,
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
                                     hls::stream<ipTcpTxDataPack_t> &txDataStream)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    orderEntryOperationEncode_t operationEncode;
    orderEntryMessagePack_t messagePack;
    ipTcpTxMeta_t txMeta;
    ipTcpTxMetaPack_t txMetaPack;

    ap_uint<16> sessionID;
    ap_uint<16> length;
    ap_uint<64> frameData;
    ap_uint<32> rangeIndexHigh, rangeIndexLow;
    ap_axiu<64,0,0,0> messageWord;
    ap_uint<24> orderIdSum, timestampSum, quantitySum, priceSum, messageSum;
    ap_uint<1> validSum;

    static ap_uint<32> countProcessOperation=0;
    static ap_uint<32> countTxOrder=0;
    static ap_uint<32> countTxData=0;
    static ap_uint<32> countTxMeta=0;
    static ap_uint<32> countTxDrop=0;
    static ap_uint<32> countDebug=0;

    if(!operationEncodeStream.empty())
    {
        operationEncode = operationEncodeStream.read();
        ++countProcessOperation;

        // egress message is transmitted on data interface as 64b words
        messageWord.last = 0;
        messageWord.strb = 0xFF;
        messageWord.keep = 0xFF;
        txMetaPack.last = 0;
        txMetaPack.keep = 0x7F;

        // currently static as we send a fixed message size
        sessionID = mConnectionStatus.sessionID;
        length = OE_MSG_LEN_BYTES;

        // apply field updates to overwrite template fields
        messageTemplate[3].range(63,0) = operationEncode.orderId.range(79,16);
        messageTemplate[4].range(63,48) = operationEncode.orderId.range(15,0);
        messageTemplate[9].range(63,0) = operationEncode.timestamp;
        messageTemplate[15].range(31,0) = operationEncode.orderId.range(79,48);
        messageTemplate[16].range(63,16) = operationEncode.orderId.range(47,0);
        messageTemplate[17].range(47,0) = operationEncode.quantity.range(79,32);
        messageTemplate[18].range(63,32) = operationEncode.quantity.range(31,0);
        messageTemplate[19].range(23,0) = operationEncode.price.range(79,56);
        messageTemplate[20].range(63,8) = operationEncode.price.range(55,0);

        // if checksum generation is enabled we calculate the partial sum for
        // the payload here and send to TCP kernel via metadata interface, this
        // reduces latency as TCP can begin sending in cut-through mode rather
        // than buffer the full packet in store and forward mode
        if(OE_TCP_GEN_SUM & regControl)
        {
            timestampSum = operationEncode.timestamp.range(15,0);
            timestampSum += operationEncode.timestamp.range(31,16);
            timestampSum = (timestampSum + (timestampSum>>16)) & 0xFFFF;
            timestampSum += operationEncode.timestamp.range(47,32);
            timestampSum = (timestampSum + (timestampSum>>16)) & 0xFFFF;
            timestampSum += operationEncode.timestamp.range(63,48);
            timestampSum = (timestampSum + (timestampSum>>16)) & 0xFFFF;

            orderIdSum = operationEncode.orderId.range(15,0);
            orderIdSum += operationEncode.orderId.range(31,16);
            orderIdSum = (orderIdSum + (orderIdSum>>16)) & 0xFFFF;
            orderIdSum += operationEncode.orderId.range(47,32);
            orderIdSum = (orderIdSum + (orderIdSum>>16)) & 0xFFFF;
            orderIdSum += operationEncode.orderId.range(63,48);
            orderIdSum = (orderIdSum + (orderIdSum>>16)) & 0xFFFF;
            orderIdSum += operationEncode.orderId.range(79,64);
            orderIdSum = (orderIdSum + (orderIdSum>>16)) & 0xFFFF;

            quantitySum = operationEncode.quantity.range(15,0);
            quantitySum += operationEncode.quantity.range(31,16);
            quantitySum = (quantitySum + (quantitySum>>16)) & 0xFFFF;
            quantitySum += operationEncode.quantity.range(47,32);
            quantitySum = (quantitySum + (quantitySum>>16)) & 0xFFFF;
            quantitySum += operationEncode.quantity.range(63,48);
            quantitySum = (quantitySum + (quantitySum>>16)) & 0xFFFF;
            quantitySum += operationEncode.quantity.range(79,64);
            quantitySum = (quantitySum + (quantitySum>>16)) & 0xFFFF;

            // the price field is not 16b aligned, need to shift by one byte
            priceSum = (operationEncode.price.range(7,0) << 8);
            priceSum += operationEncode.price.range(23,8);
            priceSum = (priceSum + (priceSum>>16)) & 0xFFFF;
            priceSum += operationEncode.price.range(39,24);
            priceSum = (priceSum + (priceSum>>16)) & 0xFFFF;
            priceSum += operationEncode.price.range(55,40);
            priceSum = (priceSum + (priceSum>>16)) & 0xFFFF;
            priceSum += operationEncode.price.range(71,56);
            priceSum = (priceSum + (priceSum>>16)) & 0xFFFF;
            priceSum += operationEncode.price.range(79,72);
            priceSum = (priceSum + (priceSum>>16)) & 0xFFFF;

            // merge template message partial sum with dynamic field updates
            messageSum = messageTemplateSum;
            messageSum += orderIdSum;
            messageSum = (messageSum + (messageSum>>16)) & 0xFFFF;
            messageSum += timestampSum;
            messageSum = (messageSum + (messageSum>>16)) & 0xFFFF;
            messageSum += orderIdSum;
            messageSum = (messageSum + (messageSum>>16)) & 0xFFFF;
            messageSum += quantitySum;
            messageSum = (messageSum + (messageSum>>16)) & 0xFFFF;
            messageSum += priceSum;
            messageSum = (messageSum + (messageSum>>16)) & 0xFFFF;
            validSum = 1;
        }
        else
        {
            messageSum = 0;
            validSum = 0;
        }

        if((mConnectionStatus.connected) &&
           (length <= mConnectionStatus.space) &&
           (TXSTATUS_SUCCESS == mConnectionStatus.error))
        {
            // send the meta data
            txMeta.validSum = validSum;
            txMeta.subSum = messageSum;
            txMeta.sessionID = sessionID;
            txMeta.length = length;
            intf.ipTcpTxMetaPack(&txMeta, &txMetaPack);
            txMetaPack.last = 1;
            txMetaStream.write(txMetaPack);
            ++countTxMeta;

loop_message_frame:
            for(int frameCount=0; frameCount<OE_MSG_NUM_FRAME; frameCount++)
            {
                // load frame from template
                frameData = messageTemplate[frameCount];

                // reverse for network byte order in egress message payload
                messageWord.data = byteReverse(frameData);

                // instruct tcp kernel if this is the last frame in payload
                if(31 == frameCount)
                {
                    messageWord.last = 1;
                    ++countDebug;
                }

                // forward frame to tcp kernel
                txDataStream.write(messageWord);
                ++countTxData;

                // add frame to message capture
                rangeIndexLow = (frameCount*64);
                rangeIndexHigh = (rangeIndexLow+63);
                if(rangeIndexHigh < 1024)
                {
                    // TODO: add 2048b message capture support, truncate for now
                    messagePack.data.range(rangeIndexHigh,rangeIndexLow) = frameData;
                }
            }

            ++countTxOrder;

            // message capture recorded in register map for host visibility
            // check if host has capture freeze control enabled before updating
            if(0 == (OE_CAPTURE_FREEZE & regCaptureControl))
            {
                regCaptureBuffer = messagePack.data;
            }
        }
        else
        {
            ++countTxDrop;
        }
    }

    regProcessOperation = countProcessOperation;
    regTxOrder = countTxOrder;
    regTxData = countTxData;
    regTxMeta = countTxMeta;
    regTxDrop = countTxDrop;

    regTxStatus.range(31,31) = mConnectionStatus.connected;
    regTxStatus.range(30,29) = mConnectionStatus.error;
    regTxStatus.range(28,0)  = mConnectionStatus.space;

    return;
}

ap_uint<64> OrderEntry::byteReverse(ap_uint<64> inputData)
{
#pragma HLS PIPELINE II=1 style=flp

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

ap_uint<80> OrderEntry::uint32ToAscii(ap_uint<32> inputData)
{
#pragma HLS PIPELINE II=1 style=flp

    ap_uint<40> bcd=0;
    ap_uint<80> outputAscii("30303030303030303030", 16);

    ap_uint<4> bcdDigit[10]={0,0,0,0,0,0,0,0,0,0};
    ap_uint<4> bcdNext[10]={0,0,0,0,0,0,0,0,0,0};
    ap_uint<1> carryIn[10]={0,0,0,0,0,0,0,0,0,0};
    ap_uint<1> carryOut[10]={0,0,0,0,0,0,0,0,0,0};

#pragma HLS ARRAY_PARTITION variable=bcdDigit complete
#pragma HLS ARRAY_PARTITION variable=bcdNext complete
#pragma HLS ARRAY_PARTITION variable=carryIn complete
#pragma HLS ARRAY_PARTITION variable=carryOut complete

    // TODO: comment required to explain the algorithm but technique is from
    //       "XAPP029 - Serial Code Conversion between BCD and Binary"

loop_bcd:
    for(int i=0; i<32; i++)
    {
        carryIn[0] = inputData.range(31-i,31-i);

loop_carry:
        for(int j=1; j<10; j++)
        {
            carryIn[j] = carryOut[j-1];
        }

loop_digit:
        for(int j=0; j<10; j++)
        {
            bcdDigitiser(carryIn[j], bcdDigit[j], bcdNext[j], carryOut[j]);
        }
    }

loop_ascii:
    for(int i=0; i<40; i+=4)
    {
        outputAscii.range(((i<<1)+3),(i<<1)) = bcdDigit[i>>2];
    }

    return outputAscii;
}

void OrderEntry::bcdDigitiser(ap_uint<1> &carryIn,
                              ap_uint<4> &bcdDigit,
                              ap_uint<4> &bcdNext,
                              ap_uint<1> &carryOut)
{
#pragma HLS INLINE

    bcdDigit = (bcdNext << 1) | carryIn;

    switch(bcdDigit)
    {
        case 5:
            bcdNext = 0;
            carryOut = 1;
            break;
        case 6:
            bcdNext = 1;
            carryOut = 1;
            break;
        case 7:
            bcdNext = 2;
            carryOut = 1;
            break;
        case 8:
            bcdNext = 3;
            carryOut = 1;
            break;
        case 9:
            bcdNext = 4;
            carryOut = 1;
            break;
        default:
            bcdNext = bcdDigit;
            carryOut = 0;
    }

    return;
}

