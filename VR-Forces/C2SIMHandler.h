/*----------------------------------------------------------------*
|     Copyright 2017 Networking and Simulation Laboratory         |
|         George Mason University, Fairfax, Virginia              |
|                                                                 |
| Permission to use, copy, modify, and distribute this            |
| software and its documentation for academic purposes is hereby  |
| granted without fee, provided that the above copyright notice   |
| and this permission appear in all copies and in supporting      |
| documentation, and that the name of George Mason University     |
| not be used in advertising or publicity pertaining to           |
| distribution of the software without specific, written prior    |
| permission. GMU makes no representations about the suitability  |
| of this software for any purposes.  It is provided "AS IS"      |
| without express or implied warranties.  All risk associated     |
| with use of this software is expressly assumed by the user.     |
*-----------------------------------------------------------------*/
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
	void startC2SIMParse(std::string newRootTag);
	bool returnData(char*, char*, char*, char*, char*, char*, char*, 
		char*, char*, char*, char*, char*, char*, char*, char*, char*);
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