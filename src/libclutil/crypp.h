#pragma once

#include <string>
//使用cryptopp 实现的一些包装

/*16字节长度key*/
namespace cl
{
	class aescbc
	{
	public:
		aescbc();
		~aescbc();

		static std::string randkey(size_t len=16);
		static int encrypt_s(const std::string& key, const std::string& in, std::string& out);
		static int decrypt_s(const std::string& key, const std::string& in, std::string& out);
	private:
		static std::string to_keylen(const std::string& key,size_t len);
	};
}


