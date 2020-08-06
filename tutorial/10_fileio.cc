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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <workflow/WFFacilities.h>

using namespace std;

static void __async_bio()
{
	uint64_t write_data = 0x1234;
	uint64_t read_data = 0;
	ssize_t sz;
	int fd = open("bio.test", O_RDWR | O_TRUNC | O_CREAT, 0644);

	auto future1 = WFFacilities::async_pwrite(fd, &write_data, 8, 0);
	// do anything you want
	sz = future1.get();
	cout << "write size = " << sz << endl;

	auto future2 = WFFacilities::async_pread(fd, &read_data, 8, 0);
	// do anything you want
	sz = future2.get();
	cout << "read size = " << sz << endl;
	if (memcmp(&read_data, &write_data, 8) != 0)
		abort();

	close(fd);
}

static void __async_dio()
{
	char *write_data;
	char *read_data;
	ssize_t sz;
	int fd = open("dio.test", O_RDWR | O_TRUNC | O_DIRECT | O_CREAT, 0644);

	posix_memalign((void **)&write_data, 4096, 4096);
	posix_memalign((void **)&read_data, 4096, 4096);

	for (int i = 0; i < 4096; i++)
		write_data[i] = (char)i;

	auto future1 = WFFacilities::async_pwrite(fd, write_data, 4096, 0);
	// do anything you want
	sz = future1.get();
	cout << "write size = " << sz << endl;

	auto future2 = WFFacilities::async_pread(fd, read_data, 4096, 0);
	// do anything you want
	sz = future2.get();
	cout << "read size = " << sz << endl;
	if (memcmp(read_data, write_data, 4096) != 0)
		abort();

	close(fd);
}

int main(int argc, char *argv[])
{
	__async_bio();
	__async_dio();

	return 0;
}

