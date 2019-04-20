#pragma once
#include <string>
#include <map>

class HttpContentType
{
public:
	HttpContentType(void);
	~HttpContentType(void);

	std::string get_ct_name(const std::string& key);
	std::string operator [](const std::string& key);

private:
	std::map<std::string, std::string> m_ct_map;
};

extern HttpContentType http_content_type;
