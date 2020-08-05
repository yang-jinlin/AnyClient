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

static WFFacilities::WaitGroup wg(8);
static int result[8];

void go_routine(int idx, int x, int y)
{
	result[idx] = x * x + y * y;
	wg.done();
}

int main(int argc, char *argv[])
{
	for (int i = 0; i < 8; i++)
		WFFacilities::go("compute", go_routine, i, i + 26, i + 62);

	cout << "---wait for compute" << endl;
	wg.wait();
	cout << "---all over" << endl;

	for (int i = 0; i < 8; i++)
		cout << "result[" << i << "] = " << result[i] << endl;

	return 0;
}

