#pragma once
#include <iostream>
#include <fstream>
#include <string>

#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/sax/AttributeList.hpp"

using namespace xercesc;

class C2SIMHandler : public HandlerBase
{
public:
	C2SIMHandler();
	~C2SIMHandler();
	void startC2SIMParse(const char* newRootTag);
	void returnData(char*, char*, char*, char*);
	void startElement(const XMLCh* const, AttributeList&);
	void fatalError(const SAXParseException&);
	void startDocument();
	void endDocument();
	void characters(const XMLCh* const, const XMLSize_t);
	void endElement(const XMLCh* const name);
	void setRootTag(const char*);
private:
	void copyXMLCh(char* output, const XMLCh* const input);
};