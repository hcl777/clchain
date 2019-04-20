#include "inifile.h"
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#ifdef _WIN32
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif


namespace cl
{
	inifile::inifile(void)
		: m_bopen(false)
		, m_bchanged(false)
	{
	}

	inifile::~inifile(void)
	{
		close();
	}

	string& inifile::string_trim(string& str, char c/*=' '*/)
	{
		int pos1 = 0, pos2 = (int)str.length() - 1;
		for (; pos1 <= pos2 && c == str.at(pos1); ++pos1);
		for (; pos2>pos1 && c == str.at(pos2); --pos2);
		if (pos1>pos2)
			str = "";
		else
			str = str.substr(pos1, pos2 - pos1 + 1);
		return str;
	}

	string& inifile::string_trim_endline(string& str)
	{
		int pos1 = 0, pos2 = (int)str.length() - 1;
		for (; pos1 <= pos2 && ('\r' == str.at(pos1) || '\n' == str.at(pos1)); ++pos1);
		for (; pos2> pos1 && ('\r' == str.at(pos2) || '\n' == str.at(pos2)); --pos2);
		if (pos1>pos2)
			str = "";
		else
			str = str.substr(pos1, pos2 - pos1 + 1);
		return str;
	}


	int inifile::parse(const char * content)
	{
		assert(!m_bopen);
		if (m_bopen)
			close();

		m_path = "/noexist";
		m_bopen = true;

		char buf[10241];
		string str;
		ini_section_t *psec = NULL;
		ini_key_t key;

		char * start = (char *)content;
		char * end = strchr(start, '\n');
		while (start != NULL)
		{
			if (end != NULL)
			{
				int len = (int)(end - start);
				if (len > 10240) {
					len = 10240;
				}
				strncpy(buf, start, len);
				buf[len] = 0;
				str = buf;
				start = end + 1;
				end = strchr(start, '\n');
			}
			else
			{
				str = start;
				start = NULL;
			}

			string_trim_endline(str);
			string_trim(str);

			//printf("parse line: %s\n", str.c_str() );

			if (str.empty() == false)
			{
				if ('[' == str.at(0) && ']' == str.at(str.length() - 1))
				{
					str = str.substr(1, str.length() - 2);
					if (psec)
					{
						m_data_list.push_back(psec);
						psec = NULL;
					}
					psec = new ini_section_t();
					psec->name = str;
				}
				else
				{
					if (!psec)
					{
						psec = new ini_section_t();
						psec->name = "";
					}
					size_t pos = str.find("=");
					if (pos != string::npos)
					{
						key.name = str.substr(0, pos);
						key.value = str.substr(pos + 1);
						key.seq = "=";
						string_trim(key.name);
						string_trim(key.value);
					}
					else
						key.name = str;
					psec->key_list.push_back(key);
				}

			}
		}
		if (psec)
		{
			m_data_list.push_back(psec);
			psec = NULL;
		}
		return 0;
	}

	bool inifile::has_key(const char* section_name, const char* key_name)
	{
		assert(m_bopen);
		if (!section_name || !key_name)
			return false;

		list<ini_section_t*>::iterator sit;
		list<ini_key_t>::iterator kit;
		ini_section_t *psec = NULL;
		for (sit = m_data_list.begin(); sit != m_data_list.end(); ++sit)
		{
			psec = *sit;
			if (0 == strcmp(psec->name.c_str(), section_name))
			{
				for (kit = psec->key_list.begin(); kit != psec->key_list.end(); ++kit)
				{
					if (0 == strcmp(key_name, (*kit).name.c_str()))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	//section和key重复的不重新整理，以前面的为准
	int inifile::open(const char* path)
	{
		assert(!m_bopen);
		if (m_bopen)
			close();

		m_path = path;
		file32 file;
		file.open(path, F32_READ | F32_BINARY);
		if (!file.is_open())
		{
			file.open(path, F32_RDWR | F32_TRUNC | F32_BINARY);
			if (!file.is_open())
				return -1;
			//不读入数据
			file.close();
			unlink(path);
			m_bopen = true;
			return 0;
		}

		char* buf = new char[10240];
		string str;
		size_t pos = 0;
		ini_section_t *psec = NULL;
		ini_key_t key;
		list<ini_section_t *>::iterator it;
		while (!file.eof())
		{
			if (NULL == file.getline(buf, 10240))
				continue;

			str = buf;
			//不支持名称和值前后空格
			string_trim_endline(str);
			string_trim(str);
			if (str.empty())
				continue;
			if ('[' == str.at(0) && ']' == str.at(str.length() - 1))
			{
				str = str.substr(1, str.length() - 2);
				if (psec)
				{
					m_data_list.push_back(psec);
					psec = NULL;
				}
				psec = new ini_section_t();
				psec->name = str;
			}
			else
			{
				if (!psec)
				{
					//建一个空section;
					psec = new ini_section_t();
					psec->name = "";
				}
				pos = str.find("=");
				if (pos != string::npos)
				{
					key.name = str.substr(0, pos);
					key.value = str.substr(pos + 1);
					key.seq = "=";
					string_trim(key.name);
					string_trim(key.value);
				}
				else
					key.name = str;
				psec->key_list.push_back(key);
			}
		}
		if (psec)
		{
			m_data_list.push_back(psec);
			psec = NULL;
		}
		delete[] buf;

		file.close();
		m_bopen = true;
		return 0;
	}
	int inifile::close()
	{
		if (m_bopen)
		{
			save();
			list<ini_section_t*>::iterator sit;
			for (sit = m_data_list.begin(); sit != m_data_list.end(); ++sit)
				delete (*sit);
			m_data_list.clear();
			m_path = "";
			m_bopen = false;
		}
		return 0;
	}
	int inifile::add_section(const char* section_name, const ini_key_t& key)
	{
		ini_section_t *psec = new ini_section_t();
		psec->name = section_name;
		psec->key_list.push_back(key);
		m_data_list.push_back(psec);
		return 0;
	}
	int inifile::write_string(const char* section_name, const char* key_name, const char* inval)
	{
		assert(m_bopen);
		if (!section_name || !key_name || !inval)
			return -1;
		m_bchanged = true;

		list<ini_section_t*>::iterator sit;
		list<ini_key_t>::iterator kit;
		ini_section_t *psec = NULL;
		ini_key_t key;
		for (sit = m_data_list.begin(); sit != m_data_list.end(); ++sit)
		{
			psec = *sit;
			if (0 == strcmp(psec->name.c_str(), section_name))
			{
				for (kit = psec->key_list.begin(); kit != psec->key_list.end(); ++kit)
				{
					if (0 == strcmp(key_name, (*kit).name.c_str()))
					{
						(*kit).value = inval;
						return 0;
					}
				}
				//
				key.name = key_name;
				key.value = inval;
				key.seq = "=";
				psec->key_list.push_back(key);
				return 0;
			}
			psec = NULL;
		}

		key.name = key_name;
		key.value = inval;
		key.seq = "=";
		//psec = new ini_section_t();
		//psec->name = section_name;
		//psec->key_list.push_back(key);
		//m_data_list.push_back(psec);
		add_section(section_name, key);
		return 0;
	}
	int inifile::write_int(const char* section_name, const char* key_name, int inval)
	{
		assert(m_bopen);
		m_bchanged = true;

		char buf[32];
		sprintf(buf, "%d", inval);
		return write_string(section_name, key_name, buf);
	}

	char* inifile::read_string(const char* section_name, const char* key_name, const char* default_val, char *outbuf, unsigned int outbuflen)
	{
		assert(m_bopen);
		if (!section_name || !key_name || !outbuf)
			return NULL;

		list<ini_section_t*>::iterator sit;
		list<ini_key_t>::iterator kit;
		ini_section_t *psec = NULL;
		for (sit = m_data_list.begin(); sit != m_data_list.end(); ++sit)
		{
			psec = *sit;
			if (0 == strcmp(psec->name.c_str(), section_name))
			{
				for (kit = psec->key_list.begin(); kit != psec->key_list.end(); ++kit)
				{
					if (0 == strcmp(key_name, (*kit).name.c_str()))
					{
						if ((*kit).value.length() >= outbuflen)
							return NULL;
						strcpy(outbuf, (*kit).value.c_str());

						return outbuf;
					}
				}
				//如果有重复section，且前面的section找不到key，会继续从后面的section找，不break;
			}
		}
		if (NULL != default_val)
		{
			if (strlen(default_val) >= outbuflen)
				return NULL;
			strcpy(outbuf, default_val);
			return outbuf;
		}
		else
			return NULL;
	}
	int inifile::read_int(const char* section_name, const char* key_name, int default_val)
	{
		assert(m_bopen);

		char buf[32];
		if (read_string(section_name, key_name, NULL, buf, 32))
			return atoi(buf);
		else
			return default_val;
	}
	void inifile::save()
	{
		assert(m_bopen);
		if (!m_bchanged)
			return;
		if (m_path == "/noexist") {
			return;
		}

		m_bchanged = false;

		file32 file;
		file.open(m_path.c_str(), F32_RDWR | F32_TRUNC | F32_BINARY);
		if (!file.is_open())
			return;

		list<ini_section_t*>::iterator sit;
		list<ini_key_t>::iterator kit;
		string str;
		char buf[1024];
		ini_section_t *psec = NULL;
		for (sit = m_data_list.begin(); sit != m_data_list.end(); ++sit)
		{
			psec = *sit;
			sprintf(buf, "[%s]\r\n", psec->name.c_str());
			//str = "[" + psec->name + "]\r\n";
			file.write(buf, (int)strlen(buf));

			for (kit = psec->key_list.begin(); kit != psec->key_list.end(); ++kit)
			{
				str = (*kit).name + (*kit).seq + (*kit).value + "\r\n";
				file.write(str.c_str(), (int)str.length());
			}
		}
		file.close();
	}

	int WritePrivateProfileIntA(const char* szAppName, const char* szKeyName, int nValue, const char* szFileName)
	{
		inifile ini;
		if (0 == ini.open(szFileName))
		{
			return ini.write_int(szAppName, szKeyName, nValue);
		}
		return -1;
	}
#ifdef __GNUC__
	int GetPrivateProfileIntA(const char* szAppName, const char* szKeyName, int nDefault, const char* szFileName)
	{
		inifile ini;
		if (0 == ini.open(szFileName))
		{
			return ini.read_int(szAppName, szKeyName, nDefault);
		}
		return nDefault;
	}
	char* GetPrivateProfileStringA(const char* szAppName, const char* szKeyName, const char* szDefault, char* szOut, unsigned int nOutLen, const char* szFileName)
	{
		inifile ini;
		if (0 == ini.open(szFileName))
		{
			return ini.read_string(szAppName, szKeyName, szDefault, szOut, nOutLen);
		}
		if (NULL != szDefault)
		{
			if (strlen(szDefault) >= nOutLen)
				return NULL;
			strcpy(szOut, szDefault);
			return szOut;
		}
		else
			return NULL;
	}
	int WritePrivateProfileStringA(const char* szAppName, const char* szKeyName, const char* szValue, const char* szFileName)
	{
		inifile ini;
		if (0 == ini.open(szFileName))
		{
			return ini.write_string(szAppName, szKeyName, szValue);
		}
		return -1;
	}
#endif

}
