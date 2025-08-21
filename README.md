# z2w-kv_store
基于C/C++的键值（KV）存储项目，支持多种底层存储引擎和丰富的操作接口，适用于高并发场景下的轻量级数据存储需求。
## 项目简介
`z2w-kv_store`是一个轻量级KV存储项目，核心特点：
- 支持多种底层存储引擎（数组、红黑树、哈希表、跳表）
- 基于Reactor模式和epoll实现高并发网络模型
- 提供多种操作接口（SET/GET/DEL/MOD/RANGE等）
## 快速开始
### 环境依赖
* 操作系统：Linux（推荐Ubuntu20.04+）
* 编译器：GCC 9.4+ 或 Clang 10+
* 构建工具：Make 或 CMake 3.22+
### 编译安装
```bash
# 克隆项目
git clone https://github.com/Zhuzewen2023/z2w-kv_store.git
cd z2w-kv_store

# 用 CMake 编译
mkdir build && cd build
cmake ..
make
```
### 启动服务器
```bash
./kv 192.168.10.99 7777
```
### 测试用例
使用测试工具kv_test验证功能
|模式编号|功能描述|
|--------|--------|
|1       |测试数组存储引擎SET,GET,DEL,MOD,EXIST性能|
|2       |测试红黑树存储引擎SET,GET,DEL,MOD,EXIST性能|
|3       |测试哈希表存储引擎SET,GET,DEL,MOD,EXIST性能|
|4       |测试跳表存储引擎SET,GET,DEL,MOD,EXIST性能|
|5       |测试数组存储引擎SET大量键值性能|
|6       |测试红黑树存储引擎SET大量键值性能|
|7       |测试哈希表存储引擎SET大量键值性能|
|8       |测试跳表存储引擎SET大量键值性能|
|9       |测试数组存储引擎落盘性能|
|10      |测试红黑树存储引擎落盘性能|
|11      |测试哈希表存储引擎落盘性能|
|12      |测试跳表存储引擎落盘性能|
|13      |测试数组存储引擎读盘性能|
|14      |测试红黑树存储引擎读盘性能|
|15      |测试哈希表存储引擎读盘性能|
|16      |测试跳表存储引擎读盘性能|
|17      |测试数组

## 性能测试
### 数组存储引擎
```C
test_kv_case(connfd, "SET Name ZZW\n", "SUCCESS\n", "SETCase");
test_kv_case(connfd, "GET Name\n", "ZZW\n", "GETCase");
test_kv_case(connfd, "SET Name Linus\n", "EXIST\n", "SETCase");
test_kv_case(connfd, "MOD Name Linus\n", "SUCCESS\n", "MODCase");
test_kv_case(connfd, "EXIST Name\n", "EXIST\n", "EXISTCase");
test_kv_case(connfd, "GET Name\n", "Linus\n", "GETCase");
test_kv_case(connfd, "DEL Name\n", "SUCCESS\n", "DELCase");
test_kv_case(connfd, "GET Name\n", "NO EXIST\n", "GETCase");
test_kv_case(connfd, "MOD Name Linus\n", "ERROR\n", "MODCase");
test_kv_case(connfd, "EXIST Name\n", "NO EXIST\n", "EXISTCase");
```
> 进行10次指令，重复10000次
<img width="405" height="105" alt="image" src="https://github.com/user-attachments/assets/7cd681d9-b360-40c7-826b-95b0537fc53f" />

### 红黑树存储引擎
```C
test_rb_case(connfd, "RSET Name ZZW\n", "SUCCESS\n", "RSETCase");
test_rb_case(connfd, "RGET Name\n", "ZZW\n", "RGETCase");
test_rb_case(connfd, "RSET Name Linus\n", "EXIST\n", "RSETCase");
test_rb_case(connfd, "RMOD Name Linus\n", "SUCCESS\n", "RMODCase");
test_rb_case(connfd, "REXIST Name\n", "EXIST\n", "REXISTCase");
test_rb_case(connfd, "RGET Name\n", "Linus\n", "RGETCase");
test_rb_case(connfd, "RDEL Name\n", "SUCCESS\n", "RDELCase");
test_rb_case(connfd, "RGET Name\n", "NO EXIST\n", "RGETCase");
test_rb_case(connfd, "RMOD Name Linus\n", "ERROR\n", "RMODCase");
test_rb_case(connfd, "REXIST Name\n", "NO EXIST\n", "REXISTCase");
```
> 进行10次指令，重复10000次
<img width="496" height="109" alt="image" src="https://github.com/user-attachments/assets/d87422ca-0bcb-4454-80ec-ab431c179449" />

### 哈希表存储引擎
```C
test_hash_case(connfd, "HSET Name ZZW\n", "SUCCESS\n", "HSETCase");
test_hash_case(connfd, "HGET Name\n", "ZZW\n", "HGETCase");
test_hash_case(connfd, "HSET Name Linus\n", "EXIST\n", "HSETCase");
test_hash_case(connfd, "HMOD Name Linus\n", "SUCCESS\n", "HMODCase");
test_hash_case(connfd, "HEXIST Name\n", "EXIST\n", "HEXISTCase");
test_hash_case(connfd, "HGET Name\n", "Linus\n", "HGETCase");
test_hash_case(connfd, "HDEL Name\n", "SUCCESS\n", "HDELCase");
test_hash_case(connfd, "HGET Name\n", "NO EXIST\n", "HGETCase");
test_hash_case(connfd, "HMOD Name Linus\n", "ERROR\n", "HMODCase");
test_hash_case(connfd, "HEXIST Name\n", "NO EXIST\n", "HEXISTCase");
```
> 进行10次指令，重复10000次
<img width="490" height="105" alt="image" src="https://github.com/user-attachments/assets/92b783d1-20d1-4ebf-9f48-df9d4b2bd793" />

### 跳表存储引擎
```C
test_kv_case(connfd, "SSET Name ZZW\n", "SUCCESS\n", "SSETCase");
test_kv_case(connfd, "SGET Name\n", "ZZW\n", "SGETCase");
test_kv_case(connfd, "SSET Name Linus\n", "EXIST\n", "SSETCase");
test_kv_case(connfd, "SMOD Name Linus\n", "SUCCESS\n", "SMODCase");
test_kv_case(connfd, "SEXIST Name\n", "EXIST\n", "SEXISTCase");
test_kv_case(connfd, "SGET Name\n", "Linus\n", "SGETCase");
test_kv_case(connfd, "SDEL Name\n", "SUCCESS\n", "SDELCase");
test_kv_case(connfd, "SGET Name\n", "NO EXIST\n", "SGETCase");
test_kv_case(connfd, "SMOD Name Linus\n", "ERROR\n", "SMODCase");
test_kv_case(connfd, "SEXIST Name\n", "NO EXIST\n", "SEXISTCase");
```
> 进行10次指令，重复10000次
<img width="440" height="109" alt="image" src="https://github.com/user-attachments/assets/406b2350-8aeb-4a5b-ad7c-ac41d4bb8b14" />

