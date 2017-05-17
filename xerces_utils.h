#pragma once
#include "xercesc/include/xercesc/util/PlatformUtils.hpp"
#include "xercesc/include/xercesc/framework/MemoryManager.hpp"
#include "xercesc/include/xercesc/util/PanicHandler.hpp"
#include "xercesc/include/xercesc/util/XMLString.hpp"
#include "xercesc/include/xercesc/dom/DOM.hpp"
#include "xercesc/include/xercesc/dom/DOMException.hpp"
#include "xercesc/include/xercesc/dom/DOMDocument.hpp"
#include "xercesc/include/xercesc/dom/DOMElement.hpp"
#include "xercesc/include/xercesc/dom/DOMNodeList.hpp"
#include "xercesc/include/xercesc/dom/DOMXPathResult.hpp"
#include "xercesc/include/xercesc/framework/StdOutFormatTarget.hpp"
#include "xercesc/include/xercesc/framework/XMLFormatter.hpp"
#include "xercesc/include/xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/include/xercesc/framework/MemBufFormatTarget.hpp"
#include "xercesc/include/xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/include/xercesc/framework/LocalFileFormatTarget.hpp"
#include "xercesc/include/xercesc/util/XMLUni.hpp"
#include "xercesc/include/xercesc/util/OutOfMemoryException.hpp"

#include <string>
#include <cstdlib>

#include <sstream>

#include "xercesc/include/xercesc/util/XercesDefs.hpp"
#include "xercesc/include/xercesc/sax/ErrorHandler.hpp"
#if defined(XERCES_NEW_IOSTREAMS)
#include <iostream>
#else
//#include <iostream.h> debugx
#endif

#include "xercesc/include/xercesc/dom/DOMErrorHandler.hpp"

XERCES_CPP_NAMESPACE_USE

using namespace std;

/************************
 * Function prototypes  *
 ************************/
int findNode(DOMDocument *domDoc, const DOMElement* baseNode, const XMLCh* xpath, DOMXPathResult*& result);

//string serializeDocument(DOMDocument *domDoc);
//XercesDOMParser *parseString(string &str);
