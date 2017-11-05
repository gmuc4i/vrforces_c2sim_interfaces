#pragma once
#include "xercesc/util/PlatformUtils.hpp"
#include "xercesc/framework/MemoryManager.hpp"
#include "xercesc/util/PanicHandler.hpp"
#include "xercesc/util/XMLString.hpp"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/dom/DOMException.hpp"
#include "xercesc/dom/DOMDocument.hpp"
#include "xercesc/dom/DOMElement.hpp"
#include "xercesc/dom/DOMNodeList.hpp"
#include "xercesc/dom/DOMXPathResult.hpp"
#include "xercesc/framework/StdOutFormatTarget.hpp"
#include "xercesc/framework/XMLFormatter.hpp"
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/framework/MemBufFormatTarget.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/framework/LocalFileFormatTarget.hpp"
#include "xercesc/util/XMLUni.hpp"
#include "xercesc/util/OutOfMemoryException.hpp"

#include <string>
#include <cstdlib>

#include <sstream>

#include "xercesc/util/XercesDefs.hpp"
#include "xercesc/sax/ErrorHandler.hpp"
#if defined(XERCES_NEW_IOSTREAMS)
#include <iostream>
#endif

#include "xercesc/dom/DOMErrorHandler.hpp"

XERCES_CPP_NAMESPACE_USE

using namespace std;

/************************
 * Function prototypes  *
 ************************/
int findNode(xercesc_3_1::DOMDocument *domDoc, const DOMElement* baseNode, const XMLCh* xpath, DOMXPathResult*& result);

//string serializeDocument(DOMDocument *domDoc);
//XercesDOMParser *parseString(string &str);
