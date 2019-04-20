#pragma once

#include "incnet.h"
#include <string>
//#include "cl_basetypes.h"

using namespace std;
class HttpResponseHeader
{
public:
	HttpResponseHeader(void);
	~HttpResponseHeader(void);

	HttpResponseHeader(const string& header):m_sHeader(header) {}

public:
//Methods
  string GetField(const string& name);
  static string GetField(const string& head,const string& name);

  void    AddStatusCode(int nStatusCode);
  void    AddString(const string& sAdd);
  void    AddContentLength(int nSize);
  void    AddContentLength(long long nSize);
  void    AddContentType(const string& sMediaType);
  void    AddDate();
  void    AddLastModified();
  void    AddExpires();
  void    AddWWWBasicAuthenticate(const string& sRealm);
  void    AddLocation(const string& sLocation);
  void    AddServer(const string& sServer);
  void    AddMyAllowFields();
  bool    Send(SOCKET socket,int timeoutms);

public:
 string m_sHeader;
};
