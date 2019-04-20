#include "mysqlchannel.h"

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "mysql.h"



/*
mysqlclient.lib的编译方法：
window编译：
1。安装cmake工具；
2。下载mysql-connector-c 源码解压；
3。生成VC工程：
打开cmake，选择源代码路径，选择生成工程路径
点configure，选择已经安装的VS平台，再点generate生成VC工程；
生成完成后会在工程目录下创建VC工程。
4。编译debug或者release版本：
可单独选择mysqlclient编译，正常成功后生成mysqlclient.lib在libmysql\Debug\目录下面。
拷去使用即可。
*/

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

#ifdef _WIN32
#ifdef _DEBUG
#pragma comment(lib,"D:/source/mysql/lib/vc15_64/debug/mysqlclient.lib")
#else
#pragma comment(lib,"D:/source/mysql/lib/vc15_64/release/mysqlclient.lib")
#endif
#endif


namespace cl
{
	//******************************************
	//没有连接数据库时，转换成gdk格式的字符串例子
	//MYSQL g_gbk_mysql;
	//g_gbk_mysql.charset = &my_charset_gbk_bin;

	//**********************************************************************

	//MYSQL_RES
	mysqlQuery::mysqlQuery(void* pmysql, mysqlchannel* cn)
		: m_pmysql(pmysql)
		, m_pres(NULL)
		, m_cn(cn)
	{
	}
	mysqlQuery::~mysqlQuery(void)
	{
		free_result();
	}
	void mysqlQuery::free_result()
	{
		if (m_pres)
		{
			mysql_free_result((MYSQL_RES*)m_pres);
			m_pres = NULL;
		}
	}
	int mysqlQuery::query(const char* sql, int* affected_rows/*=NULL*/)
	{
		free_result();
		MYSQL* pmysql = (MYSQL*)m_pmysql;
		if (NULL == pmysql)
			return -1;

		try
		{
			//有时候会抛异常
			if (0 != mysql_real_query(pmysql, sql, (int)strlen(sql)))
			{
				char log[4096];
				sprintf(log, "mysql_real_query(%s) : errno=%d,msg=[%s]", sql, mysql_errno(pmysql), mysql_error(pmysql));
				cl::write_log(log,(get_module_dir() + "mysqlerr.log").c_str());
				printf("%s\n", log);
				m_cn->ping();
				return -2;
			}
		}
		catch (...)
		{
			char log[4096];
			sprintf(log, "exception: mysql_real_query(%s) : errno=%d,msg=[%s]", sql, mysql_errno(pmysql), mysql_error(pmysql));
			cl::write_log(log, (cl::get_module_dir() + "mysqlerr.log").c_str());
			m_cn->ping();
			return -3;
		}
		if (affected_rows)
			*affected_rows = (int)mysql_affected_rows(pmysql);

		//不论有没有结果返回，以下执行语句都不会错，也就是正常后面的mysql_errno(pmysql)==0;
		m_pres = (MYSQL_RES*)mysql_store_result(pmysql);

		if (0 != mysql_errno(pmysql))
		{
			m_pres = NULL;
			printf("***mysql_store_result(%s) ERR: [%d] [%s] \n", sql, mysql_errno(pmysql), mysql_error(pmysql));
			return -4;
		}
		return 0;
	}

	int mysqlQuery::get_num_rows()
	{
		if (NULL == m_pres)
			return 0;
		return (int)mysql_num_rows((MYSQL_RES*)m_pres);
	}

	int mysqlQuery::get_num_fields()
	{
		if (NULL == m_pres)
			return 0;
		return mysql_num_fields((MYSQL_RES*)m_pres);
	}

	int mysqlQuery::next_row(char*** row)
	{
		if (NULL == m_pres)
			return -1;
		MYSQL_ROW  myrow = mysql_fetch_row((MYSQL_RES*)m_pres);
		if (myrow)
		{
			*row = myrow;
			return 0;
		}
		return -1;
	}
	char* mysqlQuery::escape_string(char *to, int tolen, const char *from)
	{
		return mysqlQuery::escape_string(to, tolen, from, m_pmysql);
	}
	char* mysqlQuery::escape_string(char *to, int tolen, const char *from, void *db)
	{
		if (0 == to || 0 == from)
			return NULL;
		int reallen = (int)strlen(from);
		if (reallen == 0 || tolen < (2 * reallen + 1))
			return NULL;

		if (db)
		{
			mysql_real_escape_string((MYSQL*)db, to, from, reallen);
		}
		else
		{
			mysql_escape_string(to, from, reallen);
		}
		return to;
	}



	//**********************************************************************
	mysqlchannel::mysqlchannel(void)
		: query(NULL, this)
		, m_pmysql(NULL)
	{
		//connect();
		m_dbcode[0] = '\0';
	}

	mysqlchannel::~mysqlchannel(void)
	{
		disconnect();
	}
	int mysqlchannel::connect(const char* dbaddr, const char* dbcode/*="utf8"*/)
	{
		//user:pass@ip:port/dbname
		std::string ip, user, passwd, dbname;
		int port;
		std::string addr, str, str2;

		strcpy(m_dbaddr, dbaddr);
		if (dbcode)
			strcpy(m_dbcode, dbcode); //utf8 or gbk
		else
			m_dbcode[0] = '\0';
		addr = m_dbaddr;
		if (addr.empty())
			return -1;

		dbname = cl::get_string_index(addr, 1, "/");
		str = cl::get_string_index(addr, 0, "@");
		user = cl::get_string_index(str, 0, ":");
		passwd = cl::get_string_index(str, 1, ":");
		str = cl::get_string_index(addr, 1, "@");
		ip = cl::get_string_index(str, 0, ":");
		port = cl::atoi(cl::get_string_index(str, 1, ":").c_str(),3306);

		disconnect();
		if (NULL == (m_pmysql = mysql_init(NULL)))
		{
			printf("mysql init() error\n");
			return -1;
		}

		if (NULL == mysql_real_connect((MYSQL*)m_pmysql, ip.c_str(), user.c_str(), passwd.c_str(), dbname.c_str(), port, NULL, 0))
		{
			printf("mysql_real_connect error: [%d] %s\n", mysql_errno((MYSQL*)m_pmysql), mysql_error((MYSQL*)m_pmysql));
			disconnect();
			return -2;
		}
		//设置写入编码
		if (strlen(m_dbcode))
		{
			char names[128];
			sprintf(names, "set names %s", m_dbcode); //utf8 or gbk
			mysql_real_query((MYSQL*)m_pmysql, names, (int)strlen(names));
		}
		query.set_mysql(m_pmysql);
		printf("#mysql connect(%s) ok! \n", m_dbaddr);
		return 0;
	}


	int mysqlchannel::disconnect()
	{
		if (m_pmysql)
		{
			mysql_close((MYSQL*)m_pmysql);
			m_pmysql = NULL;
			query.set_mysql(NULL);
			mysql_library_end(); //调用此结束，避免内存泄漏
			printf("# ** dbdisconnect() ...\n");
		}
		return 0;
	}
	int mysqlchannel::ping()
	{
		if (m_pmysql)
		{
			if (0 != mysql_ping((MYSQL*)m_pmysql))
				return connect(m_dbaddr);
			else
			{
				printf("#--db ping ok!--\n");
			}
		}
		else
		{
			connect(m_dbaddr);
		}
		return -1;
	}
}

