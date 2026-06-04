/****************************************************************************
 * apps/examples/microros_srv/microros_srv_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <unistd.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rosidl_runtime_c/string_functions.h>
#include <lifecycle_msgs/srv/get_state.h>
#include <lifecycle_msgs/msg/state.h>

#include <system/microros_transport.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lifecycle_msgs__srv__GetState_Request  g_req;
static lifecycle_msgs__srv__GetState_Response g_resp;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void srv_callback(const void *req_in, void *resp_in)
{
  lifecycle_msgs__srv__GetState_Response *resp =
    (lifecycle_msgs__srv__GetState_Response *)resp_in;

  (void)req_in;

  resp->current_state.id = lifecycle_msgs__msg__State__PRIMARY_STATE_ACTIVE;
  rosidl_runtime_c__String__assign(&resp->current_state.label, "active");

  printf("microros_srv: GetState request -> {id=%u, label=\"active\"}\n",
         (unsigned)resp->current_state.id);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  rcl_service_t    service;
  rcl_allocator_t  allocator;
  rclc_support_t   support;
  rcl_node_t       node;
  rclc_executor_t  executor;

  printf("microros_srv: starting\n");

  if (microros_transport_init() != 0)
    {
      printf("microros_srv: transport init failed\n");
      return 1;
    }

  allocator = rcl_get_default_allocator();

  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK)
    {
      printf("microros_srv: rclc_support_init failed\n");
      return 1;
    }

  if (rclc_node_init_default(&node, "nuttx_srv_node", "", &support)
      != RCL_RET_OK)
    {
      printf("microros_srv: node init failed\n");
      return 1;
    }

  if (rclc_service_init_default(
        &service,
        &node,
        ROSIDL_GET_SRV_TYPE_SUPPORT(lifecycle_msgs, srv, GetState),
        "nuttx_get_state") != RCL_RET_OK)
    {
      printf("microros_srv: service init failed\n");
      return 1;
    }

  lifecycle_msgs__srv__GetState_Response__init(&g_resp);

  executor = rclc_executor_get_zero_initialized_executor();
  if (rclc_executor_init(&executor, &support.context, 1, &allocator)
      != RCL_RET_OK)
    {
      printf("microros_srv: executor init failed\n");
      return 1;
    }

  if (rclc_executor_add_service(&executor,
                                &service,
                                &g_req,
                                &g_resp,
                                &srv_callback) != RCL_RET_OK)
    {
      printf("microros_srv: add_service failed\n");
      return 1;
    }

  printf("microros_srv: spinning on /nuttx_get_state\n");

  rclc_executor_spin(&executor);

  rclc_executor_fini(&executor);
  lifecycle_msgs__srv__GetState_Response__fini(&g_resp);
  rcl_service_fini(&service, &node);
  rcl_node_fini(&node);
  rclc_support_fini(&support);

  printf("microros_srv: done\n");
  return 0;
}
