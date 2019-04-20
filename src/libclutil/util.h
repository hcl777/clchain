#pragma once

#include <string>
#include <ctime>
#include "cltype.h"
#include "rfdelete.h"

CLBEGIN


	void debug_memleak();
	int chang_dir(const char* path);
	int _GetModuleFileName(const char* lpszModuleName, char* lpszModulePath, int cbModule);
	std::string get_module_path();
	std::string get_module_dir(); //尾带/
	std::string get_module_name();
	std::string& dir_format(std::string& dir);
	int create_dir(const std::string& dir);
	int create_dir_by_filepath(const std::string& filepath);
	//一般用于main()中的参数分析,返回str所在数据索引号
	int string_array_find(int argc, char** argv, const char* str);
	int get_scanf_pass(char *pass,int size);

	inline bool big_endian();
	inline char itoc(unsigned int i);
	std::string& byte2hexs(const uint8_t* from, int fromsize, std::string& out);
	std::string& byte2hexs(const std::string& from, std::string& out);
	uint8_t* hexs2byte(const std::string& from, uint8_t* out, int tosize);
	std::string& hexs2byte(const std::string& from, std::string& out);
	std::string b2h(const std::string& s);
	std::string h2b(const std::string& s);
	std::string randkey(int size);

	std::string get_string_index(const std::string& source, int index, const std::string& sp);
	int get_string_index_pos(const std::string& source, int index, const std::string& sp);
	int get_string_index_count(const std::string& source, const std::string& sp);
	std::string& string_trim(std::string& s);
	std::string substr_by_findlast(const std::string& s, char _Ch);

	int atoi(const std::string& _Str, int default_val);
	std::string itoa(int i);
	long long atoll(const char* _Str, int nullval=0);

	std::string time_to_time_string(const std::time_t& _Time);
	std::string time_to_date_string(const std::time_t& _Time);
	std::string time_to_datetime_string(const std::time_t& _Time);
	std::string time_to_datetime_string2(const std::time_t& _Time);
	std::string time_to_datetimeT_string(const std::time_t& _Time); //ISO datetime标准格式

	std::string url_get_param(const std::string& params, const std::string& name);

	int read_file_to_str(const std::string& path, std::string& out);
	int write_file_from_str(const std::string& path,const std::string& in);


	int write_log(const char *strline,const char *path);
	int write_logt(const char *path, int maxsize, const char* fomat, ...);
CLEND
