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
		//���ʧ�ܣ��˴����ر�s
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
		//����Ϊhost ip��host port
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
			on_connected(); //��ע�������ע��д
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
		//����Peerͬʱ��download �� shareռ��.����download���õ�send�����׻�ʹshare�޷�������.���Է���ʱ��ִ��һ��ע���д
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
		//����ʹ��more��������Ϊ����share����Ҫ��more,������download��������Ҫmore����ʱ�ᵼ��share��more��Ч
		//ע�⣺ʹ�ñ�Ե����ʱ����дֻ����һ��
		if (!m_is_regwrite && 1 == m_slist.size())
		{
			m_is_regwrite = true;
			m_rt->register_handler(this, SE_WRITE);
		}
		//1��ʾ��������
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
		//tcpchannel�����ύ����ȥ��
		int wait = 0;
		call(tcpchannelListener::Readable(), this, &wait);
		return wait;
	}
	int tcpchannel::handle_output()
	{
		//epool ��Ե����ֻ��ʾһ��
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
					//ע������޷���Ҳ���Ǽ���
					m_smore = false;
					call(tcpchannelListener::Writable(), this); //�������ܵ��¶�η��͵����������Ա����������ظ�ע��д�¼�
				}
				else
				{
					//���ڱ�Ե������ʵ�ǲ����ܵ������Ϊֻ��ʾһ�Σ�������һ�ο϶���m_smore=true
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
			on_connected(); //�п����������Ѿ��з��͵�������ע����д�¼�������������ڴ���ȥ��д�¼��ٵ��õ���
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
