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

set COMMON_DIR [pwd]/../common/include
set KERNEL_DIR [pwd]

open_project -reset prj

add_files ${KERNEL_DIR}/memWrite.cpp  -cflags "-I${COMMON_DIR} -I${KERNEL_DIR}"

set_top memWrite
open_solution -reset -flow_target vitis "memWrite"
set_part $::env(XPART)
create_clock -period $::env(XPERIOD) -name default
config_compile -pragma_strict_mode=true
csynth_design
export_design -rtl verilog -format xo -output memWrite.xo

exit
