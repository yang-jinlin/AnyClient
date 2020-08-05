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

  Authors: Wu Jiaxu (wujiaxu@sogou-inc.com)
           Li Yingxin (liyingxin@sogou-inc.com)
*/

#ifndef _WFHTTPCLIENT_H_
#define _WFHTTPCLIENT_H_

#include <string>
#include <map>
#include <functional>
#include <workflow/HttpMessage.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/WFFuture.h>

/**
 * @file   WFHttpClient.h
 * @brief  Thread Safety Http Client
 */

struct WFHttpResult
{
	protocol::HttpResponse resp;
	long long seqid;
	int task_state;
	int task_error;
	int status_code;//-1, 200, 206, 404, 500
};

class WFHttpChain;//for method chaining

class WFHttpClient
{
public:
	using ON_COMPLETE = std::function<void (WFHttpResult&)>;
	using ON_SUCCESS = std::function<void (protocol::HttpResponse&)>;
	using ON_ERROR = std::function<void (int state, int error, const std::string& errmsg)>;

	WFHttpClient():
		retry_max_(0),
		redirect_max_(0),
		send_timeout_(-1),
		recv_timeout_(-1)
	{}

	void default_retry_max(int n) { retry_max_ = n; }
	void default_redirect_max(int n) { redirect_max_ = n; }
	void default_send_timeout(int timeout) { send_timeout_ = timeout; }
	void default_recv_timeout(int timeout) { recv_timeout_ = timeout; }

	//sync
	WFHttpResult sync_request(const std::string& method,
							  const std::string& url,
							  const std::map<std::string, std::string>& headers,
							  const std::string& body);

	//async future
	WFFuture<WFHttpResult> async_request(const std::string& method,
										 const std::string& url,
										 const std::map<std::string, std::string>& headers,
										 const std::string& body);

	//async
	void request(const std::string& method,
				 const std::string& url,
				 const std::map<std::string, std::string>& headers,
				 const std::string& body,
				 WFHttpClient::ON_COMPLETE on_complete);

	void request(const std::string& method,
				 const std::string& url,
				 const std::map<std::string, std::string>& headers,
				 const std::string& body,
				 WFHttpClient::ON_SUCCESS on_success,
				 WFHttpClient::ON_ERROR on_error,
				 WFHttpClient::ON_COMPLETE on_complete);

	//async, method chaining style
	WFHttpChain request(const std::string& method, const std::string& url);

private:
	int retry_max_;
	int redirect_max_;
	int send_timeout_;
	int recv_timeout_;
};

//client.request("GET", "https://www.sogou.com").set_header("Connection", "Keep-Alive").send();
//client.request(method, url).set_header(key, value).append_body(body).retry_max(n).success(cb1).error(cb2).send()
class WFHttpChain
{
public:
	void send();
	WFHttpTask *create_task();
	WFHttpChain& set_header(const std::string& key, const std::string& value);
	WFHttpChain& set_header(const std::map<std::string, std::string>& headers);
	WFHttpChain& append_body(const std::string& str);
	WFHttpChain& operator() (const std::string& str);
	WFHttpChain& complete(WFHttpClient::ON_COMPLETE on_complete);
	WFHttpChain& success(WFHttpClient::ON_SUCCESS on_success);
	WFHttpChain& error(WFHttpClient::ON_ERROR on_error);
	WFHttpChain& retry_max(int n);
	WFHttpChain& redirect_max(int n);
	WFHttpChain& send_timeout(int timeout);
	WFHttpChain& recv_timeout(int timeout);

private:
	WFHttpChain(const std::string& method,
				const std::string& url,
				int retry_max,
				int redirect_max,
				int send_timeout,
				int recv_timeout);

	std::string method_;
	std::string url_;
	std::map<std::string, std::string> headers_;
	std::string body_;
	WFHttpClient::ON_COMPLETE on_complete_;
	WFHttpClient::ON_SUCCESS on_success_;
	WFHttpClient::ON_ERROR on_error_;
	int retry_max_;
	int redirect_max_;
	int send_timeout_;
	int recv_timeout_;

	friend class WFHttpClient;
};

#endif

