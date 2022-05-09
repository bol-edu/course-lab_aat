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

#include "aat_defines.hpp"
#include "aat_interfaces.hpp"

void mmInterface::orderBookOperationPack(orderBookOperation_t *src,
                                         orderBookOperationPack_t *dest)
{
#pragma HLS INLINE

    dest->data.range(223,160) = src->timestamp;
    dest->data.range(159,152) = src->opCode;
    dest->data.range(151,144) = src->symbolIndex;
    dest->data.range(143,112) = src->orderId;
    dest->data.range(111,80)  = src->orderCount;
    dest->data.range(79,48)   = src->quantity;
    dest->data.range(47,16)   = src->price;
    dest->data.range(15,8)    = src->direction;
    dest->data.range(7,0)     = src->level;

    return;
}

void mmInterface::orderBookOperationUnpack(orderBookOperationPack_t *src,
                                           orderBookOperation_t *dest)
{
#pragma HLS INLINE

    dest->timestamp   = src->data.range(223,160);
    dest->opCode      = src->data.range(159,152);
    dest->symbolIndex = src->data.range(151,144);
    dest->orderId     = src->data.range(143,112);
    dest->orderCount  = src->data.range(111,80);
    dest->quantity    = src->data.range(79,48);
    dest->price       = src->data.range(47,16);
    dest->direction   = src->data.range(15,8);
    dest->level       = src->data.range(7,0);

    return;
}

void mmInterface::orderBookResponsePack(orderBookResponse_t *src,
                                        orderBookResponsePack_t *dest)
{
#pragma HLS INLINE

    dest->data.range(1023,968) = src->timestamp;
    dest->data.range(967,960)  = src->symbolIndex;
    dest->data.range(959,800)  = src->bidCount;
    dest->data.range(799,640)  = src->bidPrice;
    dest->data.range(639,480)  = src->bidQuantity;
    dest->data.range(479,320)  = src->askCount;
    dest->data.range(319,160)  = src->askPrice;
    dest->data.range(159,0)    = src->askQuantity;

    return;
}

void mmInterface::orderBookResponseUnpack(orderBookResponsePack_t *src,
                                          orderBookResponse_t *dest)
{
#pragma HLS INLINE

    dest->timestamp   = src->data.range(1023,968);
    dest->symbolIndex = src->data.range(967,960);
    dest->bidCount    = src->data.range(959,800);
    dest->bidPrice    = src->data.range(799,640);
    dest->bidQuantity = src->data.range(639,480);
    dest->askCount    = src->data.range(479,320);
    dest->askPrice    = src->data.range(319,160);
    dest->askQuantity = src->data.range(159,0);

    return;
}

void mmInterface::orderEntryOperationPack(orderEntryOperation_t *src,
                                          orderEntryOperationPack_t *dest)
{
#pragma HLS INLINE

    dest->data.range(183,120) = src->timestamp;
    dest->data.range(119,112) = src->opCode;
    dest->data.range(111,104) = src->symbolIndex;
    dest->data.range(103,72)  = src->orderId;
    dest->data.range(71,40)   = src->quantity;
    dest->data.range(39,8)    = src->price;
    dest->data.range(7,0)     = src->direction;

    return;
}

void mmInterface::orderEntryOperationUnpack(orderEntryOperationPack_t *src,
                                            orderEntryOperation_t *dest)
{
#pragma HLS INLINE

    dest->timestamp   = src->data.range(183,120);
    dest->opCode      = src->data.range(119,112);
    dest->symbolIndex = src->data.range(111,104);
    dest->orderId     = src->data.range(103,72);
    dest->quantity    = src->data.range(71,40);
    dest->price       = src->data.range(39,8);
    dest->direction   = src->data.range(7,0);

    return;
}

void mmInterface::ipTuplePack(ipTuple_t *src,
                              ipTuplePack_t *dest)
{
#pragma HLS INLINE

    dest->data.range(47,32) = src->port;
    dest->data.range(31,0)  = src->address;

    return;
}

void mmInterface::udpMetaPack(ipUdpMeta_t *src,
                              ipUdpMetaPack_t *dest)
{
#pragma HLS INLINE

    dest->data.range(128,128) = src->validSum;
    dest->data.range(127,112) = src->subSum;
    dest->data.range(111,96)  = src->length;
    dest->data.range(95,80)   = src->dstPort;
    dest->data.range(79,48)   = src->dstAddress;
    dest->data.range(47,32)   = src->srcPort;
    dest->data.range(31,0)    = src->srcAddress;

    return;
}

void mmInterface::udpMetaUnpack(ipUdpMetaPack_t *src,
                                ipUdpMeta_t *dest)
{
#pragma HLS INLINE

    dest->validSum  = src->data.range(128,128);
    dest->subSum     = src->data.range(127,112);
    dest->length     = src->data.range(111,96);
    dest->dstPort    = src->data.range(95,80);
    dest->dstAddress = src->data.range(79,48);
    dest->srcPort    = src->data.range(47,32);
    dest->srcAddress = src->data.range(31,0);

    return;
}

void mmInterface::ipTcpReadRequestPack(ipTcpReadRequest_t *src,
                                       ipTcpReadRequestPack_t *dest)
{
#pragma HLS INLINE

    dest->data.range(31,16) = src->length;
    dest->data.range(15,0)  = src->sessionID;

    return;
}

void mmInterface::ipTcpTxMetaPack(ipTcpTxMeta_t *src,
                                  ipTcpTxMetaPack_t *dest)
{
#pragma HLS INLINE


    dest->data.range(48,48) = src->validSum;
    dest->data.range(47,32) = src->subSum;
    dest->data.range(31,16) = src->length;
    dest->data.range(15,0)  = src->sessionID;

    return;
}

void mmInterface::ipTcpNotificationUnpack(ipTcpNotificationPack_t *src,
                                          ipTcpNotification_t *dest)
{
#pragma HLS INLINE

    dest->rsvd      = src->data.range(103,96);
    dest->opened    = src->data.range(95,88);
    dest->closed    = src->data.range(87,80);
    dest->dstPort   = src->data.range(79,64);
    dest->ipAddress = src->data.range(63,32);
    dest->length    = src->data.range(31,16);
    dest->sessionID = src->data.range(15,0);

    return;
}

void mmInterface::ipTcpConnectionStatusUnpack(ipTcpConnectionStatusPack_t *src,
                                              ipTcpConnectionStatus_t *dest)
{
#pragma HLS INLINE

    dest->success   = src->data.range(23,16);
    dest->sessionID = src->data.range(15,0);

    return;
}

void mmInterface::ipTcpTxStatusStreamUnpack(ipTcpTxStatusPack_t *src,
                                            ipTcpTxStatus_t *dest)
{
#pragma HLS INLINE

    dest->error     = src->data.range(63,62);
    dest->space     = src->data.range(61,32);
    dest->length    = src->data.range(31,16);
    dest->sessionID = src->data.range(15,0);

    return;
}
