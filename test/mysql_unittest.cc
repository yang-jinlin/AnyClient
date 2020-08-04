/*
  Copyright (c) 2020 Sogou, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Author: Wu Jiaxu (wujiaxu@sogou-inc.com)
*/

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <gtest/gtest.h>
#include <workflow/WFMySQLServer.h>
#include <anyclient/WFMySQLClient.h>

#define RETRY_MAX  3

static void __mysql_process(WFMySQLTask *task)
{
	//auto *req = task->get_req();
	auto *resp = task->get_resp();

	resp->set_ok_packet();
}

TEST(WFMySQLTask1, mysql_unittest)
{
	WFMySQLServer server(__mysql_process);
	EXPECT_TRUE(server.start("127.0.0.1", 8899) == 0) << "server start failed";

	WFMySQLClient mysql_client("mysql://testuser:testpass@127.0.0.1:8899/testdb");

	mysql_client.default_retry_max(RETRY_MAX);
	auto fr = mysql_client.async_request("select * from testtable limit 3");
	auto result = fr.get();

	EXPECT_TRUE(result.success);
	server.stop();
}

