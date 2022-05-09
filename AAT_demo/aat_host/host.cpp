#include "cmdlineparser.h"
#include <iostream>
#include <cstring>

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_ip.h"

#include "ap_int.h"

#define NUM_PACKET           (54)
#define NUM_FRAME_PER_PACKET (13)
#define NUM_SYMBOL    10

ap_uint<64> byteReverse(ap_uint<64> inputData){
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

typedef struct orderEntryOperation_t{
    ap_uint<64> timestamp;
    ap_uint<8>  opCode;
    ap_uint<8>  symbolIndex;
    ap_uint<32> orderId;
    ap_uint<32> quantity;
    ap_uint<32> price;
    ap_uint<8>  direction;
} orderEntryOperation_t;


int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;

    // Switches
    //**************//"<Full Arg>",  "<Short Arg>", "<Description>", "<Default>"
    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
    parser.addSwitch("--device_id", "-d", "device index", "0");
    parser.parse(argc, argv);

    // Read settings
    std::string binaryFile = parser.value("xclbin_file");
    int device_index = stoi(parser.value("device_id"));

    if (argc < 3) {
        parser.printHelp();
        return EXIT_FAILURE;
    }

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);

	// set kernel & ip
	// If the kernel HLS INTERFACE is "ap_ctrl_none", the kernel must use xrt::ip
	auto mem_read = xrt::kernel(device, uuid, "memRead");
    auto mem_write = xrt::kernel(device, uuid, "memWrite");
	auto ip_m = xrt::kernel(device, uuid, "ip_m");
	auto ip_s = xrt::kernel(device, uuid, "ip_s");
	auto feedHandlerTop = xrt::ip(device, uuid, "feedHandlerTop");
	auto orderBookTop = xrt::ip(device, uuid, "orderBookTop");
	auto pricingEngineTop = xrt::ip(device, uuid, "pricingEngineTop");
	auto orderEntryTcpTop = xrt::ip(device, uuid, "orderEntryTcpTop");
	
    size_t input_size = NUM_FRAME_PER_PACKET, output_size = 32;
	
	/*** order Entry Tcp Networking Setting ***/
	orderEntryTcpTop.write_register(0x018, 0xdeadbeef); // regControl configuration
	orderEntryTcpTop.write_register(0x020, 0x00000000);	// regControl configuration
	orderEntryTcpTop.write_register(0x028, 0x640aa8c0);	// Destination address for egress IP sends, 192.168.10.100
	orderEntryTcpTop.write_register(0x030, 0x17);		// Destination port for egress IP sends, telnet (port 23)
	orderEntryTcpTop.write_register(0x010, ((1<<4) | (1<<3)));// [4] Enable partial checksum calculation and forwarding; [3] Initiate egress TCP connection request

	// ip_s config
	auto ip_s_run = xrt::run(ip_s);
    auto buffer_lport = xrt::bo(device, sizeof(ap_uint<16>), ip_s.group_id(0));
	auto buffer_lport_mapped = buffer_lport.map<ap_uint<16>*>();
    auto buffer_oconnect = xrt::bo(device, sizeof(ap_uint<64>), ip_s.group_id(0));
	auto buffer_oconnect_mapped = buffer_oconnect.map<ap_uint<64>*>();	
	ip_s_run.set_arg(0, buffer_lport);
	ip_s_run.set_arg(1, buffer_oconnect);	
	
	if(orderEntryTcpTop.read_register(0x110) & 0x21){	// OrderEntry INIT_CON complete
		ip_s_run.start();
		ip_s_run.wait();
		
		buffer_lport.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		buffer_oconnect.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		std::cout << std::hex << buffer_lport_mapped[0] << std::endl;
		std::cout << std::hex << buffer_oconnect_mapped[0] << std::endl;
	}	
	
	// ip_m config
	auto ip_m_run = xrt::run(ip_m);
    auto buffer_listen = xrt::bo(device, sizeof(ap_uint<1>), ip_m.group_id(0));
	auto buffer_listen_mapped = buffer_listen.map<ap_uint<1>*>();
    auto buffer_connect = xrt::bo(device, sizeof(ap_uint<32>), ip_m.group_id(0));
	auto buffer_connect_mapped = buffer_connect.map<ap_uint<32>*>();
    auto buffer_txstatus = xrt::bo(device, sizeof(ap_uint<64>), ip_m.group_id(0));
	auto buffer_txstatus_mapped = buffer_txstatus.map<ap_uint<64>*>();	
	ip_m_run.set_arg(0, buffer_listen);
	ip_m_run.set_arg(1, buffer_connect);
	ip_m_run.set_arg(2, buffer_txstatus);

	if(orderEntryTcpTop.read_register(0x110) & 0x321){	// OrderEntry enter WAIT_CON state
		buffer_listen_mapped[0] = 0x1;
		buffer_connect_mapped[0] = 0x10001;
		buffer_txstatus_mapped[0] = 0xffff01000001;	// TXSTATUS_SUCCESS=0, space=0xffff, length=0x0100, sessionID=0x0001
		
		buffer_listen.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		buffer_connect.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		buffer_txstatus.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		ip_m_run.start();
		ip_m_run.wait();
	}

	/*** Kernel Register Setting ***/
	// load packet file
    ap_uint<64> inputWords[NUM_PACKET][NUM_FRAME_PER_PACKET] ={
		#include "input_golden.dat"
    };

	// set Feed Handler symbols
	ap_uint<32> symbols[NUM_SYMBOL] = {0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555, 0x66666666, 0x77777777, 0x88888888, 0x99999999, 0x12345678};
	for (size_t i = 0; i < NUM_SYMBOL; i++) feedHandlerTop.write_register(0x0b8 + sizeof(ap_uint<32>)*i, symbols[i]);	// write to Feed Handler register

	// Order Book regControl configuration
	orderBookTop.write_register(0x010, 0x00000000);
	orderBookTop.write_register(0x018, 0xdeadbeef);
	orderBookTop.write_register(0x020, 0x00000000);

    // pricing engine regControl configuration
	pricingEngineTop.write_register(0x010, 0x12345678);
	pricingEngineTop.write_register(0x018, 0xdeadbeef);
	pricingEngineTop.write_register(0x020, 0x00000000);
	pricingEngineTop.write_register(0x028, 0x80000002);	//[31] Enable global strategy override for allsymbols; [7:0] Global strategy select for all symbols
    // strategy select (per symbol)
	pricingEngineTop.write_register(0x1000, 1);		// PE_SYMBOL_STRATEGY_SELECT
	pricingEngineTop.write_register(0x1004, 0xff);	// PE_SYMBOL_STRATEGY_ENABLE

	//input data map
    auto buffer_input = xrt::bo(device, sizeof(ap_uint<64>) * input_size, mem_read.group_id(0));
	auto buffer_input_mapped = buffer_input.map<ap_uint<64>*>();
	//output data map
    auto buffer_output = xrt::bo(device, sizeof(ap_uint<64>) * output_size, mem_write.group_id(0));
	auto buffer_output_mapped = buffer_output.map<ap_uint<64>*>();	
	
	//declare kernel run
	auto write_run = xrt::run(mem_write);
	auto read_run = xrt::run(mem_read);
	//set kernel arg
	write_run.set_arg(0, buffer_output);
    write_run.set_arg(1, output_size);
    read_run.set_arg(0, buffer_input);
    read_run.set_arg(1, input_size);	
 
	for (size_t n = 0; n < NUM_PACKET; n++){
		std::cout << "PACKET #" << std::dec << n << std::endl;
		// send udp packet
		for (size_t i = 0; i < input_size; i++) buffer_input_mapped[i] = byteReverse(inputWords[n][i]);
		buffer_input.sync(XCL_BO_SYNC_BO_TO_DEVICE);
		read_run.start();
		read_run.wait();
		

		// receive tcp packet
		write_run.start();
		write_run.wait();
		buffer_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE); // Copy Result from Device Global Memory to Host Local Memory
		for(size_t k=0; k<output_size; k++){
			std::cout << std::hex <<  buffer_output_mapped[k] << std::endl;
		}
		std::cout << std::endl;	
		
	}

	std::cout << std::dec << "orderEntryTcpTop" << std::endl;
	std::cout << "Number of order entry operations received: " << orderEntryTcpTop.read_register(0x058) << std::endl;
	std::cout << "Number of order entry operations processed: " << orderEntryTcpTop.read_register(0x068) << std::endl;
	std::cout << "Number of data frames transmitted: " << orderEntryTcpTop.read_register(0x078) << std::endl;
	std::cout << "Number of meta frames transmitted: " << orderEntryTcpTop.read_register(0x088) << std::endl;
	std::cout << "Number of orders transmitted: " << orderEntryTcpTop.read_register(0x098) << std::endl;
	
	bool match = true;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);

}
