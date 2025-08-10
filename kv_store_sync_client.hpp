#ifndef __KV_STORE_SYNC_CLIENT_HPP__
#define __KV_STORE_SYNC_CLIENT_HPP__

#include "kv_log.h"
#include "kv_store.hpp"
#include "kv_save.h"
#include <arpa/inet.h>
#include <netinet/tcp.h>

#ifdef __cplusplus

#include <chrono>

class SyncData
{
public:
	SyncData(const std::string& k, const std::string& v, uint64_t t) : key(k), value(v), timestamp(t)
	{

	}

	SyncData(char* k, char* v, uint64_t t) : key(k), value(v), timestamp(t)
	{

	}
	std::string key;
	std::string value;
	uint64_t timestamp;
};

class SyncClient
{
public:
	SyncClient(const std::string & ip, int port) : ip_(ip), port_(port), sockfd_(-1)
	{

	}

	~SyncClient()
	{
		if (sockfd_ != -1) {
			close(sockfd_);
		}
	}

	int connect()
	{
		sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd_ < 0) {
			KV_LOG("create SyncClient sockfd_ failed\n");
			return -1;
		}

		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(struct sockaddr_in));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port_);

		if (inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
			KV_LOG("invalid server ip: %s\n", ip_.c_str());
			close(sockfd_);
			sockfd_ = -1;
			return -2;
		}

		/*调用全局connect函数*/
		if (::connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
			KV_LOG("connect to server %s:%d failed\n", ip_.c_str(), port_);
			close(sockfd_);
			sockfd_ = -1;
			return -3;
		}

		int flags = fcntl(sockfd_, F_GETFL, 0);
		fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);

		KV_LOG("connect to sync server %s : %d success\n", ip_.c_str(), port_);
		return 0;

	}

	int send_command(const std::string& cmd) 
	{
		if (sockfd_ < 0) {
			if (connect() < 0) {
				KV_LOG("invalid sockfd and trying to connect failed, send command failed\n");
				return -1;
			}
		}

		ssize_t n = send(sockfd_, cmd.c_str(), cmd.size(), 0);
		if (n < 0) {
			KV_LOG("send command: %s failed, %s\n", cmd.c_str(), strerror(errno));
			close(sockfd_);
			sockfd_ = -1;
			return -2;
		}

		KV_LOG("Sync Client send command: %s success\n", cmd.c_str());
		return 0;
	}

	std::string receive_response()
	{
		if (sockfd_ < 0) {
			KV_LOG("invalid sockfd, receive response failed\n");
			return "";
		}

		char buffer[4096] = { 0 };
		std::string response;

		while (true) {
			memset(buffer, 0, sizeof(buffer));
			ssize_t n = recv(sockfd_, buffer, sizeof(buffer), 0);
			KV_LOG("recv: %s, %ld bytes\n", buffer, n);
			if (n > 0) {
				KV_LOG("recv: %s, %ld bytes\n",buffer, n);
				// buffer[n] = '\0';
				response += buffer;

				if (response.find('\n') != std::string::npos) {
					KV_LOG("get response : %s\n", response.c_str());
					break;
				}
			}
			else if (n == 0) {
				KV_LOG("server closed\n");
				close(sockfd_);
				sockfd_ = -1;
				break;
			}
			else {
				if (errno != EAGAIN && errno != EWOULDBLOCK) {
					KV_LOG("receive response failed: %s\n", strerror(errno));
					close(sockfd_);
					sockfd_ = -1;
					break;
				}
				/*无数据可读，跳出循环*/
				//break;
			}
		}
		KV_LOG("receive response: %s\n", response.c_str());
		return response;
	}

	int get_socket() const
	{
		return sockfd_;
	}

	const std::string& get_ip() const
	{
		return ip_;
	}

	int get_port() const
	{
		return port_;
	}

	std::vector<SyncData>
	parse_remote_data_str(const std::string& data_str)
	{
		std::vector<SyncData> result;
		std::stringstream ss(data_str);
		std::string line;
		while (std::getline(ss, line, '\n')) {
			if (line.empty()) {
			    continue;
			}
			std::string key, value;
			uint64_t timestamp;
			std::stringstream line_ss(line);
			line_ss >> key >> value >> timestamp;
			// sscanf(line.c_str(), "%s %s %lu", key.c_str(), value.c_str(), &timestamp);
			KV_LOG("parse remote data str key: %s, value: %s, timestamp: %lu\n", key.c_str(), value.c_str(), timestamp);
			result.emplace_back(key, value, timestamp);
		}
		return result;
	}

	int sync_data_to_local_array_engine(const std::vector<SyncData>& data)
	{
		int ret = 0;
		for (const auto& item : data) {
			bool key_exists = false;
			uint64_t local_timestamp = 0;
			#if ENABLE_ARRAY_KV_ENGINE
			char* local_value = kvs_array_get(&global_array, item.key.c_str());
			if (local_value != NULL) {
				key_exists = true;
				local_timestamp = kvs_array_get_timestamp(&global_array, item.key.c_str());
			}
			#endif
			
			if (key_exists) {
				if (item.timestamp > local_timestamp) {
					/*远程数据更新*/
					#if ENABLE_ARRAY_KV_ENGINE
					ret = kvs_array_modify_with_timestamp(&global_array, item.key.c_str(), item.value.c_str(), item.timestamp);
					if (ret != 0) {
						KV_LOG("kvs_array_modify failed\n");
						return ret;
					}
					#endif
				}
			}
			else {
				/*key 不存在*/
				#if ENABLE_ARRAY_KV_ENGINE
				ret = kvs_array_set_with_timestamp(&global_array, item.key.c_str(), item.value.c_str(), item.timestamp);
				if (ret != 0) {
					KV_LOG("kvs_array_set failed\n");
					return ret;
				}
				#endif
			}
			KV_LOG("sync data: key: %s, value: %s, timestamp: %lu\n", 
				item.key.c_str(), item.value.c_str(), item.timestamp);
		}
		return ret;
	}

	int sync_data_to_local_rbtree_engine(const std::vector<SyncData>& data)
	{
		#if ENABLE_RBTREE_KV_ENGINE
		int ret = 0;
		for (const auto& item : data) {
			bool key_exists = false;
			uint64_t local_timestamp = 0;
			char* local_value = kvs_rbtree_get(&global_rbtree, item.key.c_str());
			if (local_value != NULL) {
				key_exists = true;
				local_timestamp = kvs_rbtree_get_timestamp(&global_rbtree, item.key.c_str());
			}

			if (key_exists) {
				if (item.timestamp > local_timestamp) {
					/*远程数据更新*/
					ret = kvs_rbtree_modify_with_timestamp(&global_rbtree, item.key.c_str(), item.value.c_str(), item.timestamp);
					if (ret != 0) {
						KV_LOG("kvs_rbtree_modify failed\n");
						return ret;
					}
				}
			}
			else {
				/*key 不存在*/
				ret = kvs_rbtree_set_with_timestamp(&global_rbtree, item.key.c_str(), item.value.c_str(), item.timestamp);
				if (ret != 0) {
					KV_LOG("kvs_rbtree_set failed\n");
					return ret;
				}
			}
			KV_LOG("sync data: key: %s, value: %s, timestamp: %lu\n", 
				item.key.c_str(), item.value.c_str(), item.timestamp);
		}
		#endif
		return ret;
	}

	int sync_data_to_local_hash_engine(const std::vector<SyncData>& data)
	{
		#if ENABLE_HASH_KV_ENGINE
		int ret = 0;
		for (const auto& item : data) {
			bool key_exists = false;
			uint64_t local_timestamp = 0;
			char* local_value = kvs_hash_get(&global_hash, item.key.c_str());
			if (local_value != NULL) {
				key_exists = true;
				local_timestamp = kvs_hash_get_timestamp(&global_hash, item.key.c_str());
			}

			if (key_exists) {
				if (item.timestamp > local_timestamp) {
					/*远程数据更新*/
					ret = kvs_hash_modify_with_timestamp(&global_hash, item.key.c_str(), item.value.c_str(), item.timestamp);
					if (ret != 0) {
						KV_LOG("kvs_hash_modify failed\n");
						return ret;
					}
				}
			}
			else {
				/*key 不存在*/
				ret = kvs_hash_set_with_timestamp(&global_hash, item.key.c_str(), item.value.c_str(), item.timestamp);
				if (ret != 0) {
					KV_LOG("kvs_hash_set failed\n");
					return ret;
				}
			}
			KV_LOG("sync data: key: %s, value: %s, timestamp: %lu\n",
				item.key.c_str(), item.value.c_str(), item.timestamp);
		}
		#endif
		return ret;
	}

	int sync_data_to_local_skiptable_engine(const std::vector<SyncData>& data)
	{
		#if ENABLE_SKIPTABLE_KV_ENGINE
		int ret = 0;
		for (const auto& item : data) {
			bool key_exists = false;
			uint64_t local_timestamp = 0;
			char* local_value = kvs_skiptable_get(&global_skiptable, item.key.c_str());
			if (local_value != NULL) {
				key_exists = true;
				local_timestamp = kvs_skiptable_get_timestamp(&global_skiptable, item.key.c_str());
			}

			if (key_exists) {
			    if (item.timestamp > local_timestamp) {
					/*远程数据更新*/
					ret = kvs_skiptable_modify_with_timestamp(&global_skiptable, item.key.c_str(), item.value.c_str(), item.timestamp);
					if (ret != 0) {
						KV_LOG("kvs_skiptable_modify failed\n");
						return ret;
					}
				}
			}
			else {
				/*key 不存在*/
				ret = kvs_skiptable_set_with_timestamp(&global_skiptable, item.key.c_str(), item.value.c_str(), item.timestamp);
				if (ret != 0) {
					KV_LOG("kvs_skiptable_set failed\n");
					return ret;
				}
			}
			KV_LOG("sync data: key: %s, value: %s, timestamp: %lu\n",
				item.key.c_str(), item.value.c_str(), item.timestamp);
		}
		#endif
		return ret;
	}


	int sync_data_to_all_local_engine(const std::vector<SyncData>& data)
	{
		int ret = 0;
		for (const auto& item : data) {
			bool key_exists = false;
			uint64_t local_timestamp = 0;
			#if ENABLE_ARRAY_KV_ENGINE
			char* local_value = kvs_array_get(&global_array, item.key.c_str());
			if (local_value != NULL) {
				key_exists = true;
				local_timestamp = kvs_array_get_timestamp(&global_array, item.key.c_str());
			}
			#endif
			
			if (key_exists) {
				if (item.timestamp > local_timestamp) {
					/*远程数据更新*/
					#if ENABLE_ARRAY_KV_ENGINE
					ret = kvs_array_modify_with_timestamp(&global_array, item.key.c_str(), item.value.c_str(), item.timestamp);
					if (ret != 0) {
						KV_LOG("kvs_array_modify failed\n");
						return ret;
					}
					#endif
				}
			}
			else {
				/*key 不存在*/
				#if ENABLE_ARRAY_KV_ENGINE
				ret = kvs_array_set_with_timestamp(&global_array, item.key.c_str(), item.value.c_str(), item.timestamp);
				if (ret != 0) {
					KV_LOG("kvs_array_set failed\n");
					return ret;
				}
				#endif
			}
			KV_LOG("sync data: key: %s, value: %s, timestamp: %lu\n", 
				item.key.c_str(), item.value.c_str(), item.timestamp);
		}
		return ret;
	}

private:
	std::string ip_;
	int port_;
	int sockfd_;
};

#endif

#endif
