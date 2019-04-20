#pragma once
//#if !defined(ANDROID)
////FD_SETSIZE 决定 fd_set 的数组大小，即最大select数目，windows在winsock2.h中定义
//#ifdef FD_SETSIZE
//	#undef FD_SETSIZE
//#endif
//#define FD_SETSIZE 1024    
//#endif

#ifdef _MSC_VER
#include <winsock2.h>
#include <WS2tcpip.h>
typedef int socklen_t;
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#ifndef NO_EPOLL
#include <sys/epoll.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <net/if_arp.h>  
#include <net/if.h>
#include <sys/ioctl.h> 
#include <errno.h>
//#include <dirent.h>

//******************************
//定义与win 相同的宏
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)

#endif
#include "bstream.h"
namespace cl
{
	enum { DISCONNECTED = 0, CONNECTING = 1, CONNECTED = 2 };
	typedef struct tag_endpoint {
		uint32_t	ip;
		uint16_t	port;
		tag_endpoint() :ip(0), port(0) {}
		tag_endpoint(uint32_t _ip, uint16_t _port) :ip(_ip), port(_port) {}
		void set(uint32_t _ip, uint16_t _port) { ip = _ip; port = _port; }
	}endpoint_t;

	inline int operator << (bstream& ss, const endpoint_t& i)
	{
		ss << i.ip;
		ss << i.port;
		return ss.ok();
	}
	inline int operator >> (bstream& ss, endpoint_t& i)
	{
		ss >> i.ip;
		ss >> i.port;
		return ss.ok();
	}
}

