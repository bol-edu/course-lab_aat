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

typedef ap_axiu<16,0,0,0> ipTcpRxMetaPack_t;
typedef ap_axiu<64,0,0,0> ipTcpRxDataPack_t;
typedef ap_axiu<128,0,0,0> ipTcpNotificationPack_t;
typedef ap_axiu<1,0,0,0> ipTcpListenStatusPack_t;
typedef ap_axiu<32,0,0,0> ipTcpConnectionStatusPack_t;
typedef ap_axiu<64,0,0,0> ipTcpTxStatusPack_t;
						 

extern "C" {
void ip_m(	ap_uint<1> *memListen,
			ap_uint<32> *memConnect,
			ap_uint<64> *memTxStatus,
			hls::stream<ipTcpRxMetaPack_t> &rxMetaStreamPack,
            hls::stream<ipTcpRxDataPack_t> &rxDataStreamPack,
			hls::stream<ipTcpNotificationPack_t> &notificationStreamPack,
            hls::stream<ipTcpListenStatusPack_t> &listenStatusStreamPack,	//V            
            hls::stream<ipTcpConnectionStatusPack_t> &connectionStatusStreamPack,	//V
            hls::stream<ipTcpTxStatusPack_t> &txStatusStreamPack) {	//V
	
	//don't care
	ipTcpRxDataPack_t 			v_rxDataStreamPack;
	ipTcpRxMetaPack_t 			v_rxMetaStreamPack;
    ipTcpNotificationPack_t 	v_notificationStreamPack;	

	rxDataStreamPack.write(v_rxDataStreamPack);
	rxMetaStreamPack.write(v_rxMetaStreamPack);
	notificationStreamPack.write(v_notificationStreamPack);
	
	//data transfer stream
	ipTcpListenStatusPack_t v_ipTcpListenStatusPack;
	ipTcpConnectionStatusPack_t v_ipTcpConnectionStatusPack;
	ipTcpTxStatusPack_t v_ipTcpTxStatusPack;
	
	v_ipTcpListenStatusPack.data = memListen[0];
	v_ipTcpConnectionStatusPack.data = memConnect[0];
	v_ipTcpTxStatusPack.data = memTxStatus[0];
	
	listenStatusStreamPack.write(v_ipTcpListenStatusPack);
	connectionStatusStreamPack.write(v_ipTcpConnectionStatusPack);
	txStatusStreamPack.write(v_ipTcpTxStatusPack);

}
}
