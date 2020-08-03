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

#ifndef _WFWEBSERVER_H_
#define _WFWEBSERVER_H_

#include <string>
#include <functional>
#include "WFHttpServer.h"

/**
 * @file   WFWebServer.h
 * @brief  Simple Web Server by custom path handler
 */

using web_handler_t = std::function<void (const protocol::HttpRequest&,
											protocol::HttpResponse&)>;

#include "WFWebServer.inl"

class WFWebServer : public WFHttpServer
{
public:
	WFWebServer();
	WFWebServer(const WFServerParams *params);

	// if multi-path/server use one same functor for handler,
	// please use std::ref on handler
	void set_handler(const std::string& path, web_handler_t handler);
	void set_handler(const std::string& path,
					 std::function<void (WFHttpTask *)> handler);

	int start(unsigned short port);
	int start(unsigned short port, const char *cert_file, const char *key_file);
	int start(const char *host, unsigned short port);
	int start(const char *host, unsigned short port,
			  const char *cert_file, const char *key_file);

public:
	static int get_peer_addr(const protocol::HttpRequest& req,
							struct sockaddr *addr, socklen_t *addrlen);
	static WFHttpTask *task_of(const protocol::HttpRequest& req);
	static WFHttpTask *task_of(const protocol::HttpResponse& resp);

private:
	WFWebProcessor processor_;
};

#endif

