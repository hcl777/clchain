#pragma once
#include "incnet.h"
#include <assert.h>
#include "clarray.h"
#include "net.h"


namespace cl
{
	//socket event:
#define SE_READ   0x01
#define SE_WRITE  0x02
#define SE_BOTH   0x03

	//*********************** rthandle *******************************
	class rthandle
	{
		friend class reactorSelect;
		friend class reactorEpoll;
	public:
		rthandle() : __i(-1) {}
		virtual ~rthandle() {}

		virtual int sock() = 0;
		virtual int handle_input() = 0;
		virtual int handle_output() = 0;
		virtual int handle_error() = 0;
	private:
		int __i; //仅由reactor使用
	};
	//*********************** reactor *******************************
	class reactor
	{
	public:
		reactor() { assert(false); }
		reactor(int max_handle_num) :m_handler_num(0), m_max_num(max_handle_num) {}
		virtual ~reactor() {}
		typedef struct tag_Node
		{
			rthandle		*h;	//handler
			int				s;	//socket handle
			int				se;	// event (SE_READ,SE_WRITE,SE_BOTH)
			tag_Node() { reset(); }
			void reset() { h = NULL; s = (int)INVALID_SOCKET; se = 0; }
		}Node_t;

		virtual int register_handler(rthandle *h, int se) { assert(0); return 0; }
		virtual int unregister_handler(rthandle *h, int se) { assert(0); return 0; }
		virtual void handle_root(uint64_t delay_usec = 0) { assert(0); }
		int get_max_num() const { return m_max_num; }
	protected:
		unsigned int	m_handler_num;
		int				m_max_num;
	};
	//*********************** reactorSelect *******************************
	class reactorSelect : public reactor
	{
	public:
		reactorSelect(int max_handle_num);
		virtual ~reactorSelect();

		typedef struct tag_select_node
		{
			Node_t*		sn;
			fd_set		rfd, wfd, efd;
			int			max_sock;
		}select_node_t;
	public:
		virtual int register_handler(rthandle *h, int se);
		virtual int unregister_handler(rthandle *h, int se);
		virtual void handle_root(uint64_t delay_usec = 0);

	private:
		int allot_nodei();
		void select_finish(Node_t* sn, fd_set* prfd, fd_set* pwfd, fd_set* pefd, int n);
	private:
		select_node_t * m_sns;
		int				m_sn_length;
		//Node_t *m_sn;
		//fd_set m_rfd,m_wfd,m_efd;
		//int m_maxsock;
		fd_set			rfd, wfd, efd;
		int				m_cursor;
	};


	//*********************** reactorEpoll *******************************
#if defined(__GNUC__) && !defined(NO_EPOLL)
	//EPOLLLT(水平触发,可读写会反复触发：默认)，EPOLLET: 边缘触发，状态变更只提醒一次

	class reactorEpoll : public reactor
	{
	public:
		reactorEpoll(int max_handle_num);
		virtual ~reactorEpoll();
	public:
		virtual int register_handler(rthandle *h, int se);
		virtual int unregister_handler(rthandle *h, int se);
		virtual void handle_root(uint64_t delay_usec = 0);
	private:

		int					m_epfd;
		epoll_event			*events;
		int					nfds;
		array<Node_t>		m_sn;
	};
#endif
}

