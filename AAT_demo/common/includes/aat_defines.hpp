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

#ifndef AAT_DEFINES_H
#define AAT_DEFINES_H

#ifndef __SYNTHESIS__
#define _K_DEBUG_EN 1
#endif

#if _KDEBUG_EN == 1
#define KDEBUG(msg) do { std::cout << msg << std::endl; } while (0);
#else
#define KDEBUG(msg)
#endif

#define NUM_SYMBOL          (256)

#define PRICE_EXPONENT      (0.00001)
#define START_TRADING       (34200000000) // 09:30:00 in microseconds since midnight (9.5*60*60*1000000)
#define END_TRADING         (57600000000) // 16:00:00 in microseconds since midnight (16*60*60*1000000)

#define NUM_LEVEL           (5)
#define LEVEL_UNSPECIFIED   (-1)
#define NUM_TEST_SAMPLE     (54)

enum ORDERBOOK_OPCODES
{
    ORDERBOOK_ADD = 0,
    ORDERBOOK_MODIFY = 1,
    ORDERBOOK_DELETE = 2,
    ORDERBOOK_TRANSACT_VISIBLE = 3,
    ORDERBOOK_TRANSACT_HIDDEN = 4,
    ORDERBOOK_HALT= 6
};

enum ORDER_SIDES
{
    ORDER_BID = 0,
    ORDER_ASK
};

enum PRICINGENGINE_STRATEGIES
{
    STRATEGY_NONE = 0,
    STRATEGY_PEG = 1,
    STRATEGY_LIMIT = 2
};

enum ORDERENTRY_OPCODES
{
    ORDERENTRY_ADD = 0,
    ORDERENTRY_MODIFY,
    ORDERENTRY_DELETE
};

enum TCP_TXSTATUS_CODES
{
    TXSTATUS_SUCCESS = 0,
    TXSTATUS_CLOSED,
    TXSTATUS_OUTOFSPACE
};

#endif
