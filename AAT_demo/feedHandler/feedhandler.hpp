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

#ifndef FEEDHANDLER_H
#define FEEDHANDLER_H

#include "hls_stream.h"
#include "ap_int.h"
#include "ap_shift_reg.h"
#include "aat_defines.hpp"
#include "aat_interfaces.hpp"

#define FH_LOOKUP_DISABLE (1<<5)
#define FH_FILTER_DISABLE (1<<4)
#define FH_ECHO_ENABLE    (1<<3)
#define FH_RESET_COUNT    (1<<2)
#define FH_RESET_DATA     (1<<1)
#define FH_HALT           (1<<0)
#define FH_CAPTURE_FREEZE (1<<31)

typedef struct feedHandlerRegControl_t
{
    ap_uint<32> control;
    ap_uint<32> capture;
    ap_uint<32> reserved02;
    ap_uint<32> reserved03;
    ap_uint<32> reserved04;
    ap_uint<32> reserved05;
    ap_uint<32> reserved06;
    ap_uint<32> reserved07;
} feedHandlerRegControl_t;

typedef struct feedHandlerRegStatus_t
{
    ap_uint<32> processWord;
    ap_uint<32> processPacket;
    ap_uint<32> processBinary;
    ap_uint<32> processFix;
    ap_uint<32> txOperation;
    ap_uint<32> rxEvent;
    ap_uint<32> reserved06;
    ap_uint<32> reserved07;
} feedHandlerRegStatus_t;

// container wrapping symbol map used in order to apply ARRAY_PARTITION pragma
// for top level interface in vitis_hls
typedef struct regSymbolMapContainer
{
    ap_uint<32> symbols[NUM_SYMBOL];

    regSymbolMapContainer()
    {
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION variable=symbols complete
    }

} regSymbolMapContainer_t;

class FeedHandler
{
public:

    void udpPacketHandler(ap_uint<32> &regProcessWord,
                          ap_uint<32> &regProcessPacket,
                          hls::stream<axiWordExt_t> &inputStream,
                          hls::stream<axiWord_t> &outputStream);

    void binaryPacketHandler(ap_uint<32> &regProcessBinary,
                             hls::stream<axiWord_t> &inputStream,
                             hls::stream<axiWord_t> &outputStream,
                             hls::stream<templateId_t> &templateIdStream);

    void fixDecoderTop(ap_uint<32> &regProcessFix,
                       hls::stream<axiWord_t> &inputStream,
                       hls::stream<templateId_t> &templateIdStream,
                       hls::stream<securityId_t> &securityIdStream,
                       hls::stream<orderBookOperation_t> &operationStream);

    void symbolLookup(ap_uint<32> &regCaptureControl,
                      ap_uint<32> &regTxOperation,
                      ap_uint<32> regSymbolMap[NUM_SYMBOL],
                      ap_uint<256> &regCaptureBuffer,
                      hls::stream<securityId_t> &securityIdStream,
                      hls::stream<orderBookOperation_t> &operationStream,
                      hls::stream<orderBookOperationPack_t> &operationStreamPack);

    void eventHandler(ap_uint<32> &regRxEvent,
                      hls::stream<clockTickGeneratorEvent_t> &eventStream);

private:

    void fixDecoder(ap_uint<32> &regProcessFix,
                    hls::stream<axiWord_t> &inputStream,
                    hls::stream<templateId_t> &templateIdStream,
                    hls::stream<securityId_t> &securityIdStream,
                    hls::stream<orderBookOperation_t> &operationStream);

    void MDIncrementalRefreshBook32(axiWord_t &fixData,
                                    hls::stream<securityId_t> &securityIdStream,
                                    hls::stream<orderBookOperation_t> &operationStream);

    // code body for templated functions located in header file, the compiler
    // should be able to see the implementation in order to generate for all
    // specialisations

    template <typename T, int W>
    void binaryStreamAlign(ap_uint<8> offset,
                           hls::stream<T>& input,
                           hls::stream<T>& output)
    {
#pragma HLS INLINE

        enum stateIdType {PKG, REMAINDER};
        static stateIdType stateId=PKG;
        static bool firstWord=true;
        static T prevWord;

        T currWord;
        T sendWord;

        sendWord.last = 0;
        switch(stateId)
        {
            case PKG:
            {
                if(!input.empty())
                {
                    input.read(currWord);

                    if(!firstWord or (offset == 0))
                    {
                        if(offset == 0)
                        {
                            sendWord = currWord;
                        }
                        else
                        {
                            sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
                            sendWord.data((W-1), W-(8*offset)) = currWord.data((8*offset)-1, 0);
                            sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
                            sendWord.keep((W/8-1), (W/8)-offset) = currWord.keep(offset-1, 0);
                            sendWord.last = (currWord.keep((W/8-1), offset) == 0);
                        }

                        output.write(sendWord);
                    }

                    prevWord = currWord;
                    firstWord = false;
                    if(currWord.last)
                    {
                        firstWord = true;
                        if(!sendWord.last)
                        {
                            stateId = REMAINDER;
                        }
                    }
                }
                break;
            }
            case REMAINDER:
            {
                sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
                sendWord.data((W-1), W-(8*offset)) = 0;
                sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
                sendWord.keep((W/8-1), (W/8)-offset) = 0;
                sendWord.last = 1;

                output.write(sendWord);
                stateId = PKG;
                break;
            }
        }

        return;
    }

    template <typename T, int W>
    void fixStreamAlign(ap_uint<8> offset,
                        hls::stream<T>& input,
                        hls::stream<T>& output)
    {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1 style=flp

        enum stateIdType {PKG, REMAINDER};
        static stateIdType stateId=PKG;
        static bool firstWord=true;
        static T prevWord;

        T currWord;
        T sendWord;

        sendWord.last = 0;
        switch(stateId)
        {
            case PKG:
            {
                if(!input.empty())
                {
                    input.read(currWord);

                    if(!firstWord or offset == 0)
                    {
                        if(offset == 0)
                        {
                            sendWord = currWord;
                        }
                        else
                        {
                            sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
                            sendWord.data((W-1), W-(8*offset)) = currWord.data((8*offset)-1, 0);
                            sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
                            sendWord.keep((W/8-1), (W/8)-offset) = currWord.keep(offset-1, 0);
                            sendWord.last = (currWord.keep((W/8-1), offset) == 0);
                        }

                        output.write(sendWord);
                    }

                    prevWord = currWord;
                    firstWord = false;
                    if(currWord.last)
                    {
                        firstWord = true;
                        if(!sendWord.last)
                        {
                            stateId = REMAINDER;
                        }
                    }
                }
                break;
            }
            case REMAINDER:
            {
                sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
                sendWord.data((W-1), W-(8*offset)) = 0;
                sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
                sendWord.keep((W/8-1), (W/8)-offset) = 0;
                sendWord.last = 1;

                output.write(sendWord);
                stateId = PKG;
                break;
            }
        }

        return;
    }

};

#endif
