/*
  Copyright (c) 2019 Sogou, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Authors: Wu Jiaxu (wujiaxu@sogou-inc.com)
*/

#include "WFTaskFactory.h"
#include "WFMySQLClient.h"
#include "WFGlobal.h"

static inline bool __set_result(WFMySQLTask *task, WFMySQLResult& res)
{
	res.seqid = task->get_task_seq();
	res.task_state = task->get_state();
	res.task_error = task->get_error();
	res.success = false;

	if (res.task_state == WFT_STATE_SUCCESS)
	{
		res.resp = std::move(*task->get_resp());
		auto type = res.resp.get_packet_type();

		if (type == MYSQL_PACKET_EOF || type == MYSQL_PACKET_ERROR || type == MYSQL_PACKET_OK)
		{
			res.result_cursor.reset(&res.resp);
			res.success = true;
		}
	}

	return res.success;
}

static void __async_callback(WFMySQLClient::ON_SUCCESS on_success,
							 WFMySQLClient::ON_ERROR on_error,
							 WFMySQLClient::ON_COMPLETE on_complete,
							 WFMySQLTask *task)
{
	WFMySQLResult res;

	if (__set_result(task, res))
	{
		if (on_success)
			on_success(res.resp, res.result_cursor);
	}
	else
	{
		if (on_error)
		{
			std::string errmsg;

			if (res.task_state == WFT_STATE_SUCCESS)
				errmsg = res.resp.get_error_msg();
			else
				errmsg = WFGlobal::get_error_string(res.task_state, res.task_error);

			on_error(res.task_state, res.task_error, errmsg);
		}
	}

	if (on_complete)
		on_complete(res);
}

/*
static void __await_callback(WFMySQLTask *task)
{
	auto *pr = static_cast<std::promise<WFMySQLResult> *>(task->user_data);
	WFMySQLResult res;

	__set_result(task, res);
	pr->set_value(std::move(res));
	delete pr;
}
*/

static void __future_callback(WFMySQLTask *task)
{
	auto *pr = static_cast<WFPromise<WFMySQLResult> *>(task->user_data);
	WFMySQLResult res;

	__set_result(task, res);
	pr->set_value(std::move(res));
	delete pr;
}

WFMySQLClient::WFMySQLClient(const std::string& url):
	retry_max_(0),
	send_timeout_(-1),
	recv_timeout_(-1)
{
	parse_error_ = URIParser::parse(url, uri_);
}

/*
WFAsyncCtrl<WFMySQLResult> WFMySQLClient::async_request(const std::string& command,
														const std::vector<std::string>& params)
{
	auto *pr = new std::promise<WFMySQLResult>();
	auto fr = pr->get_future();
	auto *task = WFTaskFactory::create_mysql_task(uri_, retry_max_, __await_callback);

	task->get_req()->set_query(command, params);
	task->set_send_timeout(send_timeout_);
	task->set_receive_timeout(recv_timeout_);
	task->user_data = pr;

	task->start();
	return WFAsyncCtrl<WFMySQLResult>(std::move(fr));
}
*/

WFFuture<WFMySQLResult> WFMySQLClient::async_request(const std::string& sql)
{
	auto *pr = new WFPromise<WFMySQLResult>();
	auto fr = pr->get_future();
	auto *task = WFTaskFactory::create_mysql_task(uri_, retry_max_, __future_callback);

	task->get_req()->set_query(sql);
	task->set_send_timeout(send_timeout_);
	task->set_receive_timeout(recv_timeout_);
	task->user_data = pr;

	task->start();
	return fr;
}

WFMySQLResult WFMySQLClient::sync_request(const std::string& sql)
{
	return this->async_request(sql).get();
}

void WFMySQLClient::request(const std::string& sql,
							WFMySQLClient::ON_COMPLETE on_complete)
{
	request(sql, NULL, NULL, on_complete);
}

void WFMySQLClient::request(const std::string& sql,
							WFMySQLClient::ON_SUCCESS on_success,
							WFMySQLClient::ON_ERROR on_error,
							WFMySQLClient::ON_COMPLETE on_complete)
{
	auto&& cb = std::bind(__async_callback,
						  std::move(on_success),
						  std::move(on_error),
						  std::move(on_complete),
						  std::placeholders::_1);

	auto *redis_task = WFTaskFactory::create_mysql_task(uri_,
														retry_max_,
														std::move(cb));

	redis_task->get_req()->set_query(sql);
	redis_task->set_send_timeout(send_timeout_);
	redis_task->set_receive_timeout(recv_timeout_);
	redis_task->start();
}

