#include "bstream.h"
#include <string.h>

namespace cl
{

#define SS_OVERFLOW_READ_N_RETURN(n)    assert(m_rpos+n <= m_wpos);				  \
										if(n<0 || m_rpos+n > m_wpos){ m_state=-1; return m_state;} 


#define SS_OVERFLOW_WRITE_N_RETURN(n)   if(-1==check_resize(n)) return m_state;


	bstream::bstream(char endian/*=SS_LITTLE_ENDIAN*/)
	{
		//m_size = 4;
		//m_buf = new char[m_size];
		m_size = 0;
		m_buf = NULL;
		m_mynew = true;
		m_rpos = 0;
		m_wpos = 0;
		m_state = 0;
		m_endian = endian;
	}
	bstream::bstream(int buflen, char endian/*=SS_LITTLE_ENDIAN*/)
	{
		assert(buflen>0);
		m_size = buflen;
		m_buf = new char[m_size];
		m_mynew = true;
		m_rpos = 0;
		m_wpos = 0;
		m_state = 0;
		m_endian = endian;
	}
	bstream::bstream(char* buf, int buflen, int datalen/*=0*/, char endian/*=SS_LITTLE_ENDIAN*/)
	{
		assert(buf && datalen >= 0 && buflen >= datalen);
		m_buf = buf;
		m_size = buflen;
		m_rpos = 0;
		m_wpos = datalen;
		m_state = 0;
		m_endian = endian;
		m_mynew = false;
	}
	bstream::~bstream()
	{
		if (m_mynew && m_buf)
			delete[] m_buf;
	}

	bool bstream::little_endian()
	{
		union {
			int a;
			char b;
		}c;
		c.a = 1;
		return (c.b == 1);
	}
	void bstream::swap(void *arr, int len)
	{
		assert(len>1);
		char *ptr = (char*)arr;
		char tmp = 0;
		for (int i = 0; i<len / 2; ++i)
		{
			tmp = ptr[i];
			ptr[i] = ptr[len - 1 - i];
			ptr[len - 1 - i] = tmp;
		}
	}

	//h:host endian; b:big endian; l:little endian ;
	void bstream::htol(void *arr, int len)
	{
		if (!little_endian())
			swap(arr, len);
	}
	void bstream::ltoh(void *arr, int len)
	{
		if (!little_endian())
			swap(arr, len);
	}
	void bstream::htob(void *arr, int len)
	{
		if (little_endian())
			swap(arr, len);
	}
	void bstream::btoh(void *arr, int len)
	{
		if (little_endian())
			swap(arr, len);
	}

	short bstream::htol16(short val)
	{
		htol(&val, sizeof(short));
		return val;
	}
	short bstream::ltoh16(short val)
	{
		ltoh(&val, sizeof(short));
		return val;
	}
	short bstream::htob16(short val)
	{
		htob(&val, sizeof(short));
		return val;
	}
	short bstream::btoh16(short val)
	{
		btoh(&val, sizeof(short));
		return val;
	}

	int32 bstream::htol32(int32 val)
	{
		htol(&val, sizeof(int32));
		return val;
	}
	int32 bstream::ltoh32(int32 val)
	{
		ltoh(&val, sizeof(int32));
		return val;
	}
	int32 bstream::htob32(int32 val)
	{
		htob(&val, sizeof(int32));
		return val;
	}
	int32 bstream::btoh32(int32 val)
	{
		btoh(&val, sizeof(int32));
		return val;
	}

	int64 bstream::htol64(int64 val)
	{
		htol(&val, sizeof(int64));
		return val;
	}
	int64 bstream::ltoh64(int64 val)
	{
		ltoh(&val, sizeof(int64));
		return val;
	}
	int64 bstream::htob64(int64 val)
	{
		htob(&val, sizeof(int64));
		return val;
	}
	int64 bstream::btoh64(int64 val)
	{
		btoh(&val, sizeof(int64));
		return val;
	}


	int bstream::seekr(int pos)
	{
		if (0 != m_state)
			return m_state;
		if (pos<0 || pos>m_wpos)
			return -1;
		m_rpos = pos;
		return m_state;
	}
	int bstream::seekw(int pos)
	{
		if (0 != m_state)
			return m_state;
		if (pos<m_rpos || pos>m_size)
			return -1;
		m_wpos = pos;
		return m_state;
	}
	int bstream::skipr(int len)
	{
		SS_OVERFLOW_READ_N_RETURN(len)
			m_rpos += len;
		return m_state;
	}
	int bstream::skipw(int len)
	{
		SS_OVERFLOW_WRITE_N_RETURN(len)
			m_wpos += len;
		return m_state;
	}
	int bstream::read(void *arr, int len)
	{
		SS_OVERFLOW_READ_N_RETURN(len)
		if (0 == len) return m_state;
		memcpy(arr, m_buf + m_rpos, len);
		m_rpos += len;
		return m_state;
	}
	int bstream::write(const void *arr, int len)
	{
		SS_OVERFLOW_WRITE_N_RETURN(len)
		if (0 == len) return m_state;
		memcpy(m_buf + m_wpos, arr, len);
		m_wpos += len;
		return m_state;
	}
	int bstream::set_memery(int pos, void *arr, int len)
	{
		if (pos + len>m_size)
			return -1;
		memcpy(m_buf + pos, arr, len);
		return 0;
	}
	int bstream::get_memery(int pos, void *arr, int len)
	{
		if (pos + len>m_size)
			return -1;
		memcpy(arr, m_buf + pos, len);
		return 0;
	}

	int bstream::read_string(char* str, int maxsize)
	{
		SS_OVERFLOW_READ_N_RETURN(4)
			uint32 n = 0;
		*this >> n;
		if (n == 0)
		{
			str[0] = '\0';
			return m_state;
		}
		SS_OVERFLOW_READ_N_RETURN((int)n)
			if (n >= (uint32)maxsize)
			{
				skipr(n);
				str[0] = '\0';
				m_state = -1;
				return m_state;
			}
		this->read(str, n);
		str[n] = '\0';
		return m_state;
	}
	int bstream::write_string(const char* str)
	{
		uint32 n = 0;
		if (str) n = (uint32)strlen(str);
		SS_OVERFLOW_WRITE_N_RETURN((int)(n + 4))
			*this << n;
		if (n) this->write(str, n);
		return m_state;
	}

	int bstream::operator >> (char& val)
	{
		SS_OVERFLOW_READ_N_RETURN(1)
			val = m_buf[m_rpos++];
		return m_state;
	}
	int bstream::operator >> (uchar& val)
	{
		SS_OVERFLOW_READ_N_RETURN(1)
			val = (uchar)m_buf[m_rpos++];
		return m_state;
	}
	int bstream::operator >> (int16& val)
	{
		read(&val, sizeof(int16));
		mytoh(&val, sizeof(int16));
		return m_state;
	}
	int bstream::operator >> (uint16& val)
	{
		read(&val, sizeof(uint16));
		mytoh(&val, sizeof(uint16));
		return m_state;
	}
	int bstream::operator >> (int32& val)
	{
		read(&val, sizeof(int32));
		mytoh(&val, sizeof(int32));
		return m_state;
	}
	int bstream::operator >> (uint32& val)
	{
		read(&val, sizeof(uint32));
		mytoh(&val, sizeof(uint32));
		return m_state;
	}
	int bstream::operator >> (int64& val)
	{
		read(&val, sizeof(int64));
		mytoh(&val, sizeof(int64));
		return m_state;
	}
	int bstream::operator >> (uint64& val)
	{
		read(&val, sizeof(uint64));
		mytoh(&val, sizeof(uint64));
		return m_state;
	}
	int bstream::operator >> (float& val)
	{
		read(&val, sizeof(float)); //32¦Ë
		return m_state;
	}
	int bstream::operator >> (double& val)
	{
		read(&val, sizeof(double)); //64
		return m_state;
	}


	int bstream::operator << (char val)
	{
		SS_OVERFLOW_WRITE_N_RETURN(1)
			m_buf[m_wpos] = val;
		m_wpos++;
		return m_state;
	}
	int bstream::operator << (uchar val)
	{
		SS_OVERFLOW_WRITE_N_RETURN(1)
			m_buf[m_wpos] = (char)val;
		m_wpos++;
		return m_state;
	}
	int bstream::operator << (int16 val)
	{
		htomy(&val, sizeof(int16));
		write(&val, sizeof(int16));
		return m_state;
	}
	int bstream::operator << (uint16 val)
	{
		htomy(&val, sizeof(uint16));
		write(&val, sizeof(uint16));
		return m_state;
	}
	int bstream::operator << (int32 val)
	{
		htomy(&val, sizeof(int32));
		write(&val, sizeof(int32));
		return m_state;
	}
	int bstream::operator << (uint32 val)
	{
		htomy(&val, sizeof(uint32));
		write(&val, sizeof(uint32));
		return m_state;
	}
	int bstream::operator << (int64 val)
	{
		htomy(&val, sizeof(int64));
		write(&val, sizeof(int64));
		return m_state;
	}
	int bstream::operator << (uint64 val)
	{
		htomy(&val, sizeof(uint64));
		write(&val, sizeof(uint64));
		return m_state;
	}
	int bstream::operator << (float val)
	{
		write(&val, sizeof(float)); //32¦Ë
		return m_state;
	}
	int bstream::operator << (double val)
	{
		write(&val, sizeof(double)); //64
		return m_state;
	}


	void bstream::htomy(void *arr, int len)
	{
		if (SS_LITTLE_ENDIAN == m_endian)
		{
			if (!little_endian())
				swap(arr, len);
		}
		else if (SS_BIG_ENDIAN == m_endian)
		{
			if (little_endian())
				swap(arr, len);
		}
	}
	void bstream::mytoh(void *arr, int len)
	{
		htomy(arr, len);
	}
	int bstream::check_resize(int more)
	{
		if (-1 != m_state && m_wpos + more > m_size)
		{
			if (m_mynew)
			{
				int newsize = m_size * 2;
				if (newsize < m_wpos + more)
					newsize = m_wpos + more;
				char* newbuf = new char[newsize];
				if (!newbuf)
				{
					m_state = -1;
					return -1;
				}
				if (m_wpos>0)
				{
					memcpy(newbuf, m_buf, m_wpos);
				}
				if (m_buf)
					delete[] m_buf;
				m_buf = newbuf;
				m_size = newsize;
			}
			else
			{
				m_state = -1;
			}
		}
		return m_state;
	}


	int bstream::attach(char* buf, int buflen, int datalen/*=0*/)
	{
		if (!buf || !buflen)
			return -1;
		reset();
		m_buf = buf;
		m_size = buflen;
		m_wpos = datalen;
		return 0;
	}

	void bstream::reset()
	{
		if (m_mynew && m_buf)
		{
			delete[] m_buf;
		}
		m_buf = 0;
		m_mynew = false;
		m_size = 0;
		m_rpos = 0;
		m_wpos = 0;
		m_state = 0;
	}
	int operator << (bstream& ss, const std::string& s)
	{
		ss << (uint32_t)s.length();
		ss.write(s.c_str(),(int)s.length());
		return ss.ok();
	}
	int operator >> (bstream& ss, std::string& s)
	{
		uint32_t len = 0;
		if (0 == ss >> len && len>0)
		{
			s.resize(len);
			ss.read((void*)s.c_str(), len);
		}
		return ss.ok();
	}
	//slice
	int operator << (bstream& ss, const clslice& s)
	{
		ss << (uint32_t)s.size();
		ss.write(s.data(), (int)s.size());
		return ss.ok();
	}
	int operator >> (bstream& ss, clslice& s)
	{
		uint32_t len = 0;
		ss >> len;
		s.set(ss.read_ptr(), len);
		ss.skipr(len);
		return ss.ok();
	}
}

