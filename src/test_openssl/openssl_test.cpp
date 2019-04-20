#include "openssl_test.h"

#ifdef _WIN32
#pragma comment(lib,"D:\\source\\openssl\\lib\\libcrypto.lib")
#pragma comment(lib,"D:\\source\\openssl\\lib\\libssl.lib")
#endif

#include <stdio.h>
#include <string.h>
//#include "cl_util.h"

#include "openssl/evp.h"

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>
 

void ossl_test_hash256()
{
	
	const char msg[] = "hello world!";
	unsigned char dgst[EVP_MAX_MD_SIZE];
	unsigned int dgst_len = EVP_MAX_MD_SIZE;
	EVP_MD_CTX *ctx;

	//hash����
	//����1�����������
	ctx = EVP_MD_CTX_new();
	if(!EVP_DigestInit_ex(ctx,EVP_sha256(),NULL))
		goto fail;
	if(!EVP_DigestUpdate(ctx,(const void*)msg,strlen(msg)))
		goto fail;
	if(!EVP_DigestFinal_ex(ctx,dgst,&dgst_len))
		goto fail;
	EVP_MD_CTX_free(ctx);

	//
	
	//printf("sha256: %s\n",cl_util::hex2str_x(dgst,dgst_len).c_str());

	//����2��
	unsigned char dgst2[EVP_MAX_MD_SIZE];
	EVP_Digest(msg,strlen(msg),dgst2,&dgst_len,EVP_sha256(),NULL);
	//printf("sha256: %s\n",cl_util::hex2str_x(dgst2,dgst_len).c_str());
	return;
fail:
	printf(" digest fail \n");
}
int ossl_test_ecdsa1()
{
	EC_KEY *key_tmp = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	if(NULL==key_tmp)
		goto fail;

	EC_KEY_generate_key(key_tmp);

	return 0;
fail:
	printf("ecdsa fail.\n");
	return -1;
}

int ossl_test_ecdsa()
{
      EC_KEY                *key1,*key2;
      const EC_POINT            *pubkey1,*pubkey2;
      EC_GROUP           *group1,*group2;
      unsigned int                         ret,nid,size,i,sig_len;
      unsigned char*signature,digest[20];
      BIO                      *berr;
      EC_builtin_curve    *curves;
      int                                crv_len;
      char               shareKey1[128],shareKey2[128];
      int                         len1,len2;
 
      /* ����EC_KEY���ݽṹ */
      key1=EC_KEY_new();
      if(key1==NULL)
      {
             printf("EC_KEY_new err!\n");
             return -1;
      }
      key2=EC_KEY_new();
      if(key2==NULL)
      {
             printf("EC_KEY_new err!\n");
             return -1;
      }
      /* ��ȡʵ�ֵ���Բ���߸��� */
      crv_len = EC_get_builtin_curves(NULL, 0);
      curves = (EC_builtin_curve *)malloc(sizeof(EC_builtin_curve) * crv_len);
      /* ��ȡ��Բ�����б� */
      EC_get_builtin_curves(curves, crv_len);
      /*
      nid=curves[0].nid;���д���ԭ������Կ̫��
      */
      /* ѡȡһ����Բ���� */
      nid=curves[25].nid;
      /* ����ѡ�����Բ����������Կ����group */
      group1=EC_GROUP_new_by_curve_name(nid);
      if(group1==NULL)
      {
             printf("EC_GROUP_new_by_curve_name err!\n");
             return -1;
      }
      group2=EC_GROUP_new_by_curve_name(nid);
      if(group1==NULL)
      {
             printf("EC_GROUP_new_by_curve_name err!\n");
             return -1;
      }
      /* ������Կ���� */
      ret=EC_KEY_set_group(key1,group1);
      if(ret!=1)
      {
             printf("EC_KEY_set_group err.\n");
             return -1;
      }
      ret=EC_KEY_set_group(key2,group2);
      if(ret!=1)
      {
             printf("EC_KEY_set_group err.\n");
             return -1;
      }
      /* ������Կ */
      ret=EC_KEY_generate_key(key1);
      if(ret!=1)
      {
             printf("EC_KEY_generate_key err.\n");
             return -1;
      }
      ret=EC_KEY_generate_key(key2);
      if(ret!=1)
      {
             printf("EC_KEY_generate_key err.\n");
             return -1;
      }
      /* �����Կ */
      ret=EC_KEY_check_key(key1);
      if(ret!=1)
      {
             printf("check key err.\n");
             return -1;
      }
      /* ��ȡ��Կ��С */
      size=ECDSA_size(key1);
      printf("size %d \n",size);
      for(i=0;i<20;i++)
             memset(&digest[i],i+1,1);
      signature= (unsigned char*)malloc(size);
      ERR_load_crypto_strings();
      berr=BIO_new(BIO_s_file());
      //BIO_set_fp(berr,stdout,BIO_NOCLOSE);
      /* ǩ�����ݣ�����δ��ժҪ���ɽ�digest�е����ݿ�����sha1ժҪ��� */
      ret=ECDSA_sign(0,digest,20,signature,&sig_len,key1);
      if(ret!=1)
      {
             ERR_print_errors(berr);
             printf("sign err!\n");
             return -1;
      }
      /* ��֤ǩ�� */
      ret=ECDSA_verify(0,digest,20,signature,sig_len,key1);
      if(ret!=1)
      {
             ERR_print_errors(berr);
             printf("ECDSA_verify err!\n");
             return -1;
      }
      /* ��ȡ�Է���Կ������ֱ������ */
      pubkey2 = EC_KEY_get0_public_key(key2);
      /* ����һ���Ĺ�����Կ */
      len1=ECDH_compute_key(shareKey1, 128, pubkey2, key1, NULL);
      pubkey1 = EC_KEY_get0_public_key(key1);
      /* ������һ��������Կ */
      len2=ECDH_compute_key(shareKey2, 128, pubkey1, key2, NULL);
      if(len1!=len2)
      {
             printf("err\n");
      }
      else
      {
             ret=memcmp(shareKey1,shareKey2,len1);
             if(ret==0)
                    printf("���ɹ�����Կ�ɹ�\n");
             else
                    printf("���ɹ�����Կʧ��\n");
      }
      printf("test ok!\n");
      BIO_free(berr);
      EC_KEY_free(key1);
      EC_KEY_free(key2);
      free(signature);
      free(curves);
      return 0;
}