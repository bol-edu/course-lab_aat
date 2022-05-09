#
# Copyright 2021 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR=$(patsubst %/,%,$(dir $(MK_PATH)))

KERNEL_DIR=$(CUR_DIR)
COMMON_DIR=$(CUR_DIR)/../common/includes
COMMON_SRCS=$(COMMON_DIR)/aat_defines.hpp \
            $(COMMON_DIR)/aat_interfaces.cpp \
            $(COMMON_DIR)/aat_interfaces.hpp

OE_TARGET=orderentry

OE_SRCS=$(KERNEL_DIR)/orderentry.cpp \
        $(KERNEL_DIR)/orderentry.hpp \
        $(KERNEL_DIR)/orderentry_kernels.hpp \
        $(KERNEL_DIR)/orderentry_tcp_top.cpp

# use platform info utility to query correct part for board target
ifndef DEVICE
$(error DEVICE should be set to a valid Xilinx platform file (xpfm))
else
XPART=$(shell platforminfo $(DEVICE) --json="hardwarePlatform.board.part")
endif

# default build parameters
XPERIOD?=5

.PHONY: all
all: $(OE_TARGET)

$(OE_TARGET): $(OE_SRCS) $(COMMON_SRCS)
	-rm -rf prj*
	XPART=$(XPART) XPERIOD=$(XPERIOD) vitis_hls -f xo_generate.tcl

.PHONY: clean
clean:
	-rm -rf prj*
	-rm -f *.xo
	-rm -f *.log
