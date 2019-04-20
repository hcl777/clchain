#include "clreactor.h"

namespace cl
{
	//*********************** reactorSelect *******************************
	reactorSelect::reactorSelect(int max_handle_num)
		:reactor(max_handle_num)
	{
		//FD_SETSIZE 的整数倍
		m_cursor = -1;
		m_sn_length = (m_max_num - 1) / FD_SETSIZE + 1;
		m_sns = new select_node_t[m_sn_length];
		for (int i = 0; i<m_sn_length; ++i)
		{
			m_sns[i].max_sock = (int)INVALID_SOCKET;
			m_sns[i].sn = new Node_t[FD_SETSIZE];
			FD_ZERO(&m_sns[i].rfd);
			FD_ZERO(&m_sns[i].wfd);
			FD_ZERO(&m_sns[i].efd);
		}
	}
	reactorSelect::~reactorSelect()
	{
		assert(-1 == m_cursor);
		for (int i = 0; i<m_sn_length; ++i)
			delete[] m_sns[i].sn;
		delete[] m_sns;
	}

	int reactorSelect::allot_nodei()
	{
		for (int i = 0; i<m_max_num; ++i)
		{
			if (!m_sns[i / FD_SETSIZE].sn[i%FD_SETSIZE].h)
				return i;
		}
		return -1;
	}
	int reactorSelect::register_handler(rthandle *h, int se)
	{
		assert(h && (se & SE_BOTH));
		if (NULL == h)
			return -1;

		int i = h->__i;
		int i1;
		int fd = h->sock();
		Node_t *sn;
		if (-1 == i)
		{
			//新分配
			i = allot_nodei();
			if (-1 == i)
				return -1;
			if (m_cursor<i)
				m_cursor = i;
			h->__i = i;

			i1 = i / FD_SETSIZE;
			sn = &m_sns[i1].sn[i%FD_SETSIZE];
			sn->h = h;
			sn->s = fd;
			assert(fd != INVALID_SOCKET);
			net::set_nonblock(fd, 1);
			FD_SET(fd, &m_sns[i1].efd);
			if (m_sns[i1].max_sock<fd)
				m_sns[i1].max_sock = fd;
			m_handler_num++;
		}
		else
		{
			i1 = i / FD_SETSIZE;
			sn = &m_sns[i1].sn[i%FD_SETSIZE];
			assert(sn->h == h);
		}
		if (0 != (sn->se&se))
			assert(false);//避免重复注册
		if (se == (sn->se&se))
			return 0;//已经有对应的
		sn->se |= se;
		if (se & SE_READ)
			FD_SET(sn->s, &m_sns[i1].rfd);
		if (se & SE_WRITE)
			FD_SET(sn->s, &m_sns[i1].wfd);

		return 0;
	}
	int reactorSelect::unregister_handler(rthandle *h, int se)
	{
		assert(h && (se & SE_BOTH));
		if (NULL == h)
			return -1;

		int i = h->__i;
		int i1;
		Node_t *sn;
		if (-1 == i)
			return -1;
		i1 = i / FD_SETSIZE;
		sn = &m_sns[i1].sn[i%FD_SETSIZE];
		assert(sn->h == h);
		sn->se &= ~se;
		if (se & SE_READ)
			FD_CLR(sn->s, &m_sns[i1].rfd);
		if (se & SE_WRITE)
			FD_CLR(sn->s, &m_sns[i1].wfd);
		if (0 == sn->se)
		{
			FD_CLR(sn->s, &m_sns[i1].efd);
			int s = (int)sn->s;
			//释放
			h->__i = -1;
			net::set_nonblock(sn->s, 0);
			sn->reset();
			while (m_cursor >= 0 && !m_sns[m_cursor / FD_SETSIZE].sn[m_cursor%FD_SETSIZE].h)
				m_cursor--;
			if (s == m_sns[i1].max_sock)
			{
				//重新计算最大的socket
				m_sns[i1].max_sock = (int)INVALID_SOCKET;
				for (int j = 0; j <= FD_SETSIZE; ++j)
					if (m_sns[i1].max_sock<(int)m_sns[i1].sn[j].s)
						m_sns[i1].max_sock = (int)m_sns[i1].sn[j].s;
			}
			m_handler_num--;
		}
		return 0;
	}
	void reactorSelect::handle_root(uint64_t delay_usec/*=0*/)
	{
		timeval timeout;
		timeout.tv_sec = (long)(delay_usec / 1000000);
		timeout.tv_usec = (long)(delay_usec % 1000000);
		int n;

		for (int i = 0; i<m_sn_length; ++i)
		{
			memcpy(&rfd, &m_sns[i].rfd, sizeof(fd_set));
			memcpy(&wfd, &m_sns[i].wfd, sizeof(fd_set));
			memcpy(&efd, &m_sns[i].efd, sizeof(fd_set));
			//rfd=m_sns[i].rfd; //自动拷贝数组数据
			//wfd=m_sns[i].wfd;
			//efd=m_sns[i].efd;

			if (INVALID_SOCKET == m_sns[i].max_sock)
			{
				msleep((long)(delay_usec / 1000));
			}
			else
			{
				n = select(m_sns[i].max_sock + 1, &rfd, &wfd, &efd, &timeout);
				if (n>0)
					select_finish(m_sns[i].sn, &rfd, &wfd, &efd, n);
			}
		}
	}

	void reactorSelect::select_finish(Node_t* sn, fd_set* prfd, fd_set* pwfd, fd_set* pefd, int n)
	{
		Node_t *p;
		int j = 0;
		int wait = 0;
		//注意考虑回调后有些被注销了的情况
		for (int i = 0; i <= m_cursor; ++i)
		{
			if (!sn[i].h)
				continue;
			p = &sn[i];
			if (FD_ISSET(p->s, pefd))
			{
				++j;
				p->h->handle_error();
			}
			if ((p->se & SE_READ) && FD_ISSET(p->s, prfd))
			{
				++j;
				if (1 == p->h->handle_input())
				{
					wait++;
				}
			}

			if ((p->se & SE_WRITE) && FD_ISSET(p->s, pwfd))
			{
				++j;
				if (1 == p->h->handle_output())
				{
					wait++;
				}
			}
			if (j >= n)
				break;
		}
		if (wait && wait == j)
			msleep(1); //限速使用
	}
	//*********************** reactorEpoll *******************************

#if defined(__GNUC__) && !defined(NO_EPOLL)
#include <errno.h>

#define EPOLL_WAIT_SIZE 1024
	//epoll_create(int size); Since Linux 2.6.8, the size argument is unused

	reactorEpoll::reactorEpoll(int max_handle_num)
		:reactor(max_handle_num)
	{
		m_epfd = epoll_create(m_max_num);
		if (-1 == m_epfd)
			DEBUGMSG("*** epoll_create error:%d\n", errno);
		events = new epoll_event[EPOLL_WAIT_SIZE];
		memset(events, 0, EPOLL_WAIT_SIZE * sizeof(epoll_event));
		if (-1 == m_sn.resize(m_max_num))
		{
			DEBUGMSG("# *** ProReactor: m_sn.resize(%d) failed. allot memery failed !!! \n", m_max_num);
			assert(false);
		}
	}
	reactorEpoll::~reactorEpoll()
	{
		close(m_epfd);
		m_sn.resize(0);
		delete[] events;
	}
	int reactorEpoll::register_handler(rthandle *h, int se)
	{
		assert(h && (se & SE_BOTH));
		int i = h->__i;
		int s = h->sock();
		assert(-1 != s);
		bool bnew = false;
		if (-1 == i)
		{
			//不存在于列表先从列表注册一个
			i = m_sn.rallot();
			if (-1 == i)
				return -1;
			bnew = true;
			h->__i = i;
			m_sn[i].h = h;
			m_sn[i].s = s;
			net::set_nonblock(s, 1);
			m_handler_num++;
		}
		else
		{
			assert(m_sn[i].h == h);
		}
		if (0 != (m_sn[i].se&se))
			assert(false);//避免重复注册
		if (se == (m_sn[i].se&se))
			return 0;//已经有对应的

					 //注意:修改监听事件时,要把旧的事件加上
					 //m_sn[i].is_et = is_et;
		se = m_sn[i].se | se;
		epoll_event ev;
		memset(&ev, 0, sizeof(ev));
		ev.events = 0;
		ev.data.fd = i;
		//if(m_sn[i].is_et)
		//	ev.events = EPOLLET/*|EPOLLERR*/;
		if (se & SE_READ) ev.events |= EPOLLIN;
		if (se & SE_WRITE) ev.events |= EPOLLOUT;

		int ret = 0;
		if (bnew)
			ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, s, &ev);
		else
			ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, s, &ev);
		if (ret != 0)
		{
			DEBUGMSG("***error fd=%d se=%d ***\n", s, se);
			perror("epoll_ctl(m_epfd,EPOLL_CTL_,fd,&ev);");
			//取消注册
			if (bnew)
			{
				net::set_nonblock(s, 0);
				m_sn[i].reset();
				m_sn.free(i);
				h->__i = -1;
			}
			return -1;
		}
		m_sn[i].se = se;
		return 0;
	}
	int reactorEpoll::unregister_handler(rthandle *h, int se)
	{
		assert(h && (se & SE_BOTH));
		if (!h)
			return -1;
		int i = h->__i;
		int s = h->sock();
		if (-1 == i)
			return 0;

		se = m_sn[i].se & (~se);
		epoll_event ev;
		memset(&ev, 0, sizeof(ev));
		ev.events = 0;
		ev.data.fd = i;
		if (0 == se)
		{
			epoll_ctl(m_epfd, EPOLL_CTL_DEL, s, &ev);
			net::set_nonblock(m_sn[i].s, 0);
			m_sn[i].reset();
			m_sn.free(i);
			h->__i = -1;
			m_handler_num--;
		}
		else
		{
			//if(m_sn[i].is_et)
			//	ev.events = EPOLLET/*|EPOLLERR*/;
			if (se & SE_READ) ev.events |= EPOLLIN;
			if (se & SE_WRITE) ev.events |= EPOLLOUT;
			if (-1 == epoll_ctl(m_epfd, EPOLL_CTL_MOD, s, &ev))
			{
				perror("epoll_ctl(m_epfd,EPOLL_CTL_MOD,s,&ev);");
				return -1;
			}
			m_sn[i].se = se;
		}
		return 0;
	}
	void reactorEpoll::handle_root(uint64_t delay_usec/*=0*/)
	{
		nfds = epoll_wait(m_epfd, events, EPOLL_WAIT_SIZE, delay_usec / 1000);
		//返回0为timeout
		if (-1 == nfds)
		{
			perror("epoll_wait error");
			DEBUGMSG("epoll_wait error:%d\n", errno);
		}
		else if (nfds>0)
		{
			int n = 0;
			//DEBUGMSG("ProReactor::handle_root :nfds=%d \n",nfds);
			rthandle *h = NULL;
			for (int i = 0; i<nfds; ++i)
			{
				//是否一次响应多事件
				n = 0;
				if (events[i].events & EPOLLIN)
				{
					h = m_sn[events[i].data.fd].h;
					if (h)
						h->handle_input();
					n++;
				}
				if (events[i].events & EPOLLOUT)
				{
					h = m_sn[events[i].data.fd].h;
					if (h)
						h->handle_output();
					n++;
				}
				if (events[i].events & EPOLLERR)
				{
					h = m_sn[events[i].data.fd].h;
					if (h)
						h->handle_error();
					n++;
				}
				if (0 == n || n>1)
				{
					DEBUGMSG("***************once %d events : event=0x%x \n", n, events[i].events);
				}
			}
		}
	}
#endif

}

