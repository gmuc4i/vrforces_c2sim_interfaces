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
/*
To run demo on Sandbox:
1. start VR Forces; at Config panel clock Launch;
on Startup panel run Bogaland8; wait until icon shows on map
2. run remoteControlTriangle/remoteControlDIS; wait for READY FOR C2SIM ORDERS
3. send an order by C2SIM/RESTclient/RES-moveOrder.bat
4. icon should move on route from order and reports should be emitted
5. to see reports run C2SIM/STOMPclient/STOMP.bat or run BMLC2GUI
*/
#include "C2SIMinterface.h"
#include <queue>
#include <vlutil/vlProcessControl.h>
#include "textIf.h"
#include <vrfcontrol/vrfRemoteController.h>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <tchar.h>
#include <windows.h>
#include <math.h>
#include "xerces_utils.h"
#include "C2SIMOrderHandler.h"
#include "stomp-util.h"
#include "StompClient.h"
#include "RestClient.h"

typedef struct StompFrame_t { // copied from StompClient.h
	/**
	* The <code>operation</code> is the protocol operation being sent.
	*/
	std::string operation;
	/**
	* The <code>properties</code> is a set of key-value pairs contained within the
	* frame.  On similar protocols, these are also called <pre>headers</pre>.
	*/
	std::map<std::string, std::string> properties;
	/**
	* The <code>message</code> is the message body.
	*/
	std::stringstream message;
} StompFrame;

using namespace xercesc;

// globals
boost::asio::ip::tcp::iostream stompStream;
static int timesCalled = 0;
DtVrfRemoteController* cs2sim_controller;
SAXParser* parser;
C2SIMOrderHandler* c2simOrderHandler;
ErrorHandler* errHandler;
std::string stompServerAddress;
static string vrfTerrain;
const static double degreesToRadians = 57.2957795131L;

// constructor
C2SIMinterface::C2SIMinterface(
	DtVrfRemoteController* controller,
	std::string serverAddressRef)
{
	// pick up parameters from main()
	stompServerAddress = serverAddressRef;
	std::cout << "C2SIMinterface stompServerAddress:" << stompServerAddress << "\n";

	// initialize Xerces
	try {
		XMLPlatformUtils::Initialize();
	}
	catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		std::cout << "error during xeresc initialization:\n" << message << "\n";
		XMLString::release(&message);
		return;
	}

	// setup parser and handler
	parser = new SAXParser();
	parser->setDoNamespaces(false);
	parser->setDoSchema(false);
	c2simOrderHandler = new C2SIMOrderHandler();
	errHandler = (ErrorHandler*)c2simOrderHandler;
	parser->setDocumentHandler(c2simOrderHandler);
	parser->setErrorHandler(errHandler);

	// connect to STOMP server
	try {
		// Establish the connection and associate it with StompClient
		std::stringstream buf;
		buf << stompPort;
		stompStream.connect(stompServerAddress.c_str(), buf.str());
		if (stompStream) {
			client = new mee::stomp::StompClient(stompStream);
			client->connect(false, "", "");
			client->subscribe("/topic/BML", (mee::stomp::AcknowledgeMode)1, "COMMENT");
		}
		else {
			std::cerr
				<< stompServerAddress << ":" << stompPort
				<< std::endl;
		}
	}
	catch (mee::stomp::StompException e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	catch (std::exception &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "An unexpected exception has occured." << std::endl;
	}

}// end constructor


 // destructor
C2SIMinterface::~C2SIMinterface() {
	delete parser;
	delete c2simOrderHandler;
	client->disconnect();
	XMLPlatformUtils::Terminate();
}

// coordinate conversions
// GDC: lat/lon in degrees; alt in meters
// geocentric: x/y/z in meters from center of earth
// NOTE: VRForces uses radian angles; we convert here
std::string doubleToString(double d) {
	std::ostringstream oss;
	oss << d;
	return oss.str();
}
double stringToDouble(std::string s) {
	double d = atof(s.c_str());
	return d;
}
void C2SIMinterface::geodeticToGeocentric(char* lat, char* lon, char* alt,
	std::string &x, std::string &y, std::string &z) {

	// convert to geocentric
	DtGeodeticCoord geod(
		stod(lat) / degreesToRadians,
		stod(lon) / degreesToRadians,
		stod(alt));
	DtVector geoc = geod.geocentric();

	// move into output strings
	x = doubleToString(geoc.x());
	y = doubleToString(geoc.y());
	z = doubleToString(geoc.z());
}
void C2SIMinterface::geocentricToGeodetic(std::string x, std::string y, std::string z,
	std::string &lat, std::string &lon, std::string &alt) {

	// convert to geodetic
	DtVector geoc(atof(x.c_str()), atof(y.c_str()), atof(z.c_str()));
	DtGeodeticCoord geod;
	geod.setGeocentric(geoc);

	// move into output strings
	lat = doubleToString(geod.lat()*degreesToRadians);
	lon = doubleToString(geod.lon()*degreesToRadians);
	alt = doubleToString(geod.alt());
}

 // read an XML file and return it as wstring
string C2SIMinterface::readAnXmlFile(string filename) {

	// open file, get count of lines, and start reader
	ifstream xmlFile(filename);
	if (!xmlFile.is_open()) {
		cout << "can't open input file " << filename << "\n";
		return NULL;
	}

	// read the file into string
	string xmlBuffer, xmlLine;
	while (getline(xmlFile, xmlLine)) {
		xmlBuffer.append(xmlLine);
	}
	xmlFile.close();
	return xmlBuffer;

}// end readAnXmlFile()


 // write an XML file given name and wstring
void C2SIMinterface::writeAnXmlFile(char* filename, string content) {

	// open file for output
	std::remove(filename);
	std::ofstream xmlFile(filename, ios::out);
	if (!xmlFile.is_open()) {
		cout << "can't open output file " << filename << "\n";
		return;
	}

	// write the file from outputFrame message content
	xmlFile << content;
	xmlFile.close();

}// end writeAnXmlFile()


 // thread to listen for incoming STOMP message; parse the message,
 // and use the result to send a command to VRForces
void C2SIMinterface::readStomp(DtTextInterface* textIf, C2SIMinterface* c2simInterface)
{
	HRESULT hr = CoInitialize(NULL);
	mee::stomp::StompFrame inputFrame;
	mee::stomp::StompClient stompClient(stompStream);

	if (SUCCEEDED(hr)) {

		std::cout << "READY FOR C2SIM ORDERS\n";

		char testXml[] = "C:\\temp\\holdXml.xml";

		// loop reading XML Orders, parsing them into VR-Forces commands
		// and pushing to command queue
		while (true) {
			
			// read a STOMP message and write it to temp file
			stompClient.receiveFrame(inputFrame);
			C2SIMinterface::writeAnXmlFile(testXml, inputFrame.message.str());
			
			// use SAX to parse XML 
			c2simOrderHandler->C2SIMOrderHandler::startC2SIMParse();
			try {
				parser->parse(testXml);
			}
			catch (const XMLException& toCatch) {
				char* message = XMLString::transcode(toCatch.getMessage());
				cout << "Exception message is: \n" << message << "\n";
				XMLString::release(&message);
			}
			catch (const SAXParseException& toCatch) {
				char* message = XMLString::transcode(toCatch.getMessage());
				cout << "Exception message is: \n" << message << "\n";
				XMLString::release(&message);
			}
			catch (...) {
				cout << "unexpected Exception in SAX parsing\n";
			}

			// check that we parsed something; if not go to next order
			if (!c2simOrderHandler->C2SIMOrderHandler::orderRootTagFound())continue;
			cout << "Completed parse of file:" << testXml << "\n";

			// extract parsed values
			char* taskersIntent = new char[MAXCHARARRAY];
			char* dateTime = new char[MAXCHARARRAY];
			char* unitId = new char[MAXCHARARRAY];
			double x[MAXPOINTS]; // geocentric coordinates
			double y[MAXPOINTS];
			double z[MAXPOINTS];

			// parse out order elements

			// get taskersIntent
			taskersIntent = c2simOrderHandler->C2SIMOrderHandler::getTaskersIntent();

			// get dateTime
			dateTime = c2simOrderHandler->C2SIMOrderHandler::getDateTime();

			// get arrays of geocentric point coordiantes
			int numberOfPoints =
				c2simOrderHandler->getRoutePoints(x, y, z);

			// get unitID
			unitId = c2simOrderHandler->C2SIMOrderHandler::getUnitId();

			cout << "C2SIMInterface got order unitID:" << unitId << " dateTime:" << dateTime << " intent:" << taskersIntent << "\n";

			// misc parameters
			string taskersIntentString = taskersIntent;
			string dateTimeString = dateTime;
			string unitIDString = unitId;

			// check for quit command
			if (taskersIntentString == "QUIT")break;
			std::cout << "starting to issue VRF commands\n";

			// previously we generated a "new" command at this point
			// now we're expecting the user to do that on VR-Forces GUI
			// during startup, giving more flexibility to Sandbox users

				// make a tank object to run on it, via task queues
				DtVector vec(x[0], y[0], z[0]);
				textIf->controller()->createEntity(
					DtTextInterface::vrfObjectCreatedCb, (void*)"tank",
					DtEntityType(1, 1, 225, 1, 1, 3, 0), vec,
					DtForceFriendly, 90.0, unitId);

				// create waypoints for all remaining numberOfPoints
				string pointNames[MAXPOINTS];
				for (int pointNumber = 1; pointNumber < numberOfPoints; ++pointNumber) {

					// combine coordinates to a DtVector
					DtVector vec(x[pointNumber], y[pointNumber], z[pointNumber]);

					// name a point name ending in i (up to 99)
					std::ostringstream oss;
					oss << "Pt " << pointNumber;
					pointNames[pointNumber] = oss.str();
					
					// create a point with these properties
					textIf->controller()->
						createWaypoint(
							DtTextInterface::vrfObjectCreatedCb, 
							(void*)"waypoint", 
							vec,
							pointNames[pointNumber]);
				}

				// wait a little bit 
				DtSleep(1.);//debug (is there a race here?)
				
				// run the tank to each waypoint and wait until it gets there
				for (int pointNumber = 1; pointNumber < numberOfPoints; ++pointNumber) {
					textIf->controller()->moveToWaypoint(unitId, pointNames[pointNumber]);
					textIf->controller()->run();
					while(textIf->getTaskNumberCompleted() < pointNumber)
					  DtSleep(.1);
				}
				
				// next: extract other parameters from order and use them to
				// create VR-Forces commands for a range of IBML09 'what' code Orders, bypassing
				// the DtTextInterface by calling for example (textIf.cpp line 895) 
				// a->moveToPoint(str) in place of text command "moveToPoint".
				// Also figure out how to fix the compile message that says version not recognized
			
		}// end while(true)

		// tell VRForces to shutdown
		textIf->setTimeToQuit(true);
		cout << "C2SIM VRF interface quitting\n";

	}// end readStomp()

}// end class C2SIMInterface
