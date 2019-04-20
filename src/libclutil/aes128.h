#pragma once
#include <string>
/*16×Ö½Ú³¤¶Èkey*/
namespace cl
{
	class aes128
	{
	public:
		aes128() {}
		~aes128() {}

		static std::string randkey();
		static int encrypt_s(const std::string& key, const std::string& in, std::string& out);
		static int decrypt_s(const std::string& key, const std::string& in, std::string& out);
	private:
		static std::string to_key128(const std::string& key);

	};

}

