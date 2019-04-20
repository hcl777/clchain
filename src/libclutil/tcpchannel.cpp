#include "tcpchannel.h"

namespace cl
{

	tcpchannel::tcpchannel(reactor* rt)
		: m_fd(INVALID_SOCKET)
		, m_rt(rt)
		, m_smore(false)
		, m_is_regwrite(false)
		, m_state(DISCONNECTED)
		//, m_hip(0)
		//, m_hport(0)
	{
		m_last_active_tick = cl::mtick();
	}


	tcpchannel::~tcpchannel()
	{
	}

	void tcpchannel::reset()
	{
		for (SendIter it = m_slist.begin(); it != m_slist.end(); ++it)
		{
			(*it)->release();
		}
		m_slist.clear();
		m_fd = INVALID_SOCKET;
		m_state = DISCONNECTED;
		m_is_regwrite = false;
		m_smore = false;
		//m_hip = 0;
		//m_hport = 0;
		m_ep.set(0, 0);
		m_last_active_tick = cl::mtick();
	}

	void tcpchannel::close_socket()
	{
		if (INVALID_SOCKET != m_fd)
		{
			closesocket(m_fd);
			m_fd = INVALID_SOCKET;
		}
	}
	int tcpchannel::on_connected()
	{
		m_state = CONNECTED;
		m_rt->register_handler(this, SE_READ);
		call(tcpchannelListener::Connected(), this);
		return 0;
	}

	int tcpchannel::attach(SOCKET s, sockaddr_in& addr)
	{
		//如果失败，此处不关闭s
		assert(INVALID_SOCKET == m_fd);

		m_fd = s;
		m_ep.set(ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
		//m_hip = ntohl(addr.sin_addr.s_addr);
		//m_hport = ntohs(addr.sin_port);
		on_connected();
		return 0;
	}
	int tcpchannel::connect(unsigned int ip, unsigned short port)
	{
		//参数为host ip，host port
		if (DISCONNECTED != m_state)
		{
			assert(0);
			return -1;
		}
		//DEBUGMSG("tcp_connect(%s)...\n", net::htoas(ip, port).c_str());
		m_last_active_tick = cl::mtick();
		m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_fd)
		{
			//DEBUGMSG("#***socket() failed! \n");
			return -1;
		}
		if (0 != m_rt->register_handler(this, SE_WRITE))
		{
			//DEBUGMSG("#***IOReactorSngl::instance()->register_handler() failed! \n");
			close_socket();
			return -1;
		}

		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(ip);
		if (SOCKET_ERROR == ::connect(m_fd, (sockaddr*)&addr, sizeof(addr)))
		{
#ifdef _MSC_VER
			int err = WSAGetLastError();
			if (WSAEWOULDBLOCK != err)
#else
			if (EINPROGRESS != errno)
#endif
			{
				//DEBUGMSG("#***connect() failed! \n");
				m_rt->unregister_handler(this, SE_WRITE);
				close_socket();
				return -1;
			}
			m_state = CONNECTING;
		}
		else
		{
			on_connected(); //先注册读，再注销写
			m_rt->unregister_handler(this, SE_WRITE);
		}
		//m_hip = ip;
		//m_hport = port;
		m_ep.set(ip, port);
		return 0;
	}

	int tcpchannel::disconnect()
	{
		if (DISCONNECTED != m_state)
		{
			m_rt->unregister_handler(this, SE_BOTH);
			closesocket(m_fd);
			reset();
			call(tcpchannelListener::Disconnected(), this);
		}
		return 0;
	}
	int tcpchannel::send(memblock *b)
	{
		m_last_active_tick = cl::mtick();
		assert(b);
		if (CONNECTED != m_state)
		{
			b->release();
			return -1;
		}
		if (b->wpos <= b->rpos)
		{
			b->release();
			return 0;
		}
		//可能Peer同时被download 与 share占用.这样download调用的send很容易会使share无法发数据.所以发送时总执行一下注册可写
		m_smore = true;
		if (!m_slist.empty())
		{
			m_slist.push_back(b);
			return 1;
		}
		int ret = ::send(m_fd, b->buf + b->rpos, b->wpos - b->rpos, 0);
		if (ret > 0)
		{
			b->rpos += ret;
			if (b->rpos < b->wpos)
			{
				//DEBUGMSG("#send blocking...\n");
				m_slist.push_front(b);
			}
			else
			{
				b->release();
			}
		}
		else
		{
#ifdef _MSC_VER
			int err = WSAGetLastError();
			if (-1 == ret && WSAEWOULDBLOCK == err)
#else
			if (-1 == ret && EAGAIN == errno)
#endif
			{
				//DEBUGMSG("#send blocking...\n");
				m_slist.push_front(b);
			}
			else
			{
				b->release();
				disconnect();
				return -1;
			}
		}
		//不能使用more参数，因为可能share发完要求more,而跟着download发请求不需要more，此时会导致share的more无效
		//注意：使用边缘触发时，可写只提醒一次
		if (!m_is_regwrite && 1 == m_slist.size())
		{
			m_is_regwrite = true;
			m_rt->register_handler(this, SE_WRITE);
		}
		//1表示发送阻塞
		return m_slist.empty() ? 0 : 1;
	}
	int tcpchannel::recv(char *b, int size)
	{
		m_last_active_tick = cl::mtick();
		int ret = ::recv(m_fd, b, size, 0);
		if (ret > 0)
		{
			return ret;
		}
		else
		{
#ifdef _MSC_VER
			int err = WSAGetLastError();
			if (-1 == ret && WSAEWOULDBLOCK == err)
#else
			if (-1 == ret && EAGAIN == errno)
#endif
				return 0;
			else
			{
				disconnect();
				return -1;
			}
		}
	}

	int tcpchannel::handle_input()
	{
		//assert(CONNECTED==m_state);
		if (CONNECTED != m_state)
			return 0;
		//tcpchannel考虑提交上面去读
		int wait = 0;
		call(tcpchannelListener::Readable(), this, &wait);
		return wait;
	}
	int tcpchannel::handle_output()
	{
		//epool 边缘触发只提示一次
		if (CONNECTED == m_state)
		{
			while (!m_slist.empty())
			{
				memblock *b = m_slist.front();
				int ret = ::send(m_fd, b->buf + b->rpos, b->wpos - b->rpos, 0);
				if (ret > 0)
				{
					b->rpos += ret;
					if (b->rpos >= b->wpos)
					{
						m_slist.pop_front();
						b->release();
					}
					else
					{
						//DEBUGMSG("#send blocking...\n");
						break;
					}
				}
				else
				{
#ifdef _MSC_VER
					int err = WSAGetLastError();
					if (-1 == ret && WSAEWOULDBLOCK == err)
#else
					if (-1 == ret && EAGAIN == errno)
#endif
						break;
					else
					{
						disconnect();
						return -1;
					}

				}
			}
			if (m_slist.empty())
			{
				if (m_smore)
				{
					//注意避免无发送也总是监听
					m_smore = false;
					call(tcpchannelListener::Writable(), this); //这里会可能导致多次发送到阻塞，所以避免在里面重复注册写事件
				}
				else
				{
					//对于边缘触发其实是不会跑到这里，因为只提示一次，而且那一次肯定是m_smore=true
					m_rt->unregister_handler(this, SE_WRITE);
					m_is_regwrite = false;
				}
			}

		}
		else if (CONNECTING == m_state)
		{
			int err = 0;
			socklen_t len = sizeof(err);
			getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
			if (err)
			{
				//DEBUGMSG("***TCPChannel::handle_output()::CONNECTING::errno=0x%x\n",err);
				disconnect();
				return -1;
			}
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			len = sizeof(addr);
			if (0 == getsockname(m_fd, (sockaddr*)&addr, &len))
			{
				//DEBUGMSG("#TCP connect() ok my ipport=%s:%d \n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
			}
			//DEBUGMSG("#TCP connect() ok desip(%s:%d) \n",cl::net::ip_htoa(m_hip),(int)m_hport);
			m_rt->unregister_handler(this, SE_WRITE);
			on_connected(); //有可能在里面已经有发送到阻塞成注册了写事件的情况，所以在此先去掉写事件再调用到此
		}
		else
		{
			assert(0);
		}
		return 0;
	}
	int tcpchannel::handle_error()
	{
		int err = 0;
		socklen_t len = sizeof(err);
		getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
		//DEBUGMSG("***TcpConnection::OnError()::errno=0x%x\n",err);
		disconnect();
		return 0;
	}

}
