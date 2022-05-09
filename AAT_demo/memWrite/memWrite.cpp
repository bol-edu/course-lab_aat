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

typedef ap_axiu<64,0,0,0> ipTcpTxMetaPack_t;
typedef ap_axiu<64,0,0,0> ipTcpTxDataPack_t;

extern "C" {
void memWrite(	ap_uint<64>* mem,
				int & size,
				hls::stream<ipTcpTxDataPack_t>& txDataStreamPack, 
				hls::stream<ipTcpTxMetaPack_t>& txMetaStreamPack) {
						
	#pragma HLS INTERFACE axis register port=txDataStreamPack depth=64
	#pragma HLS INTERFACE axis register port=txMetaStreamPack depth=64
	
	if(!txMetaStreamPack.empty()) ipTcpTxMetaPack_t dummy_metaData = txMetaStreamPack.read();
    if(!txDataStreamPack.empty()){
		for (int i = 0; i < size; i++) {
			ap_axiu<64, 0, 0, 0> v = txDataStreamPack.read();
			mem[i] = v.data;
		}		
	}

}
}
