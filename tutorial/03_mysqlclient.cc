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
#include <anyclient/WFMySQLClient.h>

using namespace std;

static WFFacilities::WaitGroup wg(3);

int main(int argc, char *argv[])
{
	//mysql://{username}:{password}@{host}:{port}/{db number}
	//example
	//  mysql://127.0.0.1
	//  mysql://root:qwerasdf@somehost.com:1412/testdb
	std::string mysql_url = "mysql://127.0.0.1:3306/test";
	WFMySQLClient msyql_client(mysql_url);

	msyql_client.default_retry_max(2);
	std::string insert_query = "instert into workflow (ID, TASK_NAME, TIMESTAMP) values (1412, \"HttpTask\", NOW());";
	std::string update_query = "update workflow set TASK_NAME='GoTask' where ID=1412";
	std::string select_query = "select * from workflow where ID>=1412 limit 3";

//// sync
	cout << "[1]Synchronous mysql request start" << endl;
	auto result1 = msyql_client.sync_request(insert_query);
	cout << "[1]Synchronous mysql request end. success: " << result1.success << endl;
	wg.done();

//// async
	cout << "[2]Semi-synchronous mysql request start" << endl;
	auto future = msyql_client.async_request(update_query);
	// do anything you want
	auto result2 = future.get();
	cout << "[2]Semi-synchronous mysql request end." << endl;
	cout << "success: " << result2.success << endl;
	wg.done();

//// async callback
	cout << "[3]Asynchronous mysql request start" << endl;
	msyql_client.request(select_query, [](WFMySQLResult& res) {
							cout << "[3]Asynchronous mysql request end. success: " << res.success << endl;

							map<string, protocol::MySQLCell> row;
							while (res.result_cursor.fetch_row(row))
							{
								cout << "---------- ROW ----------" << endl;
								for (const auto& kv : row)
								{
									const auto& cell = kv.second;
									cout << kv.first << ": type[" << datatype2str(cell.get_data_type()) << "] value[";
									if (cell.is_string() ||
										cell.is_date() ||
										cell.is_time() ||
										cell.is_datetime())
									{
										cout << cell.as_string();
									}
									else if (cell.is_int())
										cout << cell.as_int();
									else if (cell.is_ulonglong())
										cout << cell.as_ulonglong();
									else if (cell.is_float())
										cout << cell.as_float();
									else if (cell.is_double())
										cout << cell.as_double();
									else if (cell.is_null())
										cout << "NULL";
									else
										cout << cell.as_binary_string();

									cout << "]" << endl;
								}

								cout << "---------- END ----------" << endl;
							}

							wg.done();
						});

	wg.wait();
	return 0;
}

