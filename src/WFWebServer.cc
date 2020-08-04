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

#include <workflow/StringUtil.h>
#include <workflow/HttpUtil.h>
#include "WFWebServer.h"

using namespace protocol;

class __WFHttpTask : public WFServerTask<HttpRequest, HttpResponse>
{
public:
	static size_t get_req_offset()
	{
		__WFHttpTask task(nullptr);

		return task.req_offset();
	}

	static size_t get_resp_offset()
	{
		__WFHttpTask task(nullptr);

		return task.resp_offset();
	}

private:
	__WFHttpTask(std::function<void (WFHttpTask *)> proc):
		WFServerTask(nullptr, proc)
	{}

	size_t req_offset() const
	{
		return (const char *)(&this->req) - (const char *)this;
	}

	size_t resp_offset() const
	{
		return (const char *)(&this->resp) - (const char *)this;
	}

};

void WFWebProcessor::process(WFHttpTask *task)
{
	auto *req = task->get_req();
	auto *resp = task->get_resp();
	std::string host;

	resp->set_http_version("HTTP/1.1");

	HttpHeaderCursor cursor(req);
	cursor.find("Host", host);

	if (host.empty())
	{
		//header Host not found
		HttpUtil::set_response_status(resp, HttpStatusBadRequest);
		return;
	}

	std::string request_uri;

	if (is_ssl_)
		request_uri = "https://";
	else
		request_uri = "http://";

	request_uri += host;
	request_uri += req->get_request_uri();

	ParsedURI uri;

	if (URIParser::parse(request_uri, uri) < 0)
	{
		//parse error
		//todo 500 status code when uri sys error
		HttpUtil::set_response_status(resp, HttpStatusBadRequest);
		return;
	}

	std::string path;

	if (uri.path && uri.path[0])
		path = uri.path;
	else
		path = "/";

	const processor_handler_t *p = NULL;

	pthread_rwlock_rdlock(&rwlock_);
	const auto it = handler_map_.find(path);

	if (it != handler_map_.cend())
		p = &it->second;
	else
	{
		if (path.back() != '/' &&
			handler_map_.find(path + '/') != handler_map_.end())
		{
			pthread_rwlock_unlock(&rwlock_);
			//302
			HttpUtil::set_response_status(resp, HttpStatusFound);
			resp->add_header_pair("Location", request_uri + '/');
			return;
		}

		// find longest pattern
		for (const auto& kv : path_map_)
		{
			if (StringUtil::start_with(path, kv.first))
			{
				p = kv.second;
				break;
			}
		}
	}

	pthread_rwlock_unlock(&rwlock_);

	if (!p)
	{
		HttpUtil::set_response_status(resp, HttpStatusNotFound);
		return;
	}

	const auto& handler = *p;

	if (handler)
		handler(task);
}

void WFWebProcessor::set_handler(std::string path, processor_handler_t&& handler)
{
	if (path.empty())
		return;

	pthread_rwlock_wrlock(&rwlock_);

	auto it = handler_map_.find(path);
	if (it != handler_map_.end())
		it->second = handler;
	else
		it = handler_map_.emplace(std::move(path), std::move(handler)).first;

	const std::string& key = it->first;
	if (key.back() == '/')
		path_map_[key] = &it->second;

	pthread_rwlock_unlock(&rwlock_);
}

WFWebServer::WFWebServer():
	WFHttpServer(std::bind(&WFWebProcessor::process,
						   &processor_,
						   std::placeholders::_1))
{}

WFWebServer::WFWebServer(const WFServerParams *params):
	WFHttpServer(params, std::bind(&WFWebProcessor::process,
								   &processor_,
								   std::placeholders::_1))
{}

class __WebFunctor
{
public:
	void operator() (WFHttpTask *task) const
	{
		if (handler_)
			handler_(*task->get_req(), *task->get_resp());
	}

	__WebFunctor(web_handler_t&& handler):
		handler_(std::move(handler))
	{}

	__WebFunctor(const __WebFunctor& copy):
		handler_(copy.handler_)
	{}

	__WebFunctor(__WebFunctor&& move):
		handler_(std::move(move.handler_))
	{}

private:
	web_handler_t handler_;
};

void WFWebServer::set_handler(const std::string& path, web_handler_t handler)
{
	processor_.set_handler(path, __WebFunctor(std::move(handler)));
}

void WFWebServer::set_handler(const std::string& path, processor_handler_t handler)
{
	processor_.set_handler(path, std::move(handler));
}

int WFWebServer::start(unsigned short port)
{
	int ret = this->WFHttpServer::start(port);

	processor_.ssl_start(false);
	return ret;
}

int WFWebServer::start(unsigned short port, const char *cert_file, const char *key_file)
{
	int ret = this->WFHttpServer::start(port, cert_file, key_file);

	processor_.ssl_start(true);
	return ret;
}

int WFWebServer::start(const char *host, unsigned short port)
{
	int ret = this->WFHttpServer::start(host, port);

	processor_.ssl_start(false);
	return ret;
}

int WFWebServer::start(const char *host, unsigned short port, const char *cert_file, const char *key_file)
{
	int ret = this->WFHttpServer::start(host, port, cert_file, key_file);

	processor_.ssl_start(true);
	return ret;
}

int WFWebServer::get_peer_addr(const HttpRequest& req, struct sockaddr *addr, socklen_t *addrlen)
{
	return task_of(req)->get_peer_addr(addr, addrlen);
}

WFHttpTask *WFWebServer::task_of(const HttpRequest& req)
{
	static size_t http_req_offset = __WFHttpTask::get_req_offset();

	return (WFHttpTask *)((char *)(&req) - http_req_offset);
}

WFHttpTask *WFWebServer::task_of(const HttpResponse& resp)
{
	static size_t http_resp_offset = __WFHttpTask::get_resp_offset();

	return (WFHttpTask *)((char *)(&resp) - http_resp_offset);
}

