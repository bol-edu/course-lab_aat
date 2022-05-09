/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

// network facing packed data structures

typedef ap_axiu<16,0,0,0> ipTcpListenPortPack_t;
typedef ap_axiu<32,0,0,0> ipTcpReadRequestPack_t;
typedef ap_axiu<64,0,0,0> ipTuplePack_t;
typedef ap_axiu<16,0,0,0> ipTcpCloseConnectionPack_t;

extern "C" {
void ip_s(		ap_uint<16> *listenPort_mem,
				ap_uint<64> *openConnection_mem,

                hls::stream<ipTcpListenPortPack_t> &listenPortStreamPack,	//v
				hls::stream<ipTcpReadRequestPack_t> &readRequestStreamPack,
                hls::stream<ipTuplePack_t> &openConnectionStreamPack,	//v				
				hls::stream<ipTcpCloseConnectionPack_t> &closeConnectionStreamPack		
                ) {
					
	if(!listenPortStreamPack.empty()) listenPort_mem[0] = listenPortStreamPack.read().data;
    if(!openConnectionStreamPack.empty()) openConnection_mem[0] = openConnectionStreamPack.read().data;

    }
}


