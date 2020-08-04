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
           Li Yingxin (liyingxin@sogou-inc.com)
*/

#include <workflow/WFTaskFactory.h>
#include <workflow/WFGlobal.h>
#include "WFRedisClient.h"

static inline bool __set_result(WFRedisTask *task, WFRedisResult& res)
{
	res.seqid = task->get_task_seq();
	res.task_state = task->get_state();
	res.task_error = task->get_error();
	res.success = false;

	if (res.task_state == WFT_STATE_SUCCESS)
	{
		const auto *resp = task->get_resp();

		if (resp->parse_success())
		{
			resp->get_result(res.value);
			if (res.value.is_ok())
				res.success = true;
		}
	}

	return res.success;
}

static void __async_callback(WFRedisClient::ON_SUCCESS on_success,
							 WFRedisClient::ON_ERROR on_error,
							 WFRedisClient::ON_COMPLETE on_complete,
							 WFRedisTask *task)
{
	WFRedisResult res;

	if (__set_result(task, res))
	{
		if (on_success)
			on_success(res.value);
	}
	else
	{
		if (on_error)
		{
			std::string errmsg;

			if (res.task_state == WFT_STATE_SUCCESS)
				errmsg = res.value.string_value();
			else
				errmsg = WFGlobal::get_error_string(res.task_state, res.task_error);

			on_error(res.task_state, res.task_error, errmsg);
		}
	}

	if (on_complete)
		on_complete(res);
}

/*
static void __await_callback(WFRedisTask *task)
{
	auto *pr = static_cast<std::promise<WFRedisResult> *>(task->user_data);
	WFRedisResult res;

	__set_result(task, res);
	pr->set_value(std::move(res));
	delete pr;
}
*/

static void __future_callback(WFRedisTask *task)
{
	auto *pr = static_cast<WFPromise<WFRedisResult> *>(task->user_data);
	WFRedisResult res;

	__set_result(task, res);
	pr->set_value(std::move(res));
	delete pr;
}

WFRedisClient::WFRedisClient(const std::string& url):
	retry_max_(0),
	send_timeout_(-1),
	recv_timeout_(-1)
{
	parse_error_ = URIParser::parse(url, uri_);
}

/*
WFAsyncCtrl<WFRedisResult> WFRedisClient::async_request(const std::string& command,
														const std::vector<std::string>& params)
{
	auto *pr = new std::promise<WFRedisResult>();
	auto fr = pr->get_future();
	auto *task = WFTaskFactory::create_redis_task(uri_, retry_max_, __await_callback);

	task->get_req()->set_request(command, params);
	task->set_send_timeout(send_timeout_);
	task->set_receive_timeout(recv_timeout_);
	task->user_data = pr;

	task->start();
	return WFAsyncCtrl<WFRedisResult>(std::move(fr));
}
*/

WFFuture<WFRedisResult> WFRedisClient::async_request(const std::string& command,
													 const std::vector<std::string>& params)
{
	auto *pr = new WFPromise<WFRedisResult>();
	auto fr = pr->get_future();
	auto *task = WFTaskFactory::create_redis_task(uri_, retry_max_, __future_callback);

	task->get_req()->set_request(command, params);
	task->set_send_timeout(send_timeout_);
	task->set_receive_timeout(recv_timeout_);
	task->user_data = pr;

	task->start();
	return fr;
}

WFRedisResult WFRedisClient::sync_request(const std::string& command,
										  const std::vector<std::string>& params)
{
	return this->async_request(command, params).get();
}

void WFRedisClient::request(const std::string& command,
							const std::vector<std::string>& params,
							WFRedisClient::ON_COMPLETE on_complete)
{
	request(command, params, NULL, NULL, on_complete);
}

void WFRedisClient::request(const std::string& command,
							const std::vector<std::string>& params,
							WFRedisClient::ON_SUCCESS on_success,
							WFRedisClient::ON_ERROR on_error,
							WFRedisClient::ON_COMPLETE on_complete)
{
	auto&& cb = std::bind(__async_callback,
						  std::move(on_success),
						  std::move(on_error),
						  std::move(on_complete),
						  std::placeholders::_1);

	auto *redis_task = WFTaskFactory::create_redis_task(uri_,
														retry_max_,
														std::move(cb));

	redis_task->get_req()->set_request(command, params);
	redis_task->set_send_timeout(send_timeout_);
	redis_task->set_receive_timeout(recv_timeout_);
	redis_task->start();
}

WFRedisChain WFRedisClient::request(const std::string& command)
{
	return WFRedisChain(uri_, command, retry_max_, send_timeout_, recv_timeout_);
}

WFRedisTask *WFRedisChain::create_task()
{
	auto&& cb = std::bind(__async_callback,
						  on_success_,
						  on_error_,
						  on_complete_,
						  std::placeholders::_1);

	auto *redis_task = WFTaskFactory::create_redis_task(uri_,
														retry_max_,
														std::move(cb));

	redis_task->get_req()->set_request(command_, params_);
	redis_task->set_send_timeout(send_timeout_);
	redis_task->set_receive_timeout(recv_timeout_);
	return redis_task;
}

void WFRedisChain::send()
{
	create_task()->start();
}

WFRedisChain& WFRedisChain::append(const std::string& param)
{
	params_.push_back(param);
	return *this;
}

WFRedisChain& WFRedisChain::append(const std::vector<std::string>& params)
{
	params_.insert(params_.end(), params.begin(), params.end());
	return *this;
}

WFRedisChain& WFRedisChain::operator() (const std::string& param)
{
	return append(param);
}

WFRedisChain& WFRedisChain::operator() (const std::vector<std::string>& params)
{
	return append(params);
}

WFRedisChain& WFRedisChain::complete(WFRedisClient::ON_COMPLETE on_complete)
{
	on_complete_ = std::move(on_complete);
	return *this;
}

WFRedisChain& WFRedisChain::success(WFRedisClient::ON_SUCCESS on_success)
{
	on_success_ = std::move(on_success);
	return *this;
}

WFRedisChain& WFRedisChain::error(WFRedisClient::ON_ERROR on_error)
{
	on_error_ = std::move(on_error);
	return *this;
}

WFRedisChain& WFRedisChain::retry_max(int n)
{
	retry_max_ = n;
	return *this;
}

