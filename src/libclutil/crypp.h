#pragma once

#include <string>
//ʹ��cryptopp ʵ�ֵ�һЩ��װ

/*16�ֽڳ���key*/
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


