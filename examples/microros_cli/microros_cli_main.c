/****************************************************************************
 * apps/examples/microros_cli/microros_cli_main.c
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

static void cli_callback(const void *resp_in)
{
  const lifecycle_msgs__srv__GetState_Response *resp =
    (const lifecycle_msgs__srv__GetState_Response *)resp_in;

  const char *label = (resp->current_state.label.data != NULL)
                        ? (const char *)resp->current_state.label.data
                        : "(null)";

  printf("microros_cli: GetState response -> {id=%u, label=\"%s\"}\n",
         (unsigned)resp->current_state.id, label);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  rcl_client_t     client;
  rcl_allocator_t  allocator;
  rclc_support_t   support;
  rcl_node_t       node;
  rclc_executor_t  executor;
  int64_t          seq;

  printf("microros_cli: starting\n");

  if (microros_transport_init() != 0)
    {
      printf("microros_cli: transport init failed\n");
      return 1;
    }

  allocator = rcl_get_default_allocator();

  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK)
    {
      printf("microros_cli: rclc_support_init failed\n");
      return 1;
    }

  if (rclc_node_init_default(&node, "nuttx_cli_node", "", &support)
      != RCL_RET_OK)
    {
      printf("microros_cli: node init failed\n");
      return 1;
    }

  if (rclc_client_init_default(
        &client,
        &node,
        ROSIDL_GET_SRV_TYPE_SUPPORT(lifecycle_msgs, srv, GetState),
        "nuttx_get_state") != RCL_RET_OK)
    {
      printf("microros_cli: client init failed\n");
      return 1;
    }

  lifecycle_msgs__srv__GetState_Request__init(&g_req);
  lifecycle_msgs__srv__GetState_Response__init(&g_resp);

  executor = rclc_executor_get_zero_initialized_executor();
  if (rclc_executor_init(&executor, &support.context, 1, &allocator)
      != RCL_RET_OK)
    {
      printf("microros_cli: executor init failed\n");
      return 1;
    }

  if (rclc_executor_add_client(&executor,
                               &client,
                               &g_resp,
                               &cli_callback) != RCL_RET_OK)
    {
      printf("microros_cli: add_client failed\n");
      return 1;
    }

  if (rcl_send_request(&client, &g_req, &seq) != RCL_RET_OK)
    {
      printf("microros_cli: send_request failed\n");
      return 1;
    }

  printf("microros_cli: request sent (seq=%lld), waiting on response\n",
         (long long)seq);

  rclc_executor_spin(&executor);

  rclc_executor_fini(&executor);
  lifecycle_msgs__srv__GetState_Request__fini(&g_req);
  lifecycle_msgs__srv__GetState_Response__fini(&g_resp);
  rcl_client_fini(&client, &node);
  rcl_node_fini(&node);
  rclc_support_fini(&support);

  printf("microros_cli: done\n");
  return 0;
}
