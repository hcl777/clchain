#pragma once
#include <string>
#include "cltype.h"
#include "incnet.h"

CLBEGIN
namespace net {
	int init();
	void fini();


	bool is_private_ip(unsigned long hip);
	bool is_private_ip(const std::string& ip);
	std::string get_local_private_ip();
	std::string ip_explain(const std::string& s);

	void sockaddr_format(sockaddr_in& addr, unsigned int nip, unsigned short nport);
	void sockaddr_format(sockaddr_in& addr, const char* ip, unsigned short port);

	int open_udp(unsigned short port, unsigned int ip, int rcvbuf, int sndbuf);
	int set_nonblock(int fd, int nonblock = 1); //inoblock=1 ±íÊ¾·Ç×èÈû
	int sock_set_timeout(int fd, int ms);

	std::string ntoas(unsigned int nip);
	std::string htoas(unsigned int ip);
	std::string htoas(unsigned int ip, unsigned short port);
	unsigned int atoh(const std::string& ip);
	unsigned int aton(const std::string& ip);

	int sock_set_udp_broadcast(int fd);
	int sock_set_udp_multicast(int fd, const char* multi_ip);
	int sock_bind(int fd, unsigned short port);
	int sock_select_readable(int fd, uint32_t dwTimeout = 100);
	int sock_select_writable(int fd, uint32_t dwTimeout = 100);
	int sock_select_send_n(int fd, const char *buf, int size, int timeoutms);
	int sock_select_recv_n(int fd, char *buf, int size, int timeoutms);
	int sock_send_n(int fd, const char *buf, int size);
	int sock_recv_n(int fd, char *buf, int size);
	int sock_get_myaddr(int fd, unsigned int& nip, unsigned short& nport);
}
CLEND

