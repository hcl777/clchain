#include "signature.h"
#include "clsha3.h"
#include <iostream>

#include "secp.h"

/*
linux 版本直接使用原secp256k1编译版本,
不要使用本工程独立抽出的库.
使用原secp256k1编译的验签效率高很多.
*/

using namespace std;
namespace cryp
{
	namespace sig
	{
		int rand_key(std::string& prikey32, std::string& pubkey)
		{
			string key;
			while (1)
			{
				if (0 == secp_generate_key(prikey32, key))
				{
					if (key.at(0) == 3)
					{
						pubkey = key.substr(1);
						return 0;
					}
				}
			}
			return -1;
		}
		/*检查一对key是否正确*/
		bool check_key(const std::string& prikey32, const std::string& pubkey)
		{
			string s;
			cl::hash<32> h;
			h.rand();
			sign(prikey32, h, s);
			return verify(pubkey, s, h);
		}
		int sign(const std::string& prikey, const cl::clslice& data, std::string& out)
		{
			cl::hash<32> hash;
			cl::sha3_256(hash.data(), hash.size(), (uint8_t*)data.data(), data.size());
			return sign(prikey, hash, out);
		}
		int sign(const std::string& prikey, const cl::hash<32>& hash, std::string& out)
		{
			return secp_sign(prikey, hash.data(), out);
		}
		bool verify(const std::string& pubkey, const std::string& sig, const cl::hash<32>& hash)
		{
			string k;
			k.resize(pubkey.size() + 1);
			memcpy((char*)k.data() + 1, (char*)pubkey.data(), pubkey.size());
			k.at(0) = 3;
			return 0 == secp_verify_sign(k, sig, hash.data());
		}
		//std::string key_to_userid(const std::string& pubkey)
		//{
		//	cl::hash<32> h;
		//	cl::sha3_256(h.data(), 32, (uint8_t*)pubkey.data(), pubkey.size());
		//	return h.to_string();
		//}

		void test()
		{
			string sign;
			string s1, s2, s3, s4;
			string prikey, pubkey;
			cl::hash<32> hash;
			uint64_t t;
			while (1)
			{
				cl::msleep(1000);

				hash.rand();
				sig::rand_key(prikey, pubkey);
				sig::sign(prikey, hash, sign);

				t = cl::mtick();
				s4 = sig::verify(pubkey, sign, hash) ? "true" : "false";
				t = cl::mtick() - t;
				std::cout << "hash: " << hash.to_hexString() << endl;
				std::cout << "pubkey: " << cl::byte2hexs((uint8_t*)pubkey.data(), (int)pubkey.size(), s1) << endl;
				std::cout << "prikey: " << cl::byte2hexs((uint8_t*)prikey.data(), (int)prikey.size(), s2) << endl;
				std::cout << "sign: " << cl::byte2hexs((uint8_t*)sign.data(), (int)sign.size(), s3) << endl;
				std::cout << "sig::verify: " << s4 << " tick=" << t << endl << endl;
			}
		}
	}
}



