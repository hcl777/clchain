#pragma once
#include <string>
#include "clhash.h"

/*
sig::rand_key : sig::rand_key
sig::sign
sig::verify
*/

namespace cryp
{
	namespace sig
	{
		int rand_key(std::string& prikey32, std::string& pubkey);
		bool check_key(const std::string& prikey32,const std::string& pubkey);
		int sign(const std::string& prikey, const cl::clslice& data, std::string& out);
		int sign(const std::string& prikey, const cl::hash<32>& hash, std::string& out);
		bool verify(const std::string& pubkey, const std::string& sig, const cl::hash<32>& hash);
		//将pubkey转为用户地址,暂直接使用pukkey做id
		//std::string key_to_userid(const std::string& pubkey); 

		void test();
	}
}

