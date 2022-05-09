# LAB#D Accelerated Algorithmic Trading Demo
# Introduction
The Accelerated Algorithmic Trading Platform (AAT) is an integrated hardware and software solution for the Xilinx® Alveo™ platform. The hardware side includes the network layer and transaction-related modules; the software side provides API and host program shell to set the hardware. However, the entire hardware verification process requires additional network cabling and setup. To simplify the verification process, we removed the network-related kernels and used a simple host program to verify the main transaction-related kernels.

# About this lab
In this lab, you will run the Accelerated Algorithmic Trading platform provided by Xilinx, and learn about the algorithmic trading process from receiving packets to sending orders, as well as the design of the entire system.

# How to build & run
build  
 
    $ cd AAT_demo/build/
    $ ./buildall.sh

run 

    $ ./run.sh

# Others
If you cannot run *.sh  

    $ chmod u+x buildall.sh (or run.sh)

You may need to reset the FPGA before running Host program  

    $ xbutil reset --device

## Using devtoolset-9 bash to avoid compilation problems  
If you use workstation, enter the following command, before running build & run shell.

    $ scl enable devtoolset-9 bash

# Reference
https://xilinx.github.io/XRT/2021.2/html/xrt_native_apis.html
https://github.com/Xilinx/Vitis_Accel_Examples/tree/master/host_xrt/streaming_free_running_k2k_xrt
