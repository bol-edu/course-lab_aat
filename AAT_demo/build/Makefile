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

# Makefile for xclbin creation
#

#+-------------------------------------------------------------------------------
# The following parameters are assigned with default values. These parameters can
# be overridden through the make command line
#+-------------------------------------------------------------------------------

REPORT := no
PROFILE := no
DEBUG := no
ETHERNET_DATA_WIDTH := 64

#'estimate' for estimate report generation
#'system' for system report generation
ifneq ($(REPORT), no)
CLFLAGS += --report estimate
LDCLFLAGS += --report system
endif

# generates profile summary report
ifeq ($(PROFILE), yes)
LDCLFLAGS += --profile_kernel data:all:all:all
endif

# generates debug summary report
ifeq ($(DEBUG), yes)
CLFLAGS += --dk protocol:all:all:all
endif

# configuration settings
VPP := $(XILINX_VITIS)/bin/v++

# extract platform name only from full path to xpfm
XILINX_PLATFORM=$(basename $(notdir $(DEVICE)))

# set default hardware target, allows override for hardware emulation (hw_emu)
TARGET ?= hw

# set default Slave Bridge mode, allows overide for DMA variant (DMA)
DM_MODE ?= SB

ifeq ($(TARGET),hw)
PROJECT=aat
else
PROJECT=aat_hw_emu
endif

MEMREAD_XO := memRead.xo
MEMWRITE_XO := memWrite.xo
IPM_XO := ip_m.xo
IPS_XO := ip_s.xo
FEEDHANDLER_XO := feedHandlerTop.xo
ORDERBOOK_XO := orderBookTop.xo
ORDERBOOK_DM_XO := orderBookDataMoverTop.xo
PRICINGENGINE_XO := pricingEngineTop.xo
ORDERENTRY_TCP_XO := orderEntryTcpTop.xo

MEMREAD_XO_FULLPATH=../memRead/$(MEMREAD_XO)
MEMWRITE_XO_FULLPATH=../memWrite/$(MEMWRITE_XO)
IPM_XO_FULLPATH=../ip_m/$(IPM_XO)
IPS_XO_FULLPATH=../ip_s/$(IPS_XO)
FEEDHANDLER_XO_FULLPATH=../feedHandler/$(FEEDHANDLER_XO)
ORDERBOOK_XO_FULLPATH=../orderBook/$(ORDERBOOK_XO)
ORDERBOOK_DM_XO_FULLPATH=../orderBook/$(ORDERBOOK_DM_XO)
PRICINGENGINE_XO_FULLPATH=../pricingEngine/$(PRICINGENGINE_XO)
ORDERENTRY_TCP_XO_FULLPATH=../orderEntry/$(ORDERENTRY_TCP_XO)

# shell specific connectivity
VPPLINKFLAGS := --config connect.cfg

XODIR = xo

# adds debug symbols, IPC hooks for hardware emulation builds
VPPLINKFLAGS += -g

XOS += $(MEMREAD_XO) \
	   $(MEMWRITE_XO) \
       $(IPM_XO) \
	   $(IPS_XO) \
       $(FEEDHANDLER_XO) \
       $(ORDERBOOK_XO) \
       $(ORDERBOOK_DM_XO) \
       $(PRICINGENGINE_XO) \
       $(ORDERENTRY_TCP_XO)
			   
CLFLAGS += -t $(TARGET) --platform $(DEVICE) --save-temps --kernel_frequency 320

$(XODIR):
	mkdir $@

# gather up external kernels
xo/$(MEMREAD_XO): $(MEMREAD_XO_FULLPATH) | $(XODIR)
	cp $< $@
	
xo/$(MEMWRITE_XO): $(MEMWRITE_XO_FULLPATH) | $(XODIR)
	cp $< $@

xo/$(IPM_XO): $(IPM_XO_FULLPATH) | $(XODIR)
	cp $< $@

xo/$(IPS_XO): $(IPS_XO_FULLPATH) | $(XODIR)
	cp $< $@
	
xo/$(FEEDHANDLER_XO): $(FEEDHANDLER_XO_FULLPATH) | $(XODIR)
	cp $< $@	

xo/$(ORDERBOOK_XO): $(ORDERBOOK_XO_FULLPATH) | $(XODIR)
	cp $< $@

xo/$(ORDERBOOK_DM_XO): $(ORDERBOOK_DM_XO_FULLPATH) | $(XODIR)
	cp $< $@

xo/$(PRICINGENGINE_XO): $(PRICINGENGINE_XO_FULLPATH) | $(XODIR)
	cp $< $@

xo/$(ORDERENTRY_TCP_XO): $(ORDERENTRY_TCP_XO_FULLPATH) | $(XODIR)
	cp $< $@


# emulation configuration
emconfig: $(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)

BINARY_CONTAINER = $(PROJECT).xclbin
BINARY_CONTAINER_XOS = $(addprefix $(XODIR)/,$(XOS))

.PHONY: gather-xos
gather-xos: $(BINARY_CONTAINER_XOS)

.PHONY: binary-container
binary-container: $(BINARY_CONTAINER)

$(BINARY_CONTAINER): $(BINARY_CONTAINER_XOS)
	$(VPP) $(CLFLAGS) $(LDCLFLAGS) -l -o $@ $^ $(VPPLINKFLAGS)

.PHONY: clean
clean:
	-rm -f $(PROJECT).xclbin* $(PROJECT).ltx
	-rm -rf _x .Xil
	-rm -f *.log

realclean: clean
	-rm -rf xo/

.PHONY: help
help:
	@echo "Makefile usage:"
	@echo "  make binary-container  Generate hardware build (xclbin)"
	@echo "  make clean             Clean intermediate build folders"
	@echo "  make realclean         Clean + remove imported kernel objects (xo)"

