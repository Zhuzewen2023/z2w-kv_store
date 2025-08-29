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
|模式编号|	功能描述	|核心测试场景 / 指令	|测试维度（性能 / 功能）|
|--------|--------|-------------------|----------------------|
|1	|测试数组存储引擎基础指令性能	|基础操作：SET（设值）、GET（取值）、DEL（删除）、MOD（修改）、EXIST（存在性判断）	|单指令性能（响应时间、QPS）|
|2	|测试红黑树存储引擎基础指令性能|	基础操作：SET、GET、DEL、MOD、EXIST	|单指令性能（响应时间、QPS）|
|3	|测试哈希表存储引擎基础指令性能|	基础操作：SET、GET、DEL、MOD、EXIST|	单指令性能（响应时间、QPS）|
|4	|测试跳表存储引擎基础指令性能	|基础操作：SET、GET、DEL、MOD、EXIST	|单指令性能（响应时间、QPS）|
|5	|测试数组存储引擎处理大量键值对的性能	|批量操作：连续执行大量 SET 指令（模拟高数据量场景）|	批量写入性能（吞吐量）|
|6	|测试红黑树存储引擎处理大量键值对的性能	|批量操作：连续执行大量 SET 指令（模拟高数据量场景）|	批量写入性能（吞吐量）|
|7	|测试哈希表存储引擎处理大量键值对的性能	|批量操作：连续执行大量 SET 指令（模拟高数据量场景）|	批量写入性能（吞吐量）|
|8	|测试跳表存储引擎处理大量键值对的性能	|批量操作：连续执行大量 SET 指令（模拟高数据量场景）	|批量写入性能（吞吐量）|
|9	|测试数组存储引擎数据落盘性能	|持久化操作：将内存中的数组 KV 数据完整写入磁盘（如本地文件）	|持久化性能（落盘耗时、IO 速率）|
|10|	测试红黑树存储引擎数据落盘性能	|持久化操作：将内存中的红黑树 KV 数据完整写入磁盘	|持久化性能（落盘耗时、IO 速率）|
|11|	测试哈希表存储引擎数据落盘性能	|持久化操作：将内存中的哈希表 KV 数据完整写入磁盘	|持久化性能（落盘耗时、IO 速率）|
|12|	测试跳表存储引擎数据落盘性能	|持久化操作：将内存中的跳表 KV 数据完整写入磁盘	|持久化性能（落盘耗时、IO 速率）|
|13	|测试数组存储引擎数据读盘性能|	恢复操作：从磁盘读取已落盘的数组 KV 数据并加载到内存	|恢复性能（读盘耗时、加载速率）|
|14	|测试红黑树存储引擎数据读盘性能	|恢复操作：从磁盘读取已落盘的红黑树 KV 数据并加载到内存|	恢复性能（读盘耗时、加载速率）|
|15	|测试哈希表存储引擎数据读盘性能|	恢复操作：从磁盘读取已落盘的哈希表 KV 数据并加载到内存|	恢复性能（读盘耗时、加载速率）|
|16	|测试跳表存储引擎数据读盘性能|	恢复操作：从磁盘读取已落盘的跳表 KV 数据并加载到内存|	恢复性能（读盘耗时、加载速率）|
|17	|测试数组存储引擎合并指令性能	|复合操作：SET→GET→SET→MOD→EXIST→GET→DEL→GET→MOD→EXIST（连续多指令组合）	|复合指令性能（连续操作吞吐量）|
|18	|测试红黑树存储引擎合并指令性能	|复合操作：RSET→RGET→RSET→RMOD→REXIST→RGET→RDEL→RGET→RMOD→REXIST（红黑树专属指令）	|复合指令性能（连续操作吞吐量）|
|19	|测试哈希表存储引擎合并指令性能	|复合操作：HSET→HGET→HSET→HMOD→HEXIST→HGET→HDEL→HGET→HMOD→HEXIST（哈希表专属指令）|	复合指令性能（连续操作吞吐量）|
|20	|测试跳表存储引擎合并指令性能	|复合操作：SSET→SGET→SSET→SMOD→SEXIST→SGET→SDEL→SGET→SMOD→SEXIST（跳表专属指令）|	复合指令性能（连续操作吞吐量）|
|21	|测试全部存储引擎合并指令性能|	批量复合操作：同时对数组、红黑树、哈希表、跳表执行各自的合并指令（17-20 场景汇总）|	多引擎对比性能（跨引擎吞吐量）|
|22	|测试数组存储引擎范围查询指令性能	|范围操作：按键范围查询（如 “查询键前缀为 Name_ 的所有键值对”）|范围查询性能（查询耗时、结果完整性）|
|23	|测试红黑树存储引擎范围查询指令性能	|范围操作：按键排序范围查询（红黑树有序特性，支持高效区间查询）|	范围查询性能（查询耗时、排序准确性）|
|24	|测试哈希表存储引擎范围查询指令性能	|范围操作：按键范围查询（哈希表无序，需遍历筛选，对比有序引擎性能）|	范围查询性能（查询耗时、结果完整性）|
|25	|测试跳表存储引擎范围查询指令性能	|范围操作：按键排序范围查询（跳表有序特性，支持高效区间查询）|	范围查询性能（查询耗时、排序准确性）|
|26	|测试数组存储引擎同步指令（源端角色）	|同步操作：作为 “数据源”，向目标端（Dest）推送数组 KV 数据（如全量 / 增量同步）|	同步性能（数据推送速率、完整性）|
|27	|测试数组存储引擎同步指令（目标端角色）	|同步操作：作为 “目标端”，接收源端（Source）推送的数组 KV 数据并写入本地	|同步性能（数据接收速率、一致性）|
|28	|测试红黑树存储引擎同步指令（源端角色）	|同步操作：作为 “数据源”，向目标端推送红黑树 KV 数据（红黑树专属同步逻辑）	|同步性能（数据推送速率、完整性）|
|29	|测试红黑树存储引擎同步指令（目标端角色）	|同步操作：作为 “目标端”，接收源端推送的红黑树 KV 数据并写入本地|	同步性能（数据接收速率、一致性）|
|30	|测试哈希表存储引擎同步指令（源端角色）	|同步操作：作为 “数据源”，向目标端推送哈希表 KV 数据（哈希表专属同步逻辑）|	同步性能（数据推送速率、完整性）|
|31	|测试哈希表存储引擎同步指令（目标端角色）	|同步操作：作为 “目标端”，接收源端推送的哈希表 KV 数据并写入本地	|同步性能（数据接收速率、一致性）|
|32	|测试跳表存储引擎同步指令（源端角色）	|同步操作：作为 “数据源”，向目标端推送跳表 KV 数据（跳表专属同步逻辑）	|同步性能（数据推送速率、完整性）|
|33	|测试跳表存储引擎同步指令（目标端角色）	|同步操作：作为 “目标端”，接收源端推送的跳表 KV 数据并写入本地|	同步性能（数据接收速率、一致性）|
|34	|测试数组存储引擎多线程场景下的基础性能	|多线程操作：多线程并发执行数组的 SET/GET/DEL 等基础指令（验证线程安全）	|并发性能（多线程 QPS、线程安全）|
|35	|测试红黑树存储引擎多线程场景下的基础性能	|多线程操作：多线程并发执行红黑树的 RSET/RGET/RDEL 等基础指令（验证线程安全）	|并发性能（多线程 QPS、线程安全）|
|36	|测试哈希表存储引擎多线程场景下的基础性能	|多线程操作：多线程并发执行哈希表的 HSET/HGET/HDEL 等基础指令（验证线程安全）|	并发性能（多线程 QPS、线程安全）|
|37	|测试跳表存储引擎多线程场景下的基础性能	|多线程操作：多线程并发执行跳表的 SSET/SGET/SDEL 等基础指令（验证线程安全）|	并发性能（多线程 QPS、线程安全）|
|38	|测试数组存储引擎多线程处理大量键值对的性能|	多线程批量操作：多线程并发执行数组的大量 SET 指令（高数据量 + 高并发）|	并发批量性能（多线程吞吐量）|
|39|	测试红黑树存储引擎多线程处理大量键值对的性能	|多线程批量操作：多线程并发执行红黑树的大量 RSET 指令（高数据量 + 高并发）	|并发批量性能（多线程吞吐量）|
|40	|测试哈希表存储引擎多线程处理大量键值对的性能|	多线程批量操作：多线程并发执行哈希表的大量 HSET 指令（高数据量 + 高并发）	|并发批量性能（多线程吞吐量）|
|41	|测试跳表存储引擎多线程处理大量键值对的性能|	多线程批量操作：多线程并发执行跳表的大量 SSET 指令（高数据量 + 高并发）|	并发批量性能（多线程吞吐量）|
|42	|测试数组存储引擎多线程数据落盘性能	|多线程持久化：多线程并发触发数组 KV 数据落盘（验证持久化线程安全）	|并发持久化性能（多线程 IO 速率）|
|43	|测试红黑树存储引擎多线程数据落盘性能	|多线程持久化：多线程并发触发红黑树 KV 数据落盘（验证持久化线程安全）|	并发持久化性能（多线程 IO 速率）|
|44	|测试哈希表存储引擎多线程数据落盘性能	|多线程持久化：多线程并发触发哈希表 KV 数据落盘（验证持久化线程安全）|	并发持久化性能（多线程 IO 速率）|
|45	|测试跳表存储引擎多线程数据落盘性能	|多线程持久化：多线程并发触发跳表 KV 数据落盘（验证持久化线程安全）|	并发持久化性能（多线程 IO 速率）|
|46	|测试数组存储引擎多线程数据读盘性能	|多线程恢复：多线程并发触发数组 KV 数据读盘加载（验证恢复线程安全）|	并发恢复性能（多线程加载速率）|
|47	|测试红黑树存储引擎多线程数据读盘性能	|多线程恢复：多线程并发触发红黑树 KV 数据读盘加载（验证恢复线程安全）	|并发恢复性能（多线程加载速率）|
|48	|测试哈希表存储引擎多线程数据读盘性能|	多线程恢复：多线程并发触发哈希表 KV 数据读盘加载（验证恢复线程安全）|	并发恢复性能（多线程加载速率）|
|49	|测试跳表存储引擎多线程数据读盘性能	|多线程恢复：多线程并发触发跳表 KV 数据读盘加载（验证恢复线程安全）|	并发恢复性能（多线程加载速率）|
|50	|测试数组存储引擎多线程合并指令性能	|多线程复合操作：多线程并发执行数组的合并指令（17 场景的多线程版）|	并发复合性能（多线程连续操作 QPS）|
|51	|测试红黑树存储引擎多线程合并指令性能	|多线程复合操作：多线程并发执行红黑树的合并指令（18 场景的多线程版）	|并发复合性能（多线程连续操作 QPS）|
|52|	测试哈希表存储引擎多线程合并指令性能	|多线程复合操作：多线程并发执行哈希表的合并指令（19 场景的多线程版）	|并发复合性能（多线程连续操作 QPS）|
|53|	测试跳表存储引擎多线程合并指令性能|	多线程复合操作：多线程并发执行跳表的合并指令（20 场景的多线程版）|	并发复合性能（多线程连续操作 QPS）|
|54|	测试数组存储引擎多线程范围查询性能|	多线程范围操作：多线程并发执行数组的范围查询指令（22 场景的多线程版）|	并发查询性能（多线程查询耗时、一致性）|
|55|	测试红黑树存储引擎多线程范围查询性能|	多线程范围操作：多线程并发执行红黑树的范围查询指令（23 场景的多线程版）|	并发查询性能（多线程查询耗时、一致性）|
|56|	测试哈希表存储引擎多线程范围查询性能|	多线程范围操作：多线程并发执行哈希表的范围查询指令（24 场景的多线程版）|	并发查询性能（多线程查询耗时、一致性）|
|57|	测试跳表存储引擎多线程范围查询性能|	多线程范围操作：多线程并发执行跳表的范围查询指令（25 场景的多线程版）|	并发查询性能（多线程查询耗时、一致性）|
|58|	测试数组存储引擎多线程同步指令（源端角色）	|多线程同步：多线程作为 “源端”，并发向目标端推送数组 KV 数据（26 场景的多线程版）|	并发同步性能（多线程推送速率）|
|59|	测试数组存储引擎多线程同步指令（目标端角色）|	多线程同步：多线程作为 “目标端”，并发接收源端推送的数组 KV 数据（27 场景的多线程版）	|并发同步性能（多线程接收速率）|
|60|	测试红黑树存储引擎多线程同步指令（源端角色）	|多线程同步：多线程作为 “源端”，并发向目标端推送红黑树 KV 数据（28 场景的多线程版）|	并发同步性能（多线程推送速率）|
|61|	测试红黑树存储引擎多线程同步指令（目标端角色）|	多线程同步：多线程作为 “目标端”，并发接收源端推送的红黑树 KV 数据|	并发同步性能（多线程接收速率）|
|62|	测试哈希表存储引擎多线程同步指令（源端角色）	|多线程同步：多线程作为 “源端”，并发向目标端推送哈希表 KV 数据（30 场景的多线程版）|	并发同步性能（多线程推送速率）|
|63|	测试哈希表存储引擎多线程同步指令（目标端角色）|	多线程同步：多线程作为 “目标端”，并发接收源端推送的哈希表 KV 数据（31 场景的多线程版）	|并发同步性能（多线程接收速率）|
|64|	测试跳表存储引擎多线程同步指令（源端角色）	|多线程同步：多线程作为 “源端”，并发向目标端推送跳表 KV 数据（32 场景的多线程版）	|并发同步性能（多线程推送速率）|
|65|	测试跳表存储引擎多线程同步指令（目标端角色）|	多线程同步：多线程作为 “目标端”，并发接收源端推送的跳表 KV 数据（33 场景的多线程版）	|并发同步性能（多线程接收速率）|



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

------------------------------------

> 单线程测试范围查询,先插入KEY：Name_0000001001 ~ Name_0000002001，再查询KEY:Name_0000001500 ~ Name_0000001900
<img width="550" height="185" alt="image" src="https://github.com/user-attachments/assets/c8c24756-7a1d-414c-9c67-ae23489f6530" />

-------------------------------------

> 单线程测试同步命令，先向主服务器插入10000个KEY，从服务器向主服务器发送SYNC命令并GET这10000个KEY
<img width="510" height="179" alt="image" src="https://github.com/user-attachments/assets/458b1677-9ab7-45c7-80aa-d23c16255de3" />

-------------------------------------

> 开启4个线程，每个线程插入10000个不同的KEY（根据线程id * 线程操作次数编号）并分别执行SAVE指令，随后重启KV存储服务端，每个线程再执行LOAD指令进行读盘
<img width="610" height="363" alt="image" src="https://github.com/user-attachments/assets/c87dcb59-3a08-495d-802a-ec540866c746" />
<img width="550" height="368" alt="image" src="https://github.com/user-attachments/assets/e33046df-edcb-4957-959b-3b9a076804da" />

-------------------------------------

> 开启5个线程，将SET,GET,SET,MOD,EXIST,GET,DEL,GET,MOD,EXIST指令合并为1条指令发送，每个线程重复20000次，共计执行100W条指令
<img width="745" height="485" alt="image" src="https://github.com/user-attachments/assets/7fce7ad7-8aae-45a3-82a5-cfe3c32c137f" />

--------------------------------------

> 开启4个线程，每个线程先插入KEY：thread_id * 100 ~ thread_id * 1100, 在查询KEY：thread_id * 500 ~ thread_id * 900
<img width="704" height="576" alt="image" src="https://github.com/user-attachments/assets/fe57647d-e50e-47f5-9f2a-5fe7ce000145" />

----------------------------------

> 先开启4个线程，每个线程分别向主服务器插入10000个KEY，从服务器再开启4个线程，分别向主服务器发送SYNC命令并且分别GET对应的10000个KEY，KEY按照i + (ctx->thread_id + 1) * ctx->operations_per_thread进行区分
<img width="785" height="618" alt="image" src="https://github.com/user-attachments/assets/380eba39-2865-49d7-9757-a234d42e3075" />

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

> 单线程测试范围查询,先插入KEY：Name_0000001001 ~ Name_0000002001，再查询KEY:Name_0000001500 ~ Name_0000001900
<img width="585" height="190" alt="image" src="https://github.com/user-attachments/assets/468b3e62-14f4-4598-9eb5-3c9ba3e80533" />

-------------------------------------

> 单线程测试同步命令，先向主服务器插入10000个KEY，从服务器向主服务器发送SYNC命令并GET这10000个KEY
<img width="528" height="186" alt="image" src="https://github.com/user-attachments/assets/864f470f-d0d3-46a6-97df-fa49774b7c18" />

----------------------------------------------------------------

> 开启4个线程，每个线程插入10000个不同的KEY（根据线程id * 线程操作次数编号）并分别执行RSAVE指令，随后重启KV存储服务端，每个线程再执行RLOAD指令进行读盘
<img width="639" height="460" alt="image" src="https://github.com/user-attachments/assets/927d6c3a-b273-4af9-a9b6-cc8e2ee91c85" />
<img width="714" height="460" alt="image" src="https://github.com/user-attachments/assets/d94ac7d4-ecce-48e0-8aa4-eabb207f6e9a" />

----------------------------------------------------------------

> 开启5个线程，将RSET,RGET,RSET,RMOD,REXIST,RGET,RDEL,RGET,RMOD,REXIST指令合并为1条指令发送，每个线程重复20000次，共计执行100W条指令
<img width="810" height="480" alt="image" src="https://github.com/user-attachments/assets/1057cf75-2ec8-466a-b9e8-65a7faf2cbde" />

--------------------------

> 开启4个线程，每个线程先插入KEY：thread_id * 100 ~ thread_id * 1100, 在查询KEY：thread_id * 500 ~ thread_id * 900
<img width="720" height="570" alt="image" src="https://github.com/user-attachments/assets/30e7b8c0-f429-49c7-9e35-4c77549f4231" />

----------------------------------

> 先开启4个线程，每个线程分别向主服务器插入10000个KEY，从服务器再开启4个线程，分别向主服务器发送SYNC命令并且分别GET对应的10000个KEY，KEY按照i + (ctx->thread_id + 1) * ctx->operations_per_thread进行区分
<img width="676" height="614" alt="image" src="https://github.com/user-attachments/assets/14f220ad-b38c-4408-a0a9-c2ddc45773ee" />

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

-------------------------------------------------------------------

> 单线程测试范围查询,先插入KEY：Name_0000001001 ~ Name_0000002001，再查询KEY:Name_0000001500 ~ Name_0000001900
<img width="610" height="180" alt="image" src="https://github.com/user-attachments/assets/9a928a13-ed1b-45ad-b358-b7a57ec0af6b" />

-------------------------------------

> 单线程测试同步命令，先向主服务器插入10000个KEY，从服务器向主服务器发送SYNC命令并GET这10000个KEY
<img width="568" height="181" alt="image" src="https://github.com/user-attachments/assets/f5cae26e-43cb-4526-bc93-b7163e32e04d" />

-------------------------------------------------------------------

> 开启4个线程，每个线程插入10000个不同的KEY（根据线程id * 线程操作次数编号）并分别执行HSAVE指令，随后重启KV存储服务端，每个线程再执行HLOAD指令进行读盘
<img width="716" height="465" alt="image" src="https://github.com/user-attachments/assets/6fd1ae77-4c4c-456c-8f1a-f79a0742d044" />
<img width="709" height="475" alt="image" src="https://github.com/user-attachments/assets/1c29d126-ffd8-4bfa-99f2-fbf1a532cb15" />

-----------------------------------------------------------------------

> 开启5个线程，将HSET,HGET,HSET,HMOD,HEXIST,HGET,HDEL,HGET,HMOD,HEXIST指令合并为1条指令发送，每个线程重复20000次，共计执行100W条指令
<img width="825" height="479" alt="image" src="https://github.com/user-attachments/assets/f9c68077-7e77-4dac-a7f9-90aa1fe596e3" />

-------------------------------------------

> 开启4个线程，每个线程先插入KEY：thread_id * 100 ~ thread_id * 1100, 在查询KEY：thread_id * 500 ~ thread_id * 900
<img width="699" height="576" alt="image" src="https://github.com/user-attachments/assets/d90394eb-9ad9-40b7-8a8c-6e42d5dfd0ed" />

----------------------------------

> 先开启4个线程，每个线程分别向主服务器插入10000个KEY，从服务器再开启4个线程，分别向主服务器发送SYNC命令并且分别GET对应的10000个KEY，KEY按照i + (ctx->thread_id + 1) * ctx->operations_per_thread进行区分
<img width="755" height="621" alt="image" src="https://github.com/user-attachments/assets/b1eeaef6-6016-450f-9a73-910f8bc41e90" />

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

-----------------------------------------------------------------

> 单线程测试范围查询,先插入KEY：Name_0000001001 ~ Name_0000002001，再查询KEY:Name_0000001500 ~ Name_0000001900
<img width="750" height="190" alt="image" src="https://github.com/user-attachments/assets/50029a40-d3fa-429b-bd4e-de100effdb52" />

-------------------------------------

> 单线程测试同步命令，先向主服务器插入10000个KEY，从服务器向主服务器发送SYNC命令并GET这10000个KEY
<img width="565" height="183" alt="image" src="https://github.com/user-attachments/assets/da1f61a9-9b73-4804-964f-564640c18768" />

-------------------------------------------------------------------

> 开启4个线程，每个线程插入10000个不同的KEY（根据线程id * 线程操作次数编号）并分别执行SSAVE指令，随后重启KV存储服务端，每个线程再执行SLOAD指令进行读盘
<img width="745" height="461" alt="image" src="https://github.com/user-attachments/assets/021872e3-522c-4c8c-b137-58b4a1bc72ef" />
<img width="811" height="465" alt="image" src="https://github.com/user-attachments/assets/c972c269-a338-462a-b937-1501603fbb2a" />

-----------------------------------------------------------------------

> 开启5个线程，将SSET,SGET,SSET,SMOD,SEXIST,SGET,SDEL,SGET,SMOD,SEXIST指令合并为1条指令发送，每个线程重复20000次，共计执行100W条指令
<img width="970" height="489" alt="image" src="https://github.com/user-attachments/assets/85ef43ef-154c-4698-a40b-5e9fff282fc5" />

-------------------------------------------------------------------

> 开启4个线程，每个线程先插入KEY：thread_id * 100 ~ thread_id * 1100, 在查询KEY：thread_id * 500 ~ thread_id * 900
<img width="841" height="570" alt="image" src="https://github.com/user-attachments/assets/e6de28c5-59cd-4946-86e4-63610cadd252" />
----------------------------------

> 先开启4个线程，每个线程分别向主服务器插入10000个KEY，从服务器再开启4个线程，分别向主服务器发送SYNC命令并且分别GET对应的10000个KEY，KEY按照i + (ctx->thread_id + 1) * ctx->operations_per_thread进行区分
<img width="1001" height="639" alt="image" src="https://github.com/user-attachments/assets/b332b979-7676-47e1-bfab-4daf0eead6a1" />

--------------------------------------------------------------------


















