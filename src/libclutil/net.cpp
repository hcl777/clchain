#include "net.h"
#include "incnet.h"
#include "bstream.h"
#include <string.h>
#include <iostream>
#include <thread>

#ifdef __GNUC__
#include <signal.h>
#endif

CLBEGIN
namespace net{
	int init()
	{
#ifdef _MSC_VER
		WSADATA wsaData;
		if (0 != WSAStartup(0x202, &wsaData))
		{
			perror("WSAStartup false! : ");
			return -1;
		}
#else
		signal(SIGPIPE, SIG_IGN); //忽略Broken pipe,否则socket对端关闭时，很容易写会出现Broken pipe（管道破裂）
#endif
		return 0;
	}
	void fini()
	{
#ifdef _MSC_VER
		WSACleanup();
#endif
	}

	bool is_private_ip(unsigned long hip)
	{
		return ((hip & 0xff000000) == 0x0a000000 || // 10.0.0.0/8
			(hip & 0xff000000) == 0x7f000000 || // 127.0.0.0/8
			(hip & 0xff000000) == 0xa9000000 || // 169.0.0.0/8
			(hip & 0xfff00000) == 0xac100000 || // 172.16.0.0/12
			(hip & 0xffff0000) == 0xc0a80000);  // 192.168.0.0/16
	}
	bool is_private_ip(const std::string& ip)
	{
		struct in_addr s;
		inet_pton(AF_INET,ip.c_str(),&s);
		return is_private_ip( ntohl(s.s_addr));
		
	}
	std::string get_local_private_ip()
	{
		u_long ip = 0, ip2 = 0;
		struct addrinfo hints;
		struct addrinfo *res, *cur;
		char name[256] = { 0 };
		gethostname(name, 256);

		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;     /* Allow IPv4 */
		hints.ai_flags = AI_PASSIVE;/* For wildcard IP address */
		hints.ai_protocol = 0;         /* Any protocol */
		hints.ai_socktype = SOCK_STREAM;

		if (0 == getaddrinfo(name, NULL, &hints, &res))
		{
			for (cur = res; cur != NULL; cur = cur->ai_next) 
			{
				ip2 = ntohl(((struct sockaddr_in *)cur->ai_addr)->sin_addr.s_addr);
				if (is_private_ip(ip2) && ip2 > ip)
					ip = ip2;
			}
			freeaddrinfo(res);
		}
		return htoas((unsigned int)ip);
	}
	std::string ip_explain(const std::string& s)
	{
		std::string ip;
		struct addrinfo *res;
		struct in_addr ia;
		if (s.empty()) return s;
		if (1 == inet_pton(AF_INET, s.c_str(), &ia))
		{
			if (INADDR_NONE != ia.s_addr)
				return s;
		}
		
		if (0 == getaddrinfo(s.c_str(), NULL, NULL, &res))
		{
			char sip[128];
			memset(sip, 0, 128);
			ip = inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, sip, 128);
			freeaddrinfo(res);
		}
		return ip;
	}
	void sockaddr_format(sockaddr_in& addr, unsigned int nip, unsigned short nport)
	{
		memset(&addr, 0, sizeof(sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_port = nport;
		addr.sin_addr.s_addr = nip;
	}
	void sockaddr_format(sockaddr_in& addr, const char* ip, unsigned short port)
	{
		//sockaddr_format(addr, ip_aton_try_explain_ex(ip), htons(port));
	}
	int open_udp(unsigned short port, unsigned int ip, int rcvbuf, int sndbuf)
	{
		sockaddr_in addr;
		int fd = (int)INVALID_SOCKET;
		fd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (INVALID_SOCKET == fd)
			return fd;
		//设置接收和发送缓冲
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvbuf, sizeof(int));
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&sndbuf, sizeof(int));

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(ip); //INADDR_ANY

		if (SOCKET_ERROR == ::bind(fd, (sockaddr *)&addr, sizeof(addr)))
		{
			::closesocket(fd);
			fd = (int)INVALID_SOCKET;
		}
		printf("udp bind(%s)\n", htoas(ip,port).c_str());
		return fd;
	}

	int set_nonblock(int fd, int nonblock/*=1*/)
	{
#ifdef _MSC_VER
		//NONBLOCKING=1
		u_long val = nonblock;
		if (INVALID_SOCKET != fd)
			return ioctlsocket(fd, FIONBIO, &val);
		return -1;
#elif defined(_ECOS_8203)
		int val = nonblock;
		return ioctl(fd, FIONBIO, &val);
#else
		int opts;
		opts = fcntl(fd, F_GETFL);
		if (-1 == opts)
		{
			return -1;
		}
		if (nonblock)
			opts |= O_NONBLOCK;
		else
			opts &= ~O_NONBLOCK;
		if (-1 == fcntl(fd, F_SETFL, opts))
		{
			return -1;
		}
		return 0;
#endif

	}
	int sock_set_timeout(int fd, int ms)
	{
		int ret;
#ifdef _WIN32
		int x = ms;
#else
		struct timeval x;
		x.tv_sec = ms / 1000;
		x.tv_usec = (ms % 1000) * 1000;
#endif
		if (0 != (ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&x, sizeof(x))))
		{
			perror("setsockopt SO_RCVTIMEO");
		}
		if (0 != (ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&x, sizeof(x))))
		{
			perror("setsockopt SO_SNDTIMEO");
		}
		return ret;
	}


	std::string ntoas(unsigned int nip)
	{
		char buf[32];
		unsigned char *ip_n = (unsigned char*)&nip;
		snprintf(buf,32, "%d.%d.%d.%d", (int)ip_n[0], (int)ip_n[1], (int)ip_n[2], (int)ip_n[3]);
		return buf;
	}
	std::string htoas(unsigned int ip)
	{
		return ntoas(bstream::htob32(ip));
	}
	std::string htoas(unsigned int ip, unsigned short port)
	{
		char buf[64];
		snprintf(buf,64, "%s:%d", htoas(ip).c_str(), port);
		return buf;
	}
	unsigned int atoh(const std::string& ip)
	{
		//atonl:inet_addr("")=0,inet_addr(NULL)=-1,inet_addr("asdfs.s322.dassadf")=-1
		//sscanf返回：EOF=-1为错误，其它表示成功输入参数的个数,失败返回0或-1
		unsigned int iip;
		unsigned int ip_n[4] = { 0,0,0,0 };
		if (4 != sscanf_s(ip.c_str(), "%d.%d.%d.%d", &ip_n[0], &ip_n[1], &ip_n[2], &ip_n[3]))
			return 0;
		//最前的IP放到大值
		iip = (ip_n[0] << 24) + (ip_n[1] << 16) + (ip_n[2] << 8) + ip_n[3];
		return iip;
	}
	unsigned int aton(const std::string& ip)
	{
		return bstream::htob32(atoh(ip));
	}

	int sock_set_udp_broadcast(int fd)
	{
#ifdef _WIN32
		bool isbroadcast = true;
#else
		int isbroadcast = 1;
#endif
		int ret = 0;
		if (0 != (ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char*)&isbroadcast, sizeof(isbroadcast))))
		{
			perror("*** setsockopt(SO_BROADCAST): ");
		}
		return ret;
	}
	int sock_set_udp_multicast(int fd, const char* multi_ip)
	{
		struct ip_mreq mreq;
		if (inet_pton(AF_INET, multi_ip, &mreq.imr_multiaddr) <= 0) //成功返回1，AF_INET6
			mreq.imr_multiaddr.s_addr = INADDR_ANY;
		mreq.imr_interface.s_addr = INADDR_ANY;//htonl(INADDR_ANY);    
		if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq)) < 0)
		{
			perror("setsockopt");
			return -1;
		}
		return 0;

	}
	int sock_bind(int fd, unsigned short port)
	{
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;
		return ::bind(fd, (sockaddr *)&addr, sizeof(addr));
	}
	int sock_select_readable(int fd, uint32_t dwTimeout/* = 100*/)
	{
		//assert(fd != INVALID_SOCKET);

		timeval timeout;
		timeout.tv_sec = dwTimeout / 1000;
		timeout.tv_usec = dwTimeout % 1000;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		int n = ::select(fd + 1, &fds, NULL, NULL, &timeout);
		if (n>0)
		{
			if (FD_ISSET(fd, &fds))
				return 1;
			return 0;
		}
		return -1;
	}
	int sock_select_writable(int fd, uint32_t dwTimeout/* = 100*/)
	{
		//assert(fd != INVALID_SOCKET);

		timeval timeout;
		timeout.tv_sec = dwTimeout / 1000;
		timeout.tv_usec = dwTimeout % 1000;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		int nStatus = ::select(fd + 1, NULL, &fds, NULL, &timeout);
		if (nStatus != SOCKET_ERROR)
		{
			if (FD_ISSET(fd, &fds))
				return 1;
			return 0;
		}
		return -1;
	}
	int sock_select_send_n(int fd, const char *buf, int size, int timeoutms)
	{
		int pos = 0;
		int ret = 0;
		uint64_t begintick = mtick();
		while (pos<size)
		{

			if (1 == sock_select_writable(fd))
			{
				ret = send(fd, buf + pos, size - pos, 0);
				if (ret>0)
				{
					pos += ret;
				}
				else
				{
					return -1;
				}
			}
			//超时 10秒
			if (time_after(mtick(), (begintick + timeoutms)))
				break;
		}
		return pos == size ? 0 : -1;
	}
	int sock_select_recv_n(int fd, char *buf, int size, int timeoutms)
	{
		int pos = 0;
		int ret = 0;
		uint64_t begintick = mtick();
		while (pos<size)
		{
			if (1 == sock_select_readable(fd))
			{
				ret = recv(fd, buf + pos, size - pos, 0);
				if (ret>0)
				{
					pos += ret;
				}
				else
				{
					return -1;
				}
			}
			//超时 10秒
			if (time_after(mtick(), (begintick + timeoutms)))
				break;
		}
		return pos == size ? 0 : -1;
	}

	int sock_send_n(int fd, const char *buf, int size)
	{
		int pos = 0;
		int ret = 0;
		while (pos<size)
		{
			ret = send(fd, buf + pos, size - pos, 0);
			if (ret>0)
			{
				pos += ret;
			}
			else
			{
				//int err = WSAGetLastError();
				return -1;
			}
		}
		return 0;
	}

	int sock_recv_n(int fd, char *buf, int size)
	{
		int pos = 0;
		int ret = 0;
		while (pos<size)
		{
			ret = recv(fd, buf + pos, size - pos, 0);
			if (ret>0)
			{
				pos += ret;
			}
			else
			{
				return -1;
			}
		}
		return 0;
	}
	int sock_get_myaddr(int fd, unsigned int& nip, unsigned short& nport)
	{
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		socklen_t len = sizeof(addr);

		//只有connect或者bind的才可以获取成功
		if (0 != getsockname(fd, (sockaddr*)&addr, &len))
			return -1;
		nip = addr.sin_addr.s_addr;
		nport = addr.sin_port;
		return 0;
	}
}
CLEND

