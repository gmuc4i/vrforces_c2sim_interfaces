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
#include "C2SIMinterface.h"
#include <queue>
#include <vlutil/vlProcessControl.h>
#include "textIf.h"
#include <vrfcontrol/vrfRemoteController.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <tchar.h>
#include <windows.h>
#include <math.h>
#include "xerces_utils.h"
#include "C2SIMHandler.h"
#include "stomp-util.h"
#include "StompClient.h"
#include "RestClient.h"
#include <matrix/geodeticCoord.h>

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
std::queue<char*> commandQueue;
static int timesCalled = 0;
DtVrfRemoteController* cs2sim_controller;
SAXParser* parser;
C2SIMHandler* c2simHandler;
ErrorHandler* errHandler;
std::string stompServerAddress;
static char rootElement[100];


// constructor
C2SIMinterface::C2SIMinterface(
	DtVrfRemoteController* controller, std::string serverAddressRef, char* orderXmlRootElement)
{	
	// pick up command-line arguments from main()
	cs2sim_controller = controller;
	stompServerAddress = serverAddressRef;
	std::strncpy(rootElement, orderXmlRootElement, 100);
	std::cout << "C2SIMinterface stompServerAddress:" << stompServerAddress << "\n";
	std::cout << "C2SIMinterface orderXmlRootElement:" << rootElement << "\n";

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
	c2simHandler = new C2SIMHandler();
	errHandler = (ErrorHandler*)c2simHandler;
	parser->setDocumentHandler(c2simHandler);
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
		} else {
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
C2SIMinterface::~C2SIMinterface( ){
	delete parser;
	delete c2simHandler;
	client->disconnect();
	XMLPlatformUtils::Terminate();
}

// coordinate conversions
// GDC: lat/lon in degrees; alt in meters
// geocentric: x/y/z in meters from center of earth
// NOTE: VRFOrces uses radian angles; we convert here
double degreesToRadians = 57.2957795131L;
std::string C2SIMinterface::doubleToString(double d) {
	std::ostringstream oss;
	oss << d;
	return oss.str();
}
void C2SIMinterface::geodeticToGeocentric(char* lat, char* lon, char* alt,
	std::string &x, std::string &y, std::string &z) {

	// convert to geocentric
	DtGeodeticCoord geod(
		stod(lat)/degreesToRadians,
		stod(lon)/degreesToRadians, 
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

// read from queue of commands produced by readStomp 
// in C2SIM interface this replaces types input reader
// C2SIMinterface::non_blocking_stdin_fgets()
char* C2SIMinterface::non_blocking_C2SIM_fgets(
	char* buffer, 
	int bufferSize)
{
	char* nextCommand;
	if (timesCalled++ > 2)// ignore first two calls - they have junk
	{
		//If queue is empty, return NULL to caller
		if (commandQueue.empty())return NULL;

		// pull a command off of the queue and copy it for caller
		nextCommand = commandQueue.front();
		commandQueue.pop();
		if (strlen(nextCommand) > bufferSize - 1) {
			std::cout << "error:command length over buffer size (" <<
			bufferSize << "}:" << nextCommand;
			buffer[0] = '\0';
		}
		else strncpy(buffer, nextCommand, bufferSize);

		// free the original and return the copy
		// (necessary since gettok modifies its argument)
		std::cout << "dequeue:" << buffer << "\n";//debugx
		delete nextCommand;
		return buffer;
	}
	
	// first times called just delay
	DtSleep(1.0);
	return false;
  
}// end non_blocking_C2SIM_fgets()


// push a string command into the commandQueue
void C2SIMinterface::pushCommandToQueue(std::string command) {
	char* charCommand;
	
	// if the command does not end with \n, insert it
	if (command.back() != '\n') {
		string commandPlusSlashN = command.append("\n");
		charCommand = new char[commandPlusSlashN.length()+1];
		std::strcpy(charCommand, commandPlusSlashN.c_str());
	}
	else
	{
		charCommand = new char[command.length() + 1];
		std::strcpy(charCommand, command.c_str());
	}

	cout << "queue:" << charCommand << "\n";//debugx
	commandQueue.push(charCommand);
	
	// insert 10ms between commands
	DtSleep(.01);
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
}


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
}


// thread to listen for incoming STOMP message; parse the message,
// and use the result to send a command to VRForces
void C2SIMinterface::readStomp(DtTextInterface* textIf, C2SIMinterface* c2simInterface)
{	 
	HRESULT hr = CoInitialize(NULL);
	mee::stomp::StompFrame inputFrame;
	mee::stomp::StompClient stompClient(stompStream);

	if (SUCCEEDED(hr)) {

		// wait for VR-Forces to get ready for commands
		DtSleep(15.0);
		std::cout << "READY FOR C2SIM ORDERS\n";
      
		char testXml[] = "C:\\temp\\holdXml.xml";

		// loop reading XML Orders, parsing them into VR-Forces commands
		// and pushing to command queue
		while (true) {
			
			// read a STOMP message and write it to temp file
			stompClient.receiveFrame(inputFrame);
			C2SIMinterface::writeAnXmlFile(testXml, inputFrame.message.str());

			// use SAX to parse XML 
			c2simHandler->C2SIMHandler::startC2SIMParse(rootElement);
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

			// extract parsed values
			char* taskersIntent = new char[100];
			char* dateTime = new char[100];
			char* latitude1 = new char[100];
			char* latitude2 = new char[100];
			char* latitude3 = new char[100];
			char* longitude1 = new char[100];
			char* longitude2 = new char[100];
			char* longitude3 = new char[100];
			char* elevationAgl1 = new char[100];
			char* elevationAgl2 = new char[100];
			char* elevationAgl3 = new char[100];
			char* unitID = new char[100];

			// if the rootTag was found this an order - copy out data from parser 
			if (c2simHandler->C2SIMHandler::returnData(
				rootElement,
				taskersIntent,
				dateTime,
				latitude1,
				longitude1,
				elevationAgl1,
				latitude2,
				longitude2,
				elevationAgl2,
				latitude3,
				longitude3,
				elevationAgl3,
				unitID))
			{
				cout << "Parsed file:" << testXml << "\n";
				cout << "Parse results TaskersIntent:" << taskersIntent <<
					" DateTime:" << dateTime << " Latitude1:" << latitude1 <<
					" Longitude1:" << longitude1 << " ElevationAGL1:" << elevationAgl1 <<
					" Latitude2:" << latitude2 <<
					" Longitude2:" << longitude2 << " ElevationAGL2:" << elevationAgl2 <<
					" Latitude3:" << latitude3 <<
					" Longitude3:" << longitude3 << " ElevationAGL3:" << elevationAgl3 <<
					" UnitID:" << unitID << "\n";

				// convert lan/lon/elev to MAK's preferred geocentric coords for three points
				string x1, y1, z1, x2, y2, z2, x3, y3, z3;
				c2simInterface->geodeticToGeocentric(latitude1, longitude1, elevationAgl1, x1, y1, z1);
				c2simInterface->geodeticToGeocentric(latitude2, longitude2, elevationAgl2, x2, y2, z2);
				c2simInterface->geodeticToGeocentric(latitude3, longitude3, elevationAgl3, x3, y3, z3);
				string taskersIntentString = taskersIntent;
				string dateTimeString = dateTime;
				string unitIDString = unitID;

				// check for quit command
				if (taskersIntentString == "QUIT")break;

				// start a terrain and make a tank to run on it
				pushCommandToQueue("new \"Ground-db.mtf\"");
				string createTankCommand = "create tank " + x1 + "," + y1 + "," + z1;
				pushCommandToQueue(createTankCommand);
				
				// need generator to create unique point names for use below
				// create 3 waypoints for triangle path
				string createWaypointCommand, moveTankCommand, runCommand;
				createWaypointCommand = "create waypoint \"Pt 1\" " + x2 + "," + y2 + "," +z2;
				pushCommandToQueue(createWaypointCommand);
				createWaypointCommand = "create waypoint \"Pt 2\" " + x3 + "," + y3 + "," + z3;
				pushCommandToQueue(createWaypointCommand);
				createWaypointCommand = "create waypoint \"Pt 3\" " + x1 + "," + y1 + "," + z1;
				pushCommandToQueue(createWaypointCommand);

				// run the tank to waypoint 1
				moveTankCommand = "task moveToPoint \"M1A2 1\" \"Pt 1\"";
				pushCommandToQueue(moveTankCommand);
				runCommand = "run";
				pushCommandToQueue(runCommand);

				// wait 100 secd then run it to waypoint 2
				DtSleep(100.0);
				moveTankCommand = "task moveToPoint \"M1A2 1\" \"Pt 2\"";
				pushCommandToQueue(moveTankCommand);

				// wait another 100 then run it to waypoint 3
				DtSleep(100.0);
				pushCommandToQueue(runCommand);
				moveTankCommand = "task moveToPoint \"M1A2 1\" \"Pt 3\"";
				pushCommandToQueue(moveTankCommand);
				pushCommandToQueue(runCommand);

				// next: make a method to extract other parameters from order and use them to
				// create VR-Forces commands for a range of IBML09 'what' code Orders, bypassing
				// the DtTextInterface by calling for example (textIf.cpp line 895) 
				// a->moveToPoint(str) in place of text command "moveToPoint".
				// Also figure out how to fix the compile message that says version not recognized
			}
		}// end while(true)

		// tell VRForces to shutdown
		pushCommandToQueue("quit");
		cout << "Finished SAX\n";
		DtSleep(10.0);
    
	}// end method readStomp()

}// end class C2SIMInterface
