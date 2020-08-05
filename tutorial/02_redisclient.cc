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

  Author: Wu jiaxu (wujiaxu@sogou-inc.com;void00@foxmail.com)
*/

#include <iostream>
#include <workflow/WFFacilities.h>
#include <anyclient/WFRedisClient.h>

using namespace std;

static WFFacilities::WaitGroup wg(4);

int main(int argc, char *argv[])
{
	//redis://:{password}@{host}:{port}/{db number}
	//example
	//  redis://127.0.0.1
	//  redis://:qwerasdf@somehost.com:1122/8
	std::string redis_url = "redis://:1412@127.0.0.1:6379/1";
	WFRedisClient redis_client(redis_url);

	redis_client.default_retry_max(2);

//// sync
	cout << "[1]Synchronous redis request start" << endl;
	auto result1 = redis_client.sync_request("SET", {"testkey", "testvalue"});
	cout << "[1]Synchronous redis request end. success: " << result1.success << endl;
	wg.done();

//// async
	cout << "[2]Semi-synchronous redis request start" << endl;
	auto future = redis_client.async_request("GET", {"testkey"});
	// do anything you want
	auto result2 = future.get();
	cout << "[2]Semi-synchronous redis request end." << endl;
	cout << "success: " << result2.success << endl;
	cout << "response-string: " << result2.value.string_value() << endl;
	wg.done();

//// async callback
	cout << "[3]Asynchronous redis request start" << endl;
	redis_client.request("DEL", {"testkey"}, [](WFRedisResult& res) {
							cout << "[3]Asynchronous redis request end. success: " << res.success << endl;
							wg.done();
						});

//// async chain
	cout << "[4]Asynchronous redis request start" << endl;
	redis_client.request("MGET")
				.append("testkey")
				.append("somekey")
				.append("12345")
				.success([](protocol::RedisValue& val) {
					cout << "[4]Asynchronous redis request end. success: " << val.is_ok() << endl;
					for (size_t i = 0; i < val.arr_size(); i++)
						cout << "[" << i << "] element-string: " << val[i].string_value() << endl;

					wg.done();
				})
				.error([](int state, int error, const std::string& errmsg) {
					cout << "[4]Asynchronous redis request end. error: " << errmsg << endl;
					wg.done();
				})
				.send();

	wg.wait();
	return 0;
}

