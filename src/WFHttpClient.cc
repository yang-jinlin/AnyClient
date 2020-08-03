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

#include "WFTaskFactory.h"
#include "WFHttpClient.h"
#include "WFGlobal.h"

static inline int __set_result(WFHttpTask *task, WFHttpResult& res)
{
	res.seqid = task->get_task_seq();
	res.task_state = task->get_state();
	res.task_error = task->get_error();
	res.status_code = -1;
	if (res.task_state == WFT_STATE_SUCCESS)
	{
		res.resp = std::move(*task->get_resp());
		res.status_code = atoi(res.resp.get_status_code());
	}

	return res.status_code;
}

static void __async_callback(WFHttpClient::ON_SUCCESS on_success,
							 WFHttpClient::ON_ERROR on_error,
							 WFHttpClient::ON_COMPLETE on_complete,
							 WFHttpTask *task)
{
	WFHttpResult res;

	if (__set_result(task, res) != -1)
	{
		if (on_success)
			on_success(res.resp);
	}
	else
	{
		if (on_error)
		{
			std::string errmsg = WFGlobal::get_error_string(res.task_state, res.task_error);

			on_error(res.task_state, res.task_error, errmsg);
		}
	}

	if (on_complete)
		on_complete(res);
}

static void __future_callback(WFHttpTask *task)
{
	auto *pr = static_cast<WFPromise<WFHttpResult> *>(task->user_data);
	WFHttpResult res;

	__set_result(task, res);
	pr->set_value(std::move(res));
	delete pr;
}

WFHttpResult WFHttpClient::sync_request(const std::string& method,
										const std::string& url,
										const std::map<std::string, std::string>& headers,
										const std::string& body)
{
	return this->async_request(method, url, headers, body).get();
}

WFFuture<WFHttpResult> WFHttpClient::async_request(const std::string& method,
												   const std::string& url,
												   const std::map<std::string, std::string>& headers,
												   const std::string& body)
{
	auto *pr = new WFPromise<WFHttpResult>();
	auto fr = pr->get_future();
	auto *http_task = WFTaskFactory::create_http_task(url,
													  redirect_max_,
													  retry_max_,
													  __future_callback);
	auto *req = http_task->get_req();

	req->set_method(method);

	for (const auto& kv : headers)
		req->add_header_pair(kv.first, kv.second);

	req->append_output_body(body);

	http_task->set_send_timeout(send_timeout_);
	http_task->set_receive_timeout(recv_timeout_);
	http_task->user_data = pr;
	http_task->start();

	return fr;
}

void WFHttpClient::request(const std::string& method,
						   const std::string& url,
						   const std::map<std::string, std::string>& headers,
						   const std::string& body,
						   WFHttpClient::ON_COMPLETE on_complete)
{
	request(method, url, headers, body, NULL, NULL, on_complete);
}

void WFHttpClient::request(const std::string& method,
						   const std::string& url,
						   const std::map<std::string, std::string>& headers,
						   const std::string& body,
						   WFHttpClient::ON_SUCCESS on_success,
						   WFHttpClient::ON_ERROR on_error,
						   WFHttpClient::ON_COMPLETE on_complete)
{
	auto&& cb = std::bind(__async_callback,
						  std::move(on_success),
						  std::move(on_error),
						  std::move(on_complete),
						  std::placeholders::_1);

	auto *http_task = WFTaskFactory::create_http_task(url,
													  redirect_max_,
													  retry_max_,
													  std::move(cb));
	auto *req = http_task->get_req();

	req->set_method(method);

	for (const auto& kv : headers)
		req->add_header_pair(kv.first, kv.second);

	req->append_output_body(body);

	http_task->set_send_timeout(send_timeout_);
	http_task->set_receive_timeout(recv_timeout_);
	http_task->start();
}

WFHttpChain WFHttpClient::request(const std::string& method, const std::string& url)
{
	return WFHttpChain(method, url,
					   retry_max_, redirect_max_, send_timeout_, recv_timeout_);
}

WFHttpChain::WFHttpChain(const std::string& method,
						 const std::string& url,
						 int retry_max,
						 int redirect_max,
						 int send_timeout,
						 int recv_timeout):
	method_(method),
	url_(url),
	on_complete_(NULL),
	on_success_(NULL),
	on_error_(NULL),
	retry_max_(retry_max),
	redirect_max_(redirect_max),
	send_timeout_(send_timeout),
	recv_timeout_(recv_timeout)
{}

WFHttpTask *WFHttpChain::create_task()
{
	auto&& cb = std::bind(__async_callback,
						  on_success_,
						  on_error_,
						  on_complete_,
						  std::placeholders::_1);

	auto *http_task = WFTaskFactory::create_http_task(url_,
													  redirect_max_,
													  retry_max_,
													  std::move(cb));

	auto *req = http_task->get_req();

	req->set_method(method_);

	for (const auto& kv : headers_)
		req->add_header_pair(kv.first, kv.second);

	req->append_output_body(body_);

	http_task->set_send_timeout(send_timeout_);
	http_task->set_receive_timeout(recv_timeout_);
	return http_task;
}

void WFHttpChain::send()
{
	create_task()->start();
}

WFHttpChain& WFHttpChain::set_header(const std::string& key, const std::string& value)
{
	headers_[key] = value;
	return *this;
}

WFHttpChain& WFHttpChain::set_header(const std::map<std::string, std::string>& headers)
{
	for (const auto& kv : headers_)
		headers_[kv.first] = kv.second;

	return *this;
}

WFHttpChain& WFHttpChain::append_body(const std::string& str)
{
	body_ += str;
	return *this;
}

WFHttpChain& WFHttpChain::operator() (const std::string& str)
{
	return append_body(str);
}

WFHttpChain& WFHttpChain::complete(WFHttpClient::ON_COMPLETE on_complete)
{
	on_complete_ = std::move(on_complete);
	return *this;
}

WFHttpChain& WFHttpChain::success(WFHttpClient::ON_SUCCESS on_success)
{
	on_success_ = std::move(on_success);
	return *this;
}

WFHttpChain& WFHttpChain::error(WFHttpClient::ON_ERROR on_error)
{
	on_error_ = std::move(on_error);
	return *this;
}

WFHttpChain& WFHttpChain::retry_max(int n)
{
	retry_max_ = n;
	return *this;
}

WFHttpChain& WFHttpChain::redirect_max(int n)
{
	redirect_max_ = n;
	return *this;
}

WFHttpChain& WFHttpChain::send_timeout(int timeout)
{
	send_timeout_ = timeout;
	return *this;
}

WFHttpChain& WFHttpChain::recv_timeout(int timeout)
{
	recv_timeout_ = timeout;
	return *this;
}

