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

#ifndef AAT_INTERFACES_H
#define AAT_INTERFACES_H

#include "ap_axi_sdata.h"
#include "hls_stream.h"

typedef struct orderBookOperation_t
{
    ap_uint<64> timestamp;
    ap_uint<8>  opCode;
    ap_uint<8>  symbolIndex;
    ap_uint<32> orderId;
    ap_uint<32> orderCount;
    ap_uint<32> quantity;
    ap_uint<32> price;
    ap_uint<8>  direction;
    ap_int<8>   level;
} orderBookOperation_t;

// TODO: 56b timestamp to pack within 1024b total, review if 64b required
typedef struct orderBookResponse_t
{
    ap_uint<56>  timestamp;
    ap_uint<8>   symbolIndex;
    ap_uint<160> bidCount;
    ap_uint<160> bidPrice;
    ap_uint<160> bidQuantity;
    ap_uint<160> askCount;
    ap_uint<160> askPrice;
    ap_uint<160> askQuantity;
} orderBookResponse_t;

typedef struct orderBookResponseVerify_t
{
    ap_uint<8>  symbolIndex;
    ap_uint<32> bidCount[NUM_LEVEL];
    ap_uint<32> bidPrice[NUM_LEVEL];
    ap_uint<32> bidQuantity[NUM_LEVEL];
    ap_uint<32> askCount[NUM_LEVEL];
    ap_uint<32> askPrice[NUM_LEVEL];
    ap_uint<32> askQuantity[NUM_LEVEL];
} orderBookResponseVerify_t;

typedef struct orderEntryOperation_t
{
    ap_uint<64> timestamp;
    ap_uint<8>  opCode;
    ap_uint<8>  symbolIndex;
    ap_uint<32> orderId;
    ap_uint<32> quantity;
    ap_uint<32> price;
    ap_uint<8>  direction;
} orderEntryOperation_t;

typedef struct orderEntryOperationEncode_t
{
    ap_uint<64> timestamp;
    ap_uint<8>  opCode;
    ap_uint<8>  symbolIndex;
    ap_uint<80> orderId;
    ap_uint<80> quantity;
    ap_uint<80> price;
    ap_uint<8>  direction;
} orderEntryOperationEncode_t;

typedef struct ipTuple_t
{
    ap_uint<32> address;
    ap_uint<16> port;
} ipTuple_t;

typedef struct ipUdpMeta_t
{
    ap_uint<32> srcAddress;
    ap_uint<16> srcPort;
    ap_uint<32> dstAddress;
    ap_uint<16> dstPort;
    ap_uint<16> length;
    ap_uint<16> subSum;
    ap_uint<1> validSum;
} ipUdpMeta_t;

typedef struct ipTcpReadRequest_t
{
    ap_uint<16> sessionID;
    ap_uint<16> length;
} ipTcpReadRequest_t;

typedef struct ipTcpTxMeta_t
{
    ap_uint<16> sessionID;
    ap_uint<16> length;
    ap_uint<16> subSum;
    ap_uint<1>  validSum;
} ipTcpTxMeta_t;

typedef struct ipTcpNotification_t
{
    ap_uint<16> sessionID;
    ap_uint<16> length;
    ap_uint<32> ipAddress;
    ap_uint<16> dstPort;
    ap_uint<8>  closed;
    ap_uint<8>  opened;
    ap_uint<8>  rsvd;
} ipTcpNotification_t;

typedef struct ipTcpConnectionStatus_t
{
    ap_uint<16> sessionID;
    ap_uint<8>  success;
} ipTcpConnectionStatus_t;

typedef struct ipTcpTxStatus_t
{
    ap_uint<16> sessionID;
    ap_uint<16> length;
    ap_uint<30> space;
    ap_uint<2>  error;
} ipTcpTxStatus_t;


template <unsigned W>
struct axiu
{
    ap_uint<W> data;
    ap_uint<W/8> keep;
    ap_uint<W/8> strb;
    ap_uint<1> last;

    axiu() {}

    axiu(ap_uint<W> iData, ap_uint<W/8> iKeep, ap_uint<W/8> iStrb, ap_uint<1> iLast)
        : data(iData), keep(iKeep), strb(iStrb), last(iLast) {}

    inline axiu(const ap_axiu<W,0,0,0> &axiw)
        : axiu(axiw.data, axiw.keep, axiw.strb, axiw.last) {}

    inline axiu(const qdma_axis<W,0,0,0> &qdmaw)
        : axiu(qdmaw.data, qdmaw.keep, -1, qdmaw.last) {}

    template <typename T>
    T pack(void) const
    {
#pragma HLS INLINE
        T axiw;
        axiw.data = data;
        axiw.keep = keep;
        axiw.strb = strb;
        axiw.last = last;
        return axiw;
    }

    operator ap_axiu<W,0,0,0> () const
    {
#pragma HLS INLINE
        return pack<ap_axiu<W,0,0,0>> ();
    }

    operator qdma_axis<W,0,0,0> () const
    {
#pragma HLS INLINE
        return pack<qdma_axis<W,0,0,0>> ();
    }
};

template <unsigned W>
struct axis
{
    ap_int<W> data;
    ap_uint<W/8> keep;
    ap_uint<W/8> strb;
    ap_uint<1> last;

    axis() {}

    axis(ap_int<W> iData, ap_uint<W/8> iKeep, ap_uint<W/8> iStrb, ap_uint<1> iLast)
        : data(iData), keep(iKeep), strb(iStrb), last(iLast) {}

    inline axis(const ap_axis<W,0,0,0> &axiw)
        : axis(axiw.data, axiw.keep, axiw.strb, axiw.last) {}

    inline axis(const qdma_axis<W,0,0,0> &qdmaw)
        : axis(qdmaw.data, qdmaw.keep, -1, qdmaw.last) {}

    template<typename T>
    T pack(void) const
    {
#pragma HLS INLINE
        T axiw;
        axiw.data = data;
        axiw.keep = keep;
        axiw.strb = strb;
        axiw.last = last;
        return axiw;
    }

    operator ap_axis<W,0,0,0> () const
    {
#pragma HLS INLINE
        return pack<ap_axis<W,0,0,0>> ();
    }

    operator qdma_axis<W,0,0,0> () const
    {
#pragma HLS INLINE
        return pack<qdma_axis<W,0,0,0>> ();
    }
};

// packed data structures
typedef ap_uint<16> templateId_t;
typedef ap_uint<32> securityId_t;
typedef ap_axiu<224,0,0,0> orderBookOperationPack_t;
typedef ap_axiu<1024,0,0,0> orderBookResponsePack_t;
typedef ap_axiu<184,0,0,0> orderEntryOperationPack_t;
typedef ap_axiu<1024,0,0,0> orderEntryMessagePack_t;
typedef ap_axiu<8,0,0,0> clockTickGeneratorEvent_t;

// network facing packed data structures
typedef ap_uint<16> ipTcpListenPort_t;
typedef ap_uint<16> ipTcpRxMeta_t;
typedef ap_uint<16> ipTcpCloseConnection_t;
typedef axis<64> axiWord_t;
typedef axiu<256> ipUdpMetaPack_t;
typedef ap_axis<64,0,0,0> axiWordExt_t;
typedef ap_axiu<256,0,0,0> ipUdpMetaPackExt_t;
typedef ap_axiu<64,0,0,0> ipTuplePack_t;
typedef ap_axiu<16,0,0,0> ipTcpListenPortPack_t;
typedef ap_axiu<1,0,0,0> ipTcpListenStatusPack_t;
typedef ap_axiu<128,0,0,0> ipTcpNotificationPack_t;
typedef ap_axiu<32,0,0,0> ipTcpReadRequestPack_t;
typedef ap_axiu<16,0,0,0> ipTcpRxMetaPack_t;
typedef ap_axiu<64,0,0,0> ipTcpRxDataPack_t;
typedef ap_axiu<32,0,0,0> ipTcpConnectionStatusPack_t;
typedef ap_axiu<16,0,0,0> ipTcpCloseConnectionPack_t;
typedef ap_axiu<64,0,0,0> ipTcpTxMetaPack_t;
typedef ap_axiu<64,0,0,0> ipTcpTxDataPack_t;
typedef ap_axiu<64,0,0,0> ipTcpTxStatusPack_t;

class mmInterface
{
public:

    // TODO: these pack/unpack functions could be moved to the typedefs and use
    //       the constructor functionality of C++, this would remove the need
    //       for a lot of the *Stream and *StreamPack bloat
    void orderBookOperationPack(orderBookOperation_t *src,
                                orderBookOperationPack_t *dest);

    void orderBookOperationUnpack(orderBookOperationPack_t *src,
                                  orderBookOperation_t *dest);

    void orderBookResponsePack(orderBookResponse_t *src,
                               orderBookResponsePack_t *dest);

    void orderBookResponseUnpack(orderBookResponsePack_t *src,
                                 orderBookResponse_t *dest);

    void orderEntryOperationPack(orderEntryOperation_t *src,
                                 orderEntryOperationPack_t *dest);

    void orderEntryOperationUnpack(orderEntryOperationPack_t *src,
                                   orderEntryOperation_t *dest);

    void ipTuplePack(ipTuple_t *src,
                     ipTuplePack_t *dest);

    void udpMetaPack(ipUdpMeta_t *src,
                     ipUdpMetaPack_t *dest);

    void udpMetaUnpack(ipUdpMetaPack_t *src,
                       ipUdpMeta_t *dest);

    void ipTcpReadRequestPack(ipTcpReadRequest_t *src,
                              ipTcpReadRequestPack_t *dest);

    void ipTcpTxMetaPack(ipTcpTxMeta_t *src,
                         ipTcpTxMetaPack_t *dest);

    void ipTcpNotificationUnpack(ipTcpNotificationPack_t *src,
                                 ipTcpNotification_t *dest);

    void ipTcpConnectionStatusUnpack(ipTcpConnectionStatusPack_t *src,
                                     ipTcpConnectionStatus_t *dest);

    void ipTcpTxStatusStreamUnpack(ipTcpTxStatusPack_t *src,
                                   ipTcpTxStatus_t *dest);

private:

};

#endif
