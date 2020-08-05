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

using namespace std;

int main(int argc, char *argv[])
{
//// sync
	cout << "sleep start" << endl;
	WFFacilities::usleep(1000 * 1000);
	cout << "Synchronous sleep one second over" << endl;

//// async future
	cout << "Semi-synchronous sleep one second start" << endl;
	auto future = WFFacilities::async_usleep(1000 * 1000);
	auto s = 0;
	for (int i = 0; i < 1000; i++)
		s += i;

	cout << "sum of 0-999: " << s << endl;
	future.wait();
	cout << "Semi-Synchronous sleep one second over" << endl;

	return 0;
}

