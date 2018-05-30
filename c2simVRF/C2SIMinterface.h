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
#include <condition_variable>
#include <thread>
#include <vrfcontrol/vrfRemoteController.h>
#include "textIf.h"
#include "C2SIMxmlHandler.h"

// xercesc
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/dom/DOMException.hpp"	
#include "xercesc/dom/DOMElement.hpp"
#include "xercesc/util/OutOfMemoryException.hpp"

// SAX parser
#include "xercesc/parsers/SAXParser.hpp"
#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/util/XMLString.hpp"

// Boost
#include <boost/asio.hpp>

// STOMP & REST
#include "stomp-util.h"
#include "StompClient.h"
#include "RestClient.h"

class C2SIMinterface
{
public:
	C2SIMinterface(
		DtVrfRemoteController* controller,
		std::string serverAddressRef,
		std::string stompPortRef,
		bool useIbml);
	~C2SIMinterface();

	// coordinate conversions
	static void convertCoordinates(
		std::string charX, std::string charY, std::string charZ,
		double &x, double &y, double &z);
	static void geodeticToGeocentric(char* lat, char* lon, char* elMsl,
		std::string &x, std::string &y, std::string &z);
	static void geocentricToGeodetic(std::string x, std::string y, std::string z,
		std::string &lat, std::string &lon, std::string &elMsl);
	static char* non_blocking_C2SIM_fgets(char* buffer, int bufferSize);
	static void readStomp(
		DtTextInterface* textIf, 
		C2SIMinterface* c2simInterface, 
		bool skipInitialize,
		std::string clientId);
	static std::string readAnXmlFile(std::string contents);
	static void writeAnXmlFile(char* filename, std::string content);
	static boolean isNewObject(std::string objectName);
	static void C2SIMinterface::executeTask(
		std::string taskId,
		DtTextInterface* textIf,
		bool skipInitialize,
		Task* thisTask,
		int numberOfUnits,
		Unit* units);
};