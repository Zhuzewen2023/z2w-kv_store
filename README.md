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
系统配置：
Ubuntu 22.04 64位 2核4G
### 数组存储引擎
> 按序进行SET,GET,SET,MOD,EXIST,GET,DEL,GET,MOD,EXIST指令，重复10000次
<img width="405" height="105" alt="image" src="https://github.com/user-attachments/assets/7cd681d9-b360-40c7-826b-95b0537fc53f" />

--------------------------------------------------------------
> 分别对每条SET,GET,SET,MOD,EXIST,GET,DEL,GET,MOD,EXIST指令重复100000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令

太慢，跑了几小时

------------------------------------

> 存储10W键值，每次插入的键值后缀按照重复次数累加
<img width="343" height="120" alt="image" src="https://github.com/user-attachments/assets/20173e5c-1066-48b1-9e14-da9925297d5f" />

----------------------------------
------------------------------------

### 红黑树存储引擎

> 按序进行RSET,RGET,RSET,RMOD,REXIST,RGET,RDEL,RGET,RMOD,REXIST指令，重复10000次
<img width="496" height="109" alt="image" src="https://github.com/user-attachments/assets/d87422ca-0bcb-4454-80ec-ab431c179449" />

--------------------------------------------------------------

> 分别对每条RSET,RGET,RSET,RMOD,REXIST,RGET,RDEL,RGET,RMOD,REXIST指令重复100000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="399" height="103" alt="image" src="https://github.com/user-attachments/assets/13322cbd-97fd-43e7-9520-737a20c411c3" />

--------------------------------------------------------------

> 存储10W键值，每次插入的键值后缀按照重复次数累加
<img width="395" height="120" alt="image" src="https://github.com/user-attachments/assets/6b35e59c-e34f-4290-9a6e-58cb5a29c115" />

--------------------------
--------------------------

### 哈希表存储引擎

> 按序进行HSET,HGET,HSET,HMOD,HEXIST,HGET,HDEL,HGET,HMOD,HEXIST指令，重复10000次
<img width="490" height="105" alt="image" src="https://github.com/user-attachments/assets/92b783d1-20d1-4ebf-9f48-df9d4b2bd793" />

--------------------------------------------------------------

> 分别对每条HSET,HGET,HSET,HMOD,HEXIST,HGET,HDEL,HGET,HMOD,HEXIST指令重复100000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="344" height="104" alt="image" src="https://github.com/user-attachments/assets/a1518373-9b28-4e63-b516-cf4c042b0b47" />

-----------------------------------------------------------------

> 存储10W键值，每次插入的键值后缀按照重复次数累加
<img width="481" height="119" alt="image" src="https://github.com/user-attachments/assets/3763e128-acd4-4747-8a06-cbbcc54bfe26" />

-------------------------------------------
-------------------------------------------

### 跳表存储引擎

> 按序进行SSET,SGET,SSET,SMOD,SEXIST,SGET,SDEL,SGET,SMOD,SEXIST指令，重复10000次
<img width="440" height="109" alt="image" src="https://github.com/user-attachments/assets/406b2350-8aeb-4a5b-ad7c-ac41d4bb8b14" />

--------------------------------------------------------------

> 分别对每条SSET,SGET,SSET,SMOD,SEXIST,SGET,SDEL,SGET,SMOD,SEXIST指令重复100000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="413" height="101" alt="image" src="https://github.com/user-attachments/assets/f5740bda-a337-49d3-b8e4-a3dd86a755dc" />

-----------------------------------------------------------------

> 存储10W键值，每次插入的键值后缀按照重复次数累加
<img width="425" height="119" alt="image" src="https://github.com/user-attachments/assets/aaaf7005-0740-440a-926d-d0d78e4a29c9" />


-------------------------------------------------------------------
--------------------------------------------------------------------

