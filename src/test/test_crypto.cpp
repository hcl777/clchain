#include <signature.h>
#include <iostream>
#include <assert.h>

/*
function:
测试自动生成公私钥并打印hexstring
*/
int test_generate()
{
    std::string prikey,pubkey;
    cryp::sign_rand_key(pubkey, prikey);
    std::cout << "Private_key_hex:"<< std::endl << prikey << std::endl;
    std::cout << "Public_key_hex:"<< std::endl << pubkey << std::endl;
    return 0;
}

/*
function:
测试自动生成公私钥并进行签名验签
*/
int test_sign_verify()
{
    std::string prikey,pubkey;
    cryp::sign_rand_key(pubkey, prikey);
    std::cout << "Private_key_hex:"<< std::endl << prikey << std::endl;
    std::cout << "Public_key_hex:"<< std::endl << pubkey << std::endl;
    
    std::string signature;
    cl::hash<32> message;
    message = "9e5755ec2f328cc8635a55415d0e9a09c2b6f2c9b0343c945fbbfe08247a4cbe";
    cryp::sign_encrypt(prikey,message,signature);
    std::cout << "Signature_hex" << std::endl << signature << std::endl;
    int ret;
    ret = cryp::sign_check(pubkey,signature,message);
    std::cout << "Verify_result" << std::endl << ret << std::endl;
    return 0;
}

int main()
{
    test_generate();
    test_sign_verify();
}