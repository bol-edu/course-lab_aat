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
#include "feedhandler.hpp"

/**
 * FeedHandler Core
 */
void FeedHandler::udpPacketHandler(ap_uint<32> &regProcessWord,
                                   ap_uint<32> &regProcessPacket,
                                   hls::stream<axiWordExt_t> &inputStream,
                                   hls::stream<axiWord_t> &outputStream)

{
#pragma HLS PIPELINE II=1 style=flp

    enum stateIdType {DECODE, FWDUDP, REMAINDER};
    static stateIdType stateId=DECODE;

    axiWordExt_t currWordExt;
    axiWord_t currWord, sendWord, prevData;

    static ap_shift_reg<axiWord_t, 1> Sreg;
    static ap_uint<1> wordCount=0;

    static ap_uint<32> countProcessWord=0;
    static ap_uint<32> countProcessPacket=0;

    switch(stateId)
    {
        case DECODE:
        {
            if(!inputStream.empty())
            {
                inputStream.read(currWordExt);
                currWord.data = currWordExt.data;
                currWord.keep = currWordExt.keep;
                currWord.last = currWordExt.last;
                ++countProcessWord;

                if(1 == wordCount)
                {
                    wordCount = 0;
                    Sreg.shift(currWord);
                    stateId = FWDUDP;
                } else
                {
                    wordCount++;
                }
            }

            break;
        }
        case FWDUDP:
        {
            if(!inputStream.empty() && !outputStream.full())
            {
                inputStream.read(currWordExt);
                currWord.data = currWordExt.data;
                currWord.keep = currWordExt.keep;
                currWord.last = currWordExt.last;
                prevData = Sreg.read(0);
                sendWord.data.range(31,0) = prevData.data.range(63,32);
                sendWord.data.range(63,32) = currWord.data.range(31,0);
                sendWord.last = 0x0;
                sendWord.keep = 0xFF;
                outputStream.write(sendWord);
                // breaking up the shift and read improves timing results
                Sreg.shift(currWord);
                if(currWord.last)
                {
                    stateId = REMAINDER;
                }
            }

            break;
        }
        case REMAINDER:
        {
            prevData = Sreg.read(0);
            sendWord.data.range(31,0) = prevData.data.range(63,32);
            sendWord.data.range(63,32) = 0x0;
            sendWord.last = 0x1;
            sendWord.keep = 0xF;
            outputStream.write(sendWord);
            ++countProcessPacket;
            stateId = DECODE;
            break;
        }
    }

    regProcessWord = countProcessWord;
    regProcessPacket = countProcessPacket;

    return;
}

void FeedHandler::binaryPacketHandler(ap_uint<32> &regProcessBinary,
                                      hls::stream<axiWord_t> &inputStream,
                                      hls::stream<axiWord_t> &outputStream,
                                      hls::stream<templateId_t> &templateIdStream)
{
#pragma HLS PIPELINE II=1 style=flp

    enum stateIdType {DECODE, FWDFIX};
    static stateIdType stateId=DECODE;

    axiWord_t currWord, sendWord;
    ap_uint<16> tmplID, schemaID, version;

    static ap_uint<2> wordCount= 0;
    static ap_uint<16> msgSize=0;
    static ap_uint<16> receivedBytes=0;
    static ap_uint<16> blockLength;
    static ap_uint<8> offset=0;

    static ap_uint<32> countProcessBinary=0;

    static hls::stream<axiWord_t> inputStreamAlign;

#pragma HLS STREAM variable=inputStreamAlign depth=1024

    binaryStreamAlign<axiWord_t, 64>(offset, inputStream, inputStreamAlign);

    switch(stateId)
    {
        case DECODE:
        {
            if(!inputStreamAlign.empty())
            {
                inputStreamAlign.read(currWord);
                switch(wordCount)
                {
                    case 0:
                        msgSize = currWord.data.range(15,0);
                        blockLength = currWord.data.range(31,16);
                        tmplID = currWord.data.range(47,32);
                        // TODO: add a check here for a valid template, go to drop state if not supported
                        templateIdStream.write(tmplID);
                        schemaID = currWord.data.range(63,48);
                        break;
                    case 1:
                        version = currWord.data.range(15,0);
                        sendWord.data.range(63,16) = currWord.data.range(63,16); // shifter will handle the shift
                        sendWord.data.range(15,0) = 0x0;
                        sendWord.last = 0;
                        sendWord.keep = 0xFF;
                        outputStream.write(sendWord);
                        receivedBytes = 14; // this is 8 from header and 6 from Fix message
                        break;
                }

                if(wordCount == 1)
                {
                    wordCount = 0;
                    stateId = FWDFIX;
                }
                else
                {
                    wordCount++;
                }
            }

            break;
        }
        case FWDFIX:
        {
            if(!inputStreamAlign.empty() && !outputStream.full())
            {
                inputStreamAlign.read(currWord);
                receivedBytes = receivedBytes + 8;
                if(receivedBytes >= msgSize or currWord.last)
                {
                    if(currWord.last)
                    {
                        offset = 0;
                    }
                    else
                    {
                        offset = msgSize.range(2,0); // (msgSize % 8)
                    }

                    // let the fix decoder know this is the last word in the current fix message
                    currWord.last = 1;
                    outputStream.write(currWord);
                    ++countProcessBinary;
                    stateId = DECODE;
                }
                else
                {
                    outputStream.write(currWord);
                }
            }

            break;
        }
    }

    regProcessBinary = countProcessBinary;

    return;
}

void FeedHandler::fixDecoderTop(ap_uint<32> &regProcessFix,
                                hls::stream<axiWord_t> &inputStream,
                                hls::stream<templateId_t> &templateIdStream,
                                hls::stream<securityId_t> &securityIdStream,
                                hls::stream<orderBookOperation_t> &operationStream)
{
#pragma HLS DATAFLOW disable_start_propagation

    static ap_uint<8> offset;
    static hls::stream<axiWord_t> fixMsgFifoAlign;

#pragma HLS STREAM variable=fixMsgFifoAlign depth=1024

    fixStreamAlign<axiWord_t, 64>(2, inputStream, fixMsgFifoAlign);
    fixDecoder(regProcessFix,
               fixMsgFifoAlign,
               templateIdStream,
               securityIdStream,
               operationStream);

    return;
}

void FeedHandler::symbolLookup(ap_uint<32> &regCaptureControl,
                               ap_uint<32> &regTxOperation,
                               ap_uint<32> regSymbolMap[NUM_SYMBOL],
                               ap_uint<256> &regCaptureBuffer,
                               hls::stream<securityId_t> &securityIdStream,
                               hls::stream<orderBookOperation_t> &operationStream,
                               hls::stream<orderBookOperationPack_t> &operationStreamPack)
{
#pragma HLS PIPELINE II=1 style=flp

    mmInterface intf;
    securityId_t securityId;
    orderBookOperation_t operation;
    orderBookOperationPack_t operationPack;
    bool indexMatch;

    static ap_uint<32> countTxOperation=0;

    if(!securityIdStream.empty() && !operationStream.empty())
    {
        securityIdStream.read(securityId);
        operationStream.read(operation);

        indexMatch = false;
        for(ap_uint<9> i=0; i<NUM_SYMBOL; i++)
        {
#pragma HLS UNROLL
            if(securityId == regSymbolMap[i])
            {
                operation.symbolIndex = i;
                indexMatch = true;
            }
        }

        if(indexMatch)
        {
            intf.orderBookOperationPack(&operation, &operationPack);
            operationStreamPack.write(operationPack);

            if(0 == (FH_CAPTURE_FREEZE & regCaptureControl))
            {
                regCaptureBuffer = operationPack.data;
            }

            ++countTxOperation;
        }
    }

    regTxOperation = countTxOperation;

    return;
}

void FeedHandler::eventHandler(ap_uint<32> &regRxEvent,
                               hls::stream<clockTickGeneratorEvent_t> &eventStream)
{
#pragma HLS PIPELINE II=1 style=flp

    clockTickGeneratorEvent_t tickEvent;

    static ap_uint<32> countRxEvent=0;

    if(!eventStream.empty())
    {
        tickEvent = eventStream.read();
        ++countRxEvent;

        // event notification has been received from programmable clock tick
        // generator, handling currently limited to incrementing a counter,
        // placeholder for user to extend with custom event handling code
    }

    regRxEvent = countRxEvent;

    return;
}

// private
void FeedHandler::fixDecoder(ap_uint<32> &regProcessFix,
                             hls::stream<axiWord_t> &inputStream,
                             hls::stream<templateId_t> &templateIdStream,
                             hls::stream<securityId_t> &securityIdStream,
                             hls::stream<orderBookOperation_t> &operationStream)
{
#pragma HLS PIPELINE II=1 style=flp

    enum stateIdType {IDLE, DECODE};
    static stateIdType stateId=IDLE;

    axiWord_t currWord;

    static ap_uint<16> currTmplID=0;
    static ap_uint<32> countProcessFix=0;

    switch(stateId)
    {
        case IDLE:
        {
            if(!templateIdStream.empty())
            {
                templateIdStream.read(currTmplID);
                stateId = DECODE;
            }

            break;
        }
        case DECODE:
        {
            if(!inputStream.empty())
            {
                inputStream.read(currWord);
                switch(currTmplID)
                {
                    // support currently limited to single message type
                    case 32:
                        MDIncrementalRefreshBook32(currWord,
                                                   securityIdStream,
                                                   operationStream);
                        break;
                    default:
                        break;
                }

                if(currWord.last)
                {
                    ++countProcessFix;
                    stateId = IDLE;
                }
            }

            break;
        }
    }

    regProcessFix = countProcessFix;

    return;
}

void FeedHandler::MDIncrementalRefreshBook32(axiWord_t &fixData,
                                             hls::stream<securityId_t> &securityIdStream,
                                             hls::stream<orderBookOperation_t> &operationStream)
{
#pragma HLS INLINE

    enum stateIdType {DECODE, GROUPDECODE, GROUP2SETUP, GROUPDECODE2};
    static stateIdType stateId=DECODE;

    static ap_uint<16>  wordCount=0;
    static ap_uint<16>  groupWordCount=0;
    static ap_uint<16>  groupWordCount2=0;
    static ap_uint<16>  numOfGroups=0;
    static ap_uint<16>  numOfGroups2=0;
    static ap_uint<64>  time;
    static ap_uint<8>   matchEvent;
    static ap_uint<16>  groupBlockLen;
    static ap_uint<8>   groupRepeat;
    static ap_uint<16>  groupBlockLen2;
    static ap_uint<8>   groupRepeat2;
    static ap_uint<32>  rptSeq;
    static ap_uint<64>  orderPriority;
    static ap_int<64>   mantissa;
    static ap_int<64>   price64bit;
    static ap_int<32>   orderQty;
    static ap_int<32>   securityID;
    static ap_uint<8>   updateAction;
    static ap_uint<32>  entrySize;
    static ap_uint<8>   priceLevel;
    static char         entryType;
    static ap_uint<64>  orderID;
    static ap_int<32>   displyQty;
    static ap_uint<8>   refId;
    static ap_uint<8>   orderUpdate;

    static ap_shift_reg<axiWord_t, 1> Sreg;
    static axiWord_t prevData;
    static orderBookOperation_t operation;

    static ap_uint<32> countTxOperation=0;

    switch(stateId)
    {
        case DECODE:
        {
            switch(wordCount)
            {
                case 0:
                {
                    // header is first 32 bits but not required
                    time.range(31,0) = fixData.data.range(63,32);
                    break;
                }
                case 1:
                {
                    time.range(63,32) = fixData.data.range(31,0);
                    matchEvent = fixData.data.range(39,32);
                    // gap of 16 zeros, block size is 11 bytes
                    groupBlockLen.range(7,0) = fixData.data.range(63,56); // total length of single group
                    break;
                }
                case 2:
                {
                    // decode current word
                    groupBlockLen.range(15,8) = fixData.data.range(7,0);  // total length of single group
                    groupRepeat = fixData.data.range(15,8); // total number of groups
                    // start receiving the first data in the repeating group at bit 16
                    Sreg.shift(fixData);
                    break;
                }
            }

            if(wordCount == 2)
            {
                wordCount = 0;
                stateId = GROUPDECODE;
            }
            else
            {
                wordCount++;
            }

            break;
        }
        case GROUPDECODE: // need to run a for loop groupRepeat times
        {
            switch(groupWordCount)
            {
                case 0:
                {
                    // decode current word
                    prevData = Sreg.read(0);
                    mantissa.range(47,0) = prevData.data.range(63,16);
                    mantissa.range(63,48) = fixData.data.range(15,0);
                    entrySize = fixData.data.range(47,16);
                    securityID.range(15,0) = fixData.data.range(63,48);
                    break;
                }
                case 1:
                {
                    // decode current word
                    securityID.range(31,16) = fixData.data.range(15,0);
                    rptSeq = fixData.data.range(47,16);
                    orderQty.range(15,0) = fixData.data.range(63,48);
                    // process previous word
                    price64bit = (mantissa * PRICE_EXPONENT);
                    break;
                }
                case 2:
                {
                    // decode current word
                    orderQty.range(31,16) = fixData.data.range(15,0);
                    priceLevel = fixData.data.range(23,16);
                    updateAction = fixData.data.range(31,24);
                    entryType = fixData.data.range(39,32);
                    break;
                }
                case 3:
                {
                    // decode current word
                    Sreg.shift(fixData);
                    // process previous word
                    operation.timestamp = time;
                    operation.direction = (entryType-0x30); // ascii to OB decimal encoding
                    operation.level = priceLevel;
                    operation.opCode = updateAction;
                    operation.quantity = entrySize;
                    operation.orderCount = orderQty;
                    operation.symbolIndex = 0;
                    operation.price = price64bit.range(31,0);
                    operation.orderId = 0x0;
                    securityIdStream.write(securityID);
                    operationStream.write(operation);
                    break;
                }
            }

            if(groupWordCount == 3)
            {
                groupWordCount = 0;
                numOfGroups++;
            }
            else
            {
                groupWordCount++;
            }

            if(numOfGroups == groupRepeat)
            {
                stateId = GROUP2SETUP;
                numOfGroups = 0;
            }

            break;
        }
        case GROUP2SETUP:
        {
            prevData = Sreg.shift(fixData, 0); // reading and shifting in 1 go
            groupBlockLen2 = prevData.data.range(31,16); // total length of single group

            // now have 40 zeros, 32 in prevData and 8 in current word
            groupRepeat2 = fixData.data.range(15,8); // total number of groups
            stateId = GROUPDECODE2;
            break;
        }
        case GROUPDECODE2:
        {
            switch(groupWordCount2)
            {
                case 0:
                {
                    prevData = Sreg.read(0);
                    orderID.range(47,0) = prevData.data.range(63,16);
                    orderID.range(63,48) = fixData.data.range(15,0);
                    orderPriority.range(47,0) = fixData.data.range(63,16);
                    break;
                }
                case 1:
                {
                    orderPriority.range(63,48) = fixData.data.range(15,0);
                    displyQty = fixData.data.range(47,16);
                    refId = fixData.data.range(55,48);
                    orderUpdate = fixData.data.range(63,56);
                    break;
                }
                case 2:
                {
                    // 0->15 are zeros
                    Sreg.shift(fixData);
                    break;
                }
            }

            if(2 == groupWordCount2)
            {
                groupWordCount2 = 0;
                numOfGroups2++;
            }
            else
            {
                groupWordCount2++;
            }

            if(numOfGroups2 == groupRepeat2)
            {
                stateId = DECODE;
                numOfGroups2 = 0;
            }

            break;
        }
    }

    return;
}
