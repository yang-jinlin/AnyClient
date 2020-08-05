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
#include <anyclient/WFHttpClient.h>

using namespace std;

static WFFacilities::WaitGroup wg(4);

int main(int argc, char *argv[])
{
	WFHttpClient http_client;

	http_client.default_redirect_max(5);
	http_client.default_retry_max(2);

	map<string, string> http_headers;
	http_headers["user-agent"] = "AnyClient/1.0.0";
	http_headers["cookie"] = "emotion=happy;code=simple";

	std::string http_body;

	http_body.clear();//no need http body data

//// sync
	cout << "[1]Synchronous http request start" << endl;
	auto result1 = http_client.sync_request("GET",
											"https://www.sogou.com",
											http_headers,
											http_body);
	cout << "[1]Synchronous http request end. HttpStaus: " << result1.status_code << endl;
	wg.done();

//// async
	cout << "[2]Semi-synchronous http request start" << endl;
	auto future = http_client.async_request("GET",
											"https://www.baidu.com",
											http_headers,
											http_body);
	// do anything you want
	auto result2 = future.get();
	cout << "[2]Semi-synchronous http request end. HttpStaus: " << result2.status_code << endl;
	wg.done();

//// async callback
	cout << "[3]Asynchronous http request start" << endl;
	http_client.request("GET",
						"https://www.sogou.com",
						http_headers,
						http_body,
						[](WFHttpResult& res) {
							cout << "[3]Asynchronous http request end. HttpStaus: " << res.status_code << endl;
							wg.done();
						});

//// async chain
	cout << "[4]Asynchronous http request start" << endl;
	http_client.request("GET", "https://www.baidu.com")
				.set_header("user-agent", "AnyClient/1.0.0")
				.set_header("cookie", "1412")
				.set_header("cookie", "1412")
				.success([](protocol::HttpResponse& http_resp) {
					cout << "[4]Asynchronous http request end. HttpStaus: " << http_resp.get_status_code() << endl;
					wg.done();
				})
				.error([](int state, int error, const std::string& errmsg) {
					cout << "[4]Asynchronous http request end. error: " << errmsg << endl;
					wg.done();
				})
				.send();

	wg.wait();
	return 0;
}

