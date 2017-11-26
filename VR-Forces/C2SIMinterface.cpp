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
std::queue<string> commandQueue;
std::queue<string> taskQueue;
static int timesCalled = 0;
DtVrfRemoteController* cs2sim_controller;
SAXParser* parser;
C2SIMHandler* c2simHandler;
ErrorHandler* errHandler;
std::string stompServerAddress;
static char rootElement[100];
static string vrfTerrain;


// constructor
C2SIMinterface::C2SIMinterface(
	DtVrfRemoteController* controller, 
	std::string serverAddressRef, 
	char* orderXmlRootElement)
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
// NOTE: VRForces uses radian angles; we convert here
double degreesToRadians = 57.2957795131L;
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


// read from queue of commands produced by readStomp 
// in C2SIM interface this replaces types input reader
// C2SIMinterface::non_blocking_stdin_fgets()
char* C2SIMinterface::non_blocking_C2SIM_fgets(
	char* buffer, 
	int bufferSize)
{
	string nextCommand;

	if (timesCalled++ > 2)// ignore first two calls - they have junk
	{
		//If queue is empty, return NULL to caller
		if (commandQueue.empty())return NULL;

		// pull a command off of the queue and copy it for caller
		nextCommand = commandQueue.front();
		commandQueue.pop();
		if (nextCommand.length() > bufferSize - 1) {
			std::cout << "error:command length over command buffer size (" <<
			bufferSize << "}:" << nextCommand;
			buffer[0] = '\0';
		}
		else strncpy(buffer, nextCommand.c_str(), bufferSize);

		// free the original and return the copy
		// (necessary since gettok modifies its argument)
		std::cout << "dequeue:" << buffer << "\n";//debugx
		return buffer;
	}

	return false;
  
}// end non_blocking_C2SIM_fgets()


// push a string command into the commandQueue
void C2SIMinterface::pushCommandToQueue(std::string command) {
	
	// if the command does not end with \n, insert it
	if (command.back() != '\n')command.append("\n");

	cout << "push to command queue:" << command << "\n";//debugx
	commandQueue.push(command);

}// end pushCommandToQueue()

 // push a string command into the taskQueue
void C2SIMinterface::pushCommandToTaskQueue(std::string command) {

	// if the command does not end with \n, insert it
	if (command.back() != '\n')command.append("\n");

	// push command onto queue
	cout << "push to task queue:" << command << "\n";//debugx
	taskQueue.push(command);

}// end pushTaskCommandToTaskQueue()

// transfer commands from taskQueue to commandQueue
// stop at the next "run"
void C2SIMinterface::moveTaskQueueCommandsToCommandQueue() {

	string commandToMove = "";
	
	// loop until last command moved matches "run"
	while (commandToMove.compare("run\n") != 0) {
		if (taskQueue.empty())return;
		commandToMove = taskQueue.front();
		taskQueue.pop();
		std::cout << "MOVE COMMAND:" << commandToMove << "\n";//debugx
		C2SIMinterface::pushCommandToQueue(commandToMove);
	}
	//DtSleep(60.);//debugx

}// end moveTaskQueueCommandsToCommandQueue()

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

		// wait for VR-Forces to get ready for commands
		DtSleep(10.);
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
			char* latitude4 = new char[100];
			char* longitude1 = new char[100];
			char* longitude2 = new char[100];
			char* longitude3 = new char[100];
			char* longitude4 = new char[100];
			char* elevationAgl1 = new char[100];
			char* elevationAgl2 = new char[100];
			char* elevationAgl3 = new char[100];
			char* elevationAgl4 = new char[100];
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
				latitude4,
				longitude4,
				elevationAgl4,
				unitID))
			{
				cout << "Parsed file:" << testXml << "\n";

				// convert lan/lon/elev to MAK's preferred geocentric coords for three points
				string x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
				c2simInterface->geodeticToGeocentric(latitude1, longitude1, elevationAgl1, x1, y1, z1);
				// hack for negative alt from conversion debugx
				std::string clat, clon, calt;
				c2simInterface->geocentricToGeodetic(x1, y1, z1, clat, clon, calt);
				if (stringToDouble(calt) < 0.0e0) {
					std::cout << "bad Alt: " << calt;
					c2simInterface->geodeticToGeocentric(latitude1, longitude1, "5.0", x1, y1, z1);
					c2simInterface->geocentricToGeodetic(x1, y1, z1, clat, clon, calt);
					std::cout << " revised value:" << calt << "\n";
				}// end hack debugx
				c2simInterface->geodeticToGeocentric(latitude2, longitude2, elevationAgl2, x2, y2, z2);
				// hack debugx
				c2simInterface->geocentricToGeodetic(x2, y2, z2, clat, clon, calt);
				if (stringToDouble(calt) < 0.0e0) {
					std::cout << "bad Alt: " << calt;
					c2simInterface->geodeticToGeocentric(latitude2, longitude2, "5.0", x2, y2, z2);
					c2simInterface->geocentricToGeodetic(x2, y2, z2, clat, clon, calt);
					std::cout << " revised value:" << calt << "\n";
				}// end hack debugx
				c2simInterface->geodeticToGeocentric(latitude3, longitude3, elevationAgl3, x3, y3, z3);
				// hack debugx
				c2simInterface->geocentricToGeodetic(x3, y3, z3, clat, clon, calt);
				if (stringToDouble(calt) < 0.0e0) {
					std::cout << "bad Alt: " << calt;
					c2simInterface->geodeticToGeocentric(latitude3, longitude3, "5.0", x3, y3, z3);
					c2simInterface->geocentricToGeodetic(x3, y3, z3, clat, clon, calt);
					std::cout << " revised value:" << calt << "\n";
				}// end hack debugx
				c2simInterface->geodeticToGeocentric(latitude4, longitude4, elevationAgl4, x4, y4, z4);
				// hack debugx
				c2simInterface->geocentricToGeodetic(x4, y4, z4, clat, clon, calt);
				if (stringToDouble(calt) < 0.0e0) {
					std::cout << "bad Alt: " << calt;
					c2simInterface->geodeticToGeocentric(latitude4, longitude4, "5.0", x4, y4, z4);
					c2simInterface->geocentricToGeodetic(x4, y4, z4, clat, clon, calt);
					std::cout << " revised value:" << calt << "\n";
				}// end hack debugx
				string taskersIntentString = taskersIntent;
				string dateTimeString = dateTime;
				string unitIDString = unitID;

				// check for quit command
				if (taskersIntentString == "QUIT")break;

				// start a terrain and make a tank to run on it
				//pushCommandToQueue("new \"Ground-db.mtf\"");
				pushCommandToTaskQueue("new \"Ala Moana.mtf\"");

				string createTankCommand = "create tank " + x1 + "," + y1 + "," + z1;
				pushCommandToTaskQueue(createTankCommand);
				
				// TODO: need generator to create unique point names for use below
				// create 3 waypoints for path
				string createWaypointCommand, moveTankCommand;
				string runCommand = "run";
				createWaypointCommand = "create waypoint \"Pt 1\" " + x2 + "," + y2 + "," +z2;
				pushCommandToTaskQueue(createWaypointCommand);
				createWaypointCommand = "create waypoint \"Pt 2\" " + x3 + "," + y3 + "," + z3;
				pushCommandToTaskQueue(createWaypointCommand);
				createWaypointCommand = "create waypoint \"Pt 3\" " + x4 + "," + y4 + "," + z4;
				pushCommandToTaskQueue(createWaypointCommand);

				// run the tank to waypoint 1
				moveTankCommand = "task moveToPoint \"M1A2 1\" \"Pt 1\"";
				pushCommandToTaskQueue(moveTankCommand);
				pushCommandToTaskQueue(runCommand);

				// now run it to waypoint 2
				moveTankCommand = "task moveToPoint \"M1A2 1\" \"Pt 2\"";
				pushCommandToTaskQueue(moveTankCommand);
				pushCommandToTaskQueue(runCommand);

				// then run it to waypoint 3
				moveTankCommand = "task moveToPoint \"M1A2 1\" \"Pt 3\"";
				pushCommandToTaskQueue(moveTankCommand);
				pushCommandToTaskQueue(runCommand);

				// move the first set of commands from taskQueue to commandQueue
				C2SIMinterface::moveTaskQueueCommandsToCommandQueue();

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
    
	}// end readStomp()

}// end class C2SIMInterface
