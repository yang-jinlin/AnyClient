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
#include <thread>
#include <workflow/WFFacilities.h>

using namespace std;

void thread_routine(int idx, WFFacilities::WaitGroup *wg)
{
	WFFacilities::usleep(idx * 1000 * 1000);
	cout << "thread [" << idx << "] is over" << endl;
	wg->done();//reduce waitgroup
}

int main(int argc, char *argv[])
{
	WFFacilities::WaitGroup wg(3);//Thread-Safety, need 3 times done
	std::thread *th[8];

	for (int i = 0; i < 8; i++)
		th[i] = new std::thread(thread_routine, i, &wg);

	cout << "---wait for couter" << endl;
	wg.wait();
	cout << "---couter is over" << endl;

	//wg.wait();//Reentrant for WaitGroup.wait(), can wait in any

	for (int i = 0; i < 8; i++)
	{
		th[i]->join();
		delete th[i];
	}

	return 0;
}

