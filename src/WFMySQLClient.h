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

#ifndef _WFMYSQLCLIENT_H_
#define _WFMYSQLCLIENT_H_

#include <string>
#include <vector>
#include <functional>
#include "MySQLMessage.h"
#include "MySQLResult.h"
#include "URIParser.h"
#include "WFTaskFactory.h"
#include "WFFuture.h"

/**
 * @file   WFMySQLClient.h
 * @brief  Thread Safety MySQL Client, BUT not use for transaction or prepare. If you need transaction or prepare use WFMySQLConnection
 */

struct WFMySQLResult
{
	protocol::MySQLResponse resp;
	protocol::MySQLResultCursor result_cursor;
	long long seqid;
	int task_state;
	int task_error;
	bool success;//task_state == WFT_STATE_SUCCESS && (resp.get_packet_type() == MYSQL_PACKET_EOF || MYSQL_PACKET_ERROR || MYSQL_PACKET_OK)
};

class WFMySQLClient
{
public:
	using ON_COMPLETE = std::function<void (WFMySQLResult&)>;
	using ON_SUCCESS = std::function<void (protocol::MySQLResponse&, protocol::MySQLResultCursor&)>;
	using ON_ERROR = std::function<void (int state, int error, const std::string& errmsg)>;

public:
	WFMySQLClient(const std::string& url);

	void default_retry_max(int n) { retry_max_ = n; }
	void default_send_timeout(int timeout) { send_timeout_ = timeout; }
	void default_recv_timeout(int timeout) { recv_timeout_ = timeout; }

	// return REG_ERR
	int parse_error() const { return parse_error_; }

	//sync
	WFMySQLResult sync_request(const std::string& sql);

	//async future
	WFFuture<WFMySQLResult> async_request(const std::string& sql);

	//async
	void request(const std::string& sql,
				 WFMySQLClient::ON_COMPLETE on_complete);

	void request(const std::string& sql,
				 WFMySQLClient::ON_SUCCESS on_success,
				 WFMySQLClient::ON_ERROR on_error,
				 WFMySQLClient::ON_COMPLETE on_complete);

	void set_send_timeout(int send_timeout) { send_timeout_ = send_timeout; }
	void set_recv_timeout(int recv_timeout) { recv_timeout_ = recv_timeout; }

private:
	ParsedURI uri_;
	int parse_error_;//REG_ERR
	int retry_max_;
	int send_timeout_;
	int recv_timeout_;
};

#endif

