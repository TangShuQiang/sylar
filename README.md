# 基于协程的C++服务器框架

## 性能测试

### 测试工具: AB httpd-tools

1. 安装工具

```shell
sudo apt install apache2-utils
```

2. 启动测试程序

```shell
cd sylar
make
bin/my_http_server
```

3. 压测命令

```shell
ab -n 1000000 -c 200 "http://127.0.0.1:8020/"
ab -n 1000000 -c 200 -k "http://127.0.0.1:8020/"  #长连接
```

4. 压测结果（Apple M3 8核）

|                 |              ours(QPS)              |         nginx/1.18.0(QPS)          |
| :-------------: | :---------------------------------: | :--------------------------------: |
| 1master+8worker |                  -                  | 58241.39 / 209406.63（keep-alive） |
|     1个线程     | 65172.95  /  84912.24（keep-alive） |                 -                  |
|     2个线程     |        81227.67 / 140316.20         |                 -                  |
|     3个线程     |        75324.73 / 176805.58         |                 -                  |
|     4个线程     |        65372.22 / 194265.55         |                 -                  |
|     5个线程     |        61155.99 / 149011.95         |                 -                  |
|     6个线程     |        56401.74 / 137128.33         |                 -                  |



