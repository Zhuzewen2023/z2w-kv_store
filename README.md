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
//todo

## 性能测试
系统配置：
Ubuntu 22.04 64位 2核4G
### 数组存储引擎
> 单线程按序进行SET,GET,SET,MOD,EXIST,GET,DEL,GET,MOD,EXIST指令，重复10000次
<img width="405" height="105" alt="image" src="https://github.com/user-attachments/assets/7cd681d9-b360-40c7-826b-95b0537fc53f" />

--------------------------------------------------------------

> 开启4个线程，按序进行SET,GET,SET,MOD,EXIST,GET,DEL,GET,MOD,EXIST指令，重复10000次，合计重复40000次，执行40万条指令
<img width="659" height="335" alt="image" src="https://github.com/user-attachments/assets/43d37e1e-cc82-43f0-a64c-d5a06d6f6b38" />

--------------------------------------------------------------
> 单线程分别对每条SET,GET,SET,MOD,EXIST,GET,DEL,GET,MOD,EXIST指令重复100000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令

太慢，跑了几小时

-------------------------------------

> 开启5个线程，分别对每条SET,GET,SET,MOD,EXIST,GET,DEL,GET,MOD,EXIST指令重复20000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="650" height="363" alt="image" src="https://github.com/user-attachments/assets/0af48936-65f4-4dbc-9437-f092f6d77dd2" />

------------------------------------

> 单线程存储10W键值，每次插入的键值后缀按照重复次数累加
<img width="343" height="120" alt="image" src="https://github.com/user-attachments/assets/20173e5c-1066-48b1-9e14-da9925297d5f" />

------------------------------------

> 单线程读取存储的10W键值
<img width="395" height="120" alt="image" src="https://github.com/user-attachments/assets/2ba8f6f5-1a99-484c-a23b-1933c9e64bf6" />

--------------------------------------

> 单线程将SET,GET,SET,MOD,EXIST,GET,DEL,GET,MOD,EXIST指令合并为1条指令发送，重复100000次
<img width="511" height="110" alt="image" src="https://github.com/user-attachments/assets/9c47d66d-430d-4255-a7e9-5367ef21a6fa" />

----------------------------------
------------------------------------

### 红黑树存储引擎

> 单线程按序进行RSET,RGET,RSET,RMOD,REXIST,RGET,RDEL,RGET,RMOD,REXIST指令，重复10000次
<img width="496" height="109" alt="image" src="https://github.com/user-attachments/assets/d87422ca-0bcb-4454-80ec-ab431c179449" />

--------------------------------------------------------------

> 开启4个线程，按序进行RSET,RGET,RSET,RMOD,REXIST,RGET,RDEL,RGET,RMOD,REXIST指令，重复10000次，合计重复40000次，执行40万条指令
<img width="521" height="308" alt="image" src="https://github.com/user-attachments/assets/df9d965f-06c3-447f-83f0-98cee2aaf4ee" />

--------------------------------------------------------------

> 单线程分别对每条RSET,RGET,RSET,RMOD,REXIST,RGET,RDEL,RGET,RMOD,REXIST指令重复100000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="399" height="103" alt="image" src="https://github.com/user-attachments/assets/13322cbd-97fd-43e7-9520-737a20c411c3" />

--------------------------------------------------------------

> 开启5个线程，分别对每条RSET,RGET,RSET,RMOD,REXIST,RGET,RDEL,RGET,RMOD,HEXIST指令重复20000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="524" height="336" alt="image" src="https://github.com/user-attachments/assets/87c826b1-19cd-4761-8881-316cc8b56a67" />

--------------------------------------------------------------

> 单线程存储10W键值，每次插入的键值后缀按照重复次数累加
<img width="395" height="120" alt="image" src="https://github.com/user-attachments/assets/6b35e59c-e34f-4290-9a6e-58cb5a29c115" />

--------------------------------------------------------------

> 单线程读取存储的10W键值
<img width="465" height="120" alt="image" src="https://github.com/user-attachments/assets/ff40577d-dbe9-4d52-9937-6eb0a9b7b664" />

--------------------------------------------------------------

> 单线程将RSET,RGET,RSET,RMOD,REXIST,RGET,RDEL,RGET,RMOD,REXIST指令合并为1条指令发送，重复100000次
<img width="610" height="106" alt="image" src="https://github.com/user-attachments/assets/c9701cf9-6982-4d0a-86cc-9a50a7f14985" />

--------------------------------------------------------------

--------------------------
--------------------------

### 哈希表存储引擎

> 单线程按序进行HSET,HGET,HSET,HMOD,HEXIST,HGET,HDEL,HGET,HMOD,HEXIST指令，重复10000次
<img width="490" height="105" alt="image" src="https://github.com/user-attachments/assets/92b783d1-20d1-4ebf-9f48-df9d4b2bd793" />

--------------------------------------------------------------

> 开启4个线程，按序进行HSET,HGET,HSET,HMOD,HEXIST,HGET,HDEL,HGET,HMOD,HEXIST指令，重复10000次，合计重复40000次,执行40万条指令
<img width="458" height="305" alt="image" src="https://github.com/user-attachments/assets/fab51427-1b2b-4ea3-9026-e749c63fad35" />

--------------------------------------------------------------

> 单线程分别对每条HSET,HGET,HSET,HMOD,HEXIST,HGET,HDEL,HGET,HMOD,HEXIST指令重复100000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="344" height="104" alt="image" src="https://github.com/user-attachments/assets/a1518373-9b28-4e63-b516-cf4c042b0b47" />

-----------------------------------------------------------------

> 开启5个线程，分别对每条HSET,HGET,HSET,HMOD,HEXIST,HGET,HDEL,HGET,HMOD,HEXIST指令重复20000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="541" height="336" alt="image" src="https://github.com/user-attachments/assets/125faee5-6d09-447a-8e67-1e4dd9dc7434" />
 
-----------------------------------------------------------------

> 单线程存储10W键值，每次插入的键值后缀按照重复次数累加
<img width="481" height="119" alt="image" src="https://github.com/user-attachments/assets/3763e128-acd4-4747-8a06-cbbcc54bfe26" />

-----------------------------------------------------------------

> 单线程读取存储的10W键值
<img width="500" height="129" alt="image" src="https://github.com/user-attachments/assets/d1bae8e2-2bf2-495e-8e74-c7370978725d" />

-------------------------------------------------------------------

> 单线程将HSET,HGET,HSET,HMOD,HEXIST,HGET,HDEL,HGET,HMOD,HEXIST指令合并为1条指令发送，重复100000次
<img width="595" height="114" alt="image" src="https://github.com/user-attachments/assets/cbeefa2a-b605-4123-b6d0-103aef0c4c2a" />

-------------------------------------------
-------------------------------------------

### 跳表存储引擎

> 单线程按序进行SSET,SGET,SSET,SMOD,SEXIST,SGET,SDEL,SGET,SMOD,SEXIST指令，重复10000次
<img width="440" height="109" alt="image" src="https://github.com/user-attachments/assets/406b2350-8aeb-4a5b-ad7c-ac41d4bb8b14" />

--------------------------------------------------------------

> 开启4个线程，按序进行SSET,SGET,SSET,SMOD,SEXIST,SGET,SDEL,SGET,SMOD,SEXIST指令，重复10000次，合计重复40000次，执行40万条指令
<img width="446" height="304" alt="image" src="https://github.com/user-attachments/assets/a95d2339-58c3-4f2a-8807-9972bf87cf0f" />

--------------------------------------------------------------

> 单线程分别对每条SSET,SGET,SSET,SMOD,SEXIST,SGET,SDEL,SGET,SMOD,SEXIST指令重复100000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="413" height="101" alt="image" src="https://github.com/user-attachments/assets/f5740bda-a337-49d3-b8e4-a3dd86a755dc" />

---------------------------------------------------------------

> 开启5个线程，分别对每条SSET,SGET,SSET,SMOD,SEXIST,SGET,SDEL,SGET,SMOD,SEXIST指令重复20000次，每次插入的键值后缀按照重复次数累加，共计执行100万次指令
<img width="565" height="340" alt="image" src="https://github.com/user-attachments/assets/2b08e1b5-ca60-478a-bb1f-9e2e70ae1aca" />

-----------------------------------------------------------------

> 单线程存储10W键值，每次插入的键值后缀按照重复次数累加
<img width="425" height="119" alt="image" src="https://github.com/user-attachments/assets/aaaf7005-0740-440a-926d-d0d78e4a29c9" />

-----------------------------------------------------------------

> 单线程读取存储的10W键值
<img width="486" height="124" alt="image" src="https://github.com/user-attachments/assets/30a09a6e-4daf-4f30-be75-cb197bb6fbd2" />

------------------------------------------------------------------

> 单线程将SSET,SGET,SSET,SMOD,SEXIST,SGET,SDEL,SGET,SMOD,SEXIST指令合并为1条指令发送，重复100000次
<img width="496" height="104" alt="image" src="https://github.com/user-attachments/assets/46ff3747-9a23-44b8-b8f9-7945295deb9c" />

-------------------------------------------------------------------
--------------------------------------------------------------------










