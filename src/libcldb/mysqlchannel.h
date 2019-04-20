#pragma once

namespace cl
{
	class mysqlchannel;
	class mysqlQuery
	{
	public:
		mysqlQuery(void* pmysql, mysqlchannel* cn);
		~mysqlQuery(void);
	public:
		void set_mysql(void* pmysql) { m_pmysql = pmysql; }
		int query(const char* sql, int* affected_rows = 0);
		int get_num_rows();
		int get_num_fields();
		int next_row(char*** row);

		//ת���ַ���Ϊsql��ʶ���
		char* escape_string(char *to, int tolen, const char *from);
		static char* escape_string(char *to, int tolen, const char *from, void *db);

	private:
		void free_result();
	private:
		void* m_pmysql;
		void* m_pres;
		mysqlchannel* m_cn;
	};

	class mysqlchannel
	{
	public:
		mysqlchannel(void);
		~mysqlchannel(void);
	public:
		int connect(const char* dbaddr, const char* dbcode = "utf8"); //user:pass@ip:port/dbname
		int disconnect();
		int ping();
		void *get_mysql() const { return m_pmysql; }


	public:
		mysqlQuery query;
	private:
		void	*m_pmysql;
		char	m_dbaddr[256];
		char	m_dbcode[32];
	};
}
