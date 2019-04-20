#pragma once
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <list>
#include "file32.h"

namespace cl
{
	using namespace std;
	class inifile
	{
	public:
		inifile(void);
		~inifile(void);
		typedef struct ini_key {
			string name;
			string value;
			string seq;
			//ini_key(void){}
			//ini_key(const ini_key& akey): name(akey.name),value(akey.value),seq(akey.seq) {}
			//~ini_key(void){}
		}ini_key_t;

		typedef struct ini_section {
			string name;
			list<ini_key_t> key_list;
			~ini_section(void) {}
		}ini_section_t;

		static string& string_trim(string& str, char c = ' ');
		static string& string_trim_endline(string& str);
	public:
		/*
		add 2 new function

		int parse
		bool has_key
		*/
		int parse(const char * content);
		bool has_key(const char * section_name, const char * key_name);


		int open(const char* path);
		int close();

		int write_string(const char* section_name, const char* key_name, const char* inval);
		int write_int(const char* section_name, const char* key_name, int inval);

		char* read_string(const char* section_name, const char* key_name, const char* default_val, char *outbuf, unsigned int outbuflen);
		int read_int(const char* section_name, const char* key_name, int default_val);
	private:
		int add_section(const char* section_name, const ini_key_t& key);
		void save();
	private:
		bool m_bopen;
		bool m_bchanged;
		list<ini_section_t*> m_data_list;
		string m_path;
	};

	int WritePrivateProfileIntA(const char* szAppName, const char* szKeyName, int nValue, const char* szFileName);
#ifdef __GNUC__
	int GetPrivateProfileIntA(const char* szAppName, const char* szKeyName, int nDefault, const char* szFileName);
	char* GetPrivateProfileStringA(const char* szAppName, const char* szKeyName, const char* szDefault, char* szOut, unsigned int nOutLen, const char* szFileName);
	int WritePrivateProfileStringA(const char* szAppName, const char* szKeyName, const char* szValue, const char* szFileName);
#endif

}

