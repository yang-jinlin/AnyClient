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

#include <pthread.h>
#include <map>
#include <unordered_map>

using processor_handler_t = std::function<void (WFHttpTask *)>;

class PathCompare
{
public:
	bool operator() (const std::string& x, const std::string& y) const
	{
		if (x.size() != y.size())
			return x.size() > y.size();

		return x < y;
	}
};

class WFWebProcessor
{
public:
	WFWebProcessor():
		rwlock_(PTHREAD_RWLOCK_INITIALIZER),
		is_ssl_(false)
	{}

	void process(WFHttpTask *task);

	void set_handler(std::string path, processor_handler_t&& handler);

	void ssl_start(bool is_ssl)
	{
		is_ssl_ = is_ssl;
	}

private:
	std::map<std::string, processor_handler_t *, PathCompare> path_map_;
	std::unordered_map<std::string, processor_handler_t> handler_map_;
	pthread_rwlock_t rwlock_;
	bool is_ssl_;
};

