/******************************************************************************
 *
 *  Copyright 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  this file contains the main Bluetooth Upper Layer definitions. The Broadcom
 *  implementations of L2CAP RFCOMM, SDP and the BTIf run as one GKI task. The
 *  btu_task switches between them.
 *
 ******************************************************************************/

#ifndef BTU_H
#define BTU_H

#include <base/functional/callback.h>
#include <base/location.h>
#include <base/threading/thread.h>

#include <cstdint>

#include "bt_target.h"
#include "common/message_loop_thread.h"
#include "osi/include/alarm.h"

/* Global BTU data */
extern uint8_t btu_trace_level;

/* Functions provided by btu_task.cc
 ***********************************
*/
bluetooth::common::MessageLoopThread* get_main_thread();
bt_status_t do_in_main_thread(const base::Location& from_here,
                              base::OnceClosure task);
bt_status_t do_in_main_thread_delayed(const base::Location& from_here,
                                      base::OnceClosure task,
                                      const base::TimeDelta& delay);

using BtMainClosure = std::function<void()>;
void post_on_bt_main(BtMainClosure closure);

#endif
