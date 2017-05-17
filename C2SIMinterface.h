#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <condition_variable>
#include <thread>
#include <vrfcontrol/vrfRemoteController.h>
#include "textIf.h"

// xercesc
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/dom/DOMException.hpp"	
#include "xercesc/dom/DOMElement.hpp"

// probably most of these are not needed DOTO: figure this out  debugx
#include "xercesc/dom/DOMDocument.hpp"
#include "xercesc/dom/DOMNode.hpp"

#include "xercesc/dom/DOMNodeList.hpp"
#include "xercesc/dom/DOMXPathResult.hpp"

#include "xercesc/framework/StdOutFormatTarget.hpp"
#include "xercesc/framework/XMLFormatter.hpp"

#include "xercesc/framework/LocalFileFormatTarget.hpp"
#include "xercesc/util/XMLUni.hpp"
#include "xercesc/util/OutOfMemoryException.hpp"

//SAX parser
#include "xercesc/parsers/SAXParser.hpp"
#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/util/XMLString.hpp"

// Boost
#include <boost/asio.hpp>-

		class C2SIMinterface
		{
			// globals
			boost::asio::ip::tcp::iostream stream;
			std::string host = "10.2.10.30"; //debugx this should be a passed parameter
			std::string port = "61613";

		public:
			C2SIMinterface();
			~C2SIMinterface();
			static char* non_blocking_C2SIM_fgets(char* buffer, int bufferSize);
			static void readStomp(DtVrfRemoteController* controller, DtTextInterface* textIf);
			static std::string readAnXmlFile(std::string);
			static void pushCommandToQueue(char* command);
		};