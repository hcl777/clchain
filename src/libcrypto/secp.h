#pragma once


#include "cltype.h"

namespace cryp
{
	int secp_init();
	void secp_fini();

	//������key32����ǩ��(��prikey). return: 0=ok
	int secp_generate_key(std::string& prikey32, std::string& pubkey);
	int secp_sign(const std::string& prikey32, const uint8_t msg32[32], std::string& out64);
	int secp_verify_sign(const std::string& pubkey, const std::string& sig64, const uint8_t msg32[32]);

}


