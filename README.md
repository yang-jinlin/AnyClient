# AnyClient

## C++万能客户端
- Http Client
- Redis Client
- MySQL Client
- Kafka Client

## 特点
- 极致高性能
- 线程安全
- 自带连接池
- 自带线程池
- 全异步工作模式
- 支持异步回调、半同步future、同步等待三套接口
- 协议无关，可以模仿现有client开发自定义协议

## 网络IO
### HttpClient
~~~cpp
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
~~~

### RedisClient
~~~cpp
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
~~~

### MySQLClient
~~~cpp
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
~~~

### KafkaClient
workflow此项功能进入开发尾声，即将开放！

## 磁盘IO
~~~cpp
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
~~~

## 定时器
~~~cpp
cout << "Semi-synchronous sleep one second start" << endl;
auto future = WFFacilities::async_usleep(1000 * 1000);
auto s = 0;
for (int i = 0; i < 1000; i++)
    s += i;

cout << "sum of 0-999: " << s << endl;
future.wait();
cout << "Semi-Synchronous sleep one second over" << endl;
~~~

## 计数器
~~~cpp
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
~~~

## CPU
~~~cpp
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
~~~

## GPU
workflow此项功能目前处于闭源状态，``WFCUDATask``暂不开放任何源码和接口实现

## AnyRPC
SogouRPC即将开源，以下功能即将开放！
### Thrift Binary
#### Thrift Framed Transport
#### Thrift Http Transport

### BRPC
#### BaiduRPC Standard

### SRPC
#### SogouRCP Binary
#### SogouRCP Http

