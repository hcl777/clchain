#pragma once
#include "httpcinfo.h"
#include "slice.h"

CLBEGIN
namespace httpc
{

	int post(const string& url, const cl::clslice& body, string& rspbody);
	int get(const string& url, string& rspbody);
}

CLEND

