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

#ifndef _WFREDISCLIENT_H_
#define _WFREDISCLIENT_H_

#include <string>
#include <vector>
#include <functional>
#include <workflow/RedisMessage.h>
#include <workflow/URIParser.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/WFFuture.h>

/**
 * @file   WFRedisClient.h
 * @brief  Thread Safety Redis Client
 */

struct WFRedisResult
{
	protocol::RedisValue value;
	long long seqid;
	int task_state;
	int task_error;
	bool success;//task_state == WFT_STATE_SUCCESS && value.is_ok()
};

class WFRedisChain;//for method chaining

class WFRedisClient
{
public:
	using ON_COMPLETE = std::function<void (WFRedisResult&)>;
	using ON_SUCCESS = std::function<void (protocol::RedisValue&)>;
	using ON_ERROR = std::function<void (int state, int error, const std::string& errmsg)>;

public:
	WFRedisClient(const std::string& url);

	void default_retry_max(int n) { retry_max_ = n; }
	void default_send_timeout(int timeout) { send_timeout_ = timeout; }
	void default_recv_timeout(int timeout) { recv_timeout_ = timeout; }

	// return REG_ERR
	int parse_error() const { return parse_error_; }

	//sync
	WFRedisResult sync_request(const std::string& command,
							   const std::vector<std::string>& params);

	//async future
	WFFuture<WFRedisResult> async_request(const std::string& command,
										  const std::vector<std::string>& params);

	//async
	void request(const std::string& command,
				 const std::vector<std::string>& params,
				 WFRedisClient::ON_COMPLETE on_complete);

	void request(const std::string& command,
				 const std::vector<std::string>& params,
				 WFRedisClient::ON_SUCCESS on_success,
				 WFRedisClient::ON_ERROR on_error,
				 WFRedisClient::ON_COMPLETE on_complete);

	//async, method chaining style
	WFRedisChain request(const std::string& command);

	void set_send_timeout(int send_timeout) { send_timeout_ = send_timeout; }
	void set_recv_timeout(int recv_timeout) { recv_timeout_ = recv_timeout; }

private:
	ParsedURI uri_;
	int parse_error_;//REG_ERR
	int retry_max_;
	int send_timeout_;
	int recv_timeout_;
};

//client.request("HSET")("Key")({"Hashkey","Value"}).send();
//client.request(command).append(key).append(value).retry_max(n).success(cb1).error(cb2).send()
class WFRedisChain
{
public:
	void send();
	WFRedisTask *create_task();
	WFRedisChain& append(const std::string& param);
	WFRedisChain& append(const std::vector<std::string>& params);
	WFRedisChain& operator() (const std::string& param);
	WFRedisChain& operator() (const std::vector<std::string>& params);
	WFRedisChain& complete(WFRedisClient::ON_COMPLETE on_complete);
	WFRedisChain& success(WFRedisClient::ON_SUCCESS on_success);
	WFRedisChain& error(WFRedisClient::ON_ERROR on_error);
	WFRedisChain& retry_max(int n);
	WFRedisChain& send_timeout(int timeout);
	WFRedisChain& recv_timeout(int timeout);

private:
	WFRedisChain(const ParsedURI &uri,
				 const std::string& command,
				 int retry_max,
				 int send_timeout,
				 int recv_timeout):
		uri_(uri),
		command_(command),
		on_complete_(NULL),
		on_success_(NULL),
		on_error_(NULL),
		retry_max_(retry_max),
		send_timeout_(send_timeout),
		recv_timeout_(recv_timeout)
	{}

	ParsedURI uri_;
	std::string command_;
	std::vector<std::string> params_;
	WFRedisClient::ON_COMPLETE on_complete_;
	WFRedisClient::ON_SUCCESS on_success_;
	WFRedisClient::ON_ERROR on_error_;
	int retry_max_;
	int send_timeout_;
	int recv_timeout_;

	friend class WFRedisClient;
};

#endif

