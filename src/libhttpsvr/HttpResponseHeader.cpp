
#include "HttpResponseHeader.h"
#include "net.h"
#include <ctime>

#ifdef _MSC_VER
// 4482 4267 4018 4800 4311 4312 4102
#pragma warning(disable:4996)
#endif

HttpResponseHeader::HttpResponseHeader(void)
{
}

HttpResponseHeader::~HttpResponseHeader(void)
{
}
string HttpResponseHeader::GetField(const string& name)
{
	return GetField(m_sHeader,name);
}
string HttpResponseHeader::GetField(const string& head,const string& name)
{
	int pos1,pos2;
	pos1 = (int)head.find(name);
	if(pos1<0)
		return "";
	pos2 = (int)head.find("\r\n",pos1);
	if(pos2>0)
		return head.substr(pos1+name.length(),pos2-pos1-name.length());
	else
		return head.substr(pos1+name.length());
}
void HttpResponseHeader::AddStatusCode(int nStatusCode)
{
	string sLine;
	switch (nStatusCode)
	{
		case 200: sLine = "HTTP/1.0 200 OK\r\n";                    break;
		case 201: sLine = "HTTP/1.0 201 Created\r\n";               break;
		case 202: sLine = "HTTP/1.0 202 Accepted\r\n";              break;
		case 204: sLine = "HTTP/1.0 204 No Content\r\n";            break;
		case 300: sLine = "HTTP/1.0 300 Multiple Choices\r\n";      break;
		case 301: sLine = "HTTP/1.0 301 Moved Permanently\r\n";     break;
		case 302: sLine = "HTTP/1.0 302 Moved Temporarily\r\n";     break;
		case 304: sLine = "HTTP/1.0 304 Not Modified\r\n";          break;
		case 400: sLine = "HTTP/1.0 400 Bad Request\r\n";           break;
		case 401: sLine = "HTTP/1.0 401 Unauthorized\r\n";          break;
		case 403: sLine = "HTTP/1.0 403 Forbidden\r\n";             break;
		case 404: sLine = "HTTP/1.0 404 Not Found\r\n";             break;
		case 406: sLine = "HTTP/1.0 406 Not Acceptable\r\n";        break;
		case 500: sLine = "HTTP/1.0 500 Internal Server Error\r\n"; break;
		case 501: sLine = "HTTP/1.0 501 Not Implemented\r\n";       break;
		case 502: sLine = "HTTP/1.0 502 Bad Gateway\r\n";           break;
		case 503: sLine = "HTTP/1.0 503 Service Unavailable\r\n";   break;
		default: 
			char buf[128];
			sprintf(buf,"HTTP/1.0 %d\r\n",nStatusCode);
			sLine = buf;      
			break;
	}
	m_sHeader += sLine;
}
void HttpResponseHeader::AddString(const string& sAdd)
{
	m_sHeader += sAdd;
}
void HttpResponseHeader::AddContentLength(int nSize)
{
	char buf[128];
	sprintf(buf,"Content-Length: %d\r\n",nSize);
	m_sHeader += buf;
}
void HttpResponseHeader::AddContentLength(long long nSize)
{
	char buf[128];
	sprintf(buf,"Content-Length: %lld\r\n",nSize);
	m_sHeader += buf;
}

void HttpResponseHeader::AddContentType(const string& sMediaType)
{
	char buf[128];
	sprintf(buf,"Content-Type: %s\r\n",sMediaType.c_str());
	m_sHeader += buf;
}


void HttpResponseHeader::AddMyAllowFields()
{
  string sLine = "Allow: GET, HEAD, DELETE\r\n";
  m_sHeader += sLine;
}

void HttpResponseHeader::AddDate()
{
	time_t t = std::time(NULL);
  char buf[1024];
  sprintf(buf,"Date: %s GMT\r\n", std::ctime(&t));
  m_sHeader += buf;
}

void HttpResponseHeader::AddLastModified()
{
  time_t t = std::time(NULL);
  char buf[1024];
  sprintf(buf, "Last-Modified: %s GMT\r\n", std::ctime(&t));
  m_sHeader += buf;
}

void HttpResponseHeader::AddExpires()
{
	time_t t = std::time(NULL);
	char buf[1024];
	sprintf(buf, "Expires: %s GMT\r\n", std::ctime(&t));
	m_sHeader += buf;
}

void HttpResponseHeader::AddWWWBasicAuthenticate(const string& sRealm)
{
	char buf[1024];
	sprintf(buf,"WWW-Authenticate: Basic realm=%s\r\n",sRealm.c_str());
	m_sHeader += buf;
}

void HttpResponseHeader::AddLocation(const string& sLocation)
{
	char buf[1024];
	sprintf(buf,"Location: %s\r\n",sLocation.c_str());
	m_sHeader += buf;
}

void HttpResponseHeader::AddServer(const string& sServer)
{
    char buf[1024];
	sprintf(buf,"Server: %s\r\n",sServer.c_str());
	m_sHeader += buf;
}

bool HttpResponseHeader::Send(SOCKET socket,int timeoutms)
{
	//For correct operation of the T2A macro, see MFC Tech Note 59
	//USES_CONVERSION;
	if(m_sHeader.length()<=0)
		return false;
  m_sHeader += "\r\n";

  return 0==cl::net::sock_select_send_n((int)socket,m_sHeader.c_str(), (int)m_sHeader.length(),timeoutms);
  //return send(socket,m_sHeader.c_str(), (int)m_sHeader.length(),0);
}
