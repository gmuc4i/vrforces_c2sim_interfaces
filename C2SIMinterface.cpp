#include "C2SIMinterface.h"
#include <queue>
#include <vlutil/vlProcessControl.h>
#include "textIf.h"
#include <vrfcontrol/vrfRemoteController.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <tchar.h>
#include <windows.h>
#include "xerces_utils.h"
#include "C2SIMHandler.h"
#include "stomp-util.h"
#include "StompClient.h"

using namespace xercesc;

// global data
std::queue<char*> commandQueue;
static int timesCalled = 0;
DtVrfRemoteController* cs2sim_controller;
DtTextInterface* cs2sim_textIf;
SAXParser* parser;
C2SIMHandler* c2simHandler;
ErrorHandler* errHandler;
mee::stomp::StompClient* client;

// constructor
C2SIMinterface::C2SIMinterface()
{
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
		// Establish the connection
		std::stringstream buf;
		buf << port;
		stream.connect(host, buf.str());
		client = new mee::stomp::StompClient(stream);
		if (stream) {
			client->connect();
		} else {
			std::cerr
				<< "Failed to connect to "
				<< host << ":" << port
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

	// start C2SIM VRForces command queue
	char *commandInput = new char[50];
	char *command = "new	Ground-db.mtf";//debugx queue priming
	strcpy(commandInput, command);
	pushCommandToQueue(commandInput);

}// end constructor

// destructor
C2SIMinterface::~C2SIMinterface( ){
	delete parser;
	delete c2simHandler;
	client->disconnect();
	XMLPlatformUtils::Terminate();
}


// read from queue of commands produced by readStomp 
// in C2SIM interface this replaces types input reader
// C2SIMinterface::non_blocking_stdin_fgets()
char* C2SIMinterface::non_blocking_C2SIM_fgets(char* buffer, int bufferSize)
{
	if (timesCalled++ > 2)
	{
		//If queue is empty, return NULl to caller
		if (commandQueue.empty())return NULL;

		// pull a command off of the queue and copy it for caller
		char* nextCommand = commandQueue.front();
		commandQueue.pop();
		if (strlen(nextCommand) > bufferSize - 1) {
		cout << "error:command length over buffer size (" <<
			bufferSize << "}:" << nextCommand;
			buffer[0] = '\0';
		}
		else strncpy(buffer, nextCommand, bufferSize);

		// free the original and return the copy
		// (necessary since gettok modifies its argument)
		cout << "queue returning:" << buffer << "\n";//debugx
		delete nextCommand;
		return buffer;
	}

	// first times called just delay
	DtSleep(1.0);
	return false;
  
}// end non_blocking_C2SIM_fgets()

// push a text command onto the commandQueue
void  C2SIMinterface::pushCommandToQueue(char* command) {
	cout << "pushing to queue:" << command << "\n";//debugx
	commandQueue.push(command);
}

// read an XML file and return it as wstring
string C2SIMinterface::readAnXmlFile(string filename) {

	// open file, get count of lines, and start reader
	ifstream xmlFile(filename);
	if (!xmlFile.is_open()) {
		cout << "can't open file " << filename << "\n";
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

// thread to listen for incoming STOMP message; parse the message,
// and use the result to send a command to VRForces
void C2SIMinterface::readStomp(DtVrfRemoteController* controller, DtTextInterface* textIf)
{	
	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr)) {
      
		char testXml[] = "C:\\Users\\c2sim\\Desktop\\IBML09_Order.xml";//debugx
		//char testXml[] = "C:\\Users\\c2sim\\Desktop\\IBML09_MoveOrder-nonsx.xml";//debugx

		// use SAX to parse XML message
		c2simHandler->C2SIMHandler::startC2SIMParse("OrderPushIBML");
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
		char dateTime[100], latitude[100], longitude[100], unitID[100];
		c2simHandler->C2SIMHandler::returnData(dateTime, latitude, longitude, unitID);
		cout << "Parse reesults DateTime:" << dateTime << " Latitude:" << latitude <<
			" Longitude:" << longitude << " UnitID:" << unitID << "\n";

		// tell VRForces to shutdown
		DtSleep(10.0);//debugx
		//pushCommandToQueue("quit	"); debugx
		cout << "Finished SAX\n";
        
    // debugx  
	// when the above works we'll extract other parameters from order, read order from STOMP client,
	// use the parameters to make a text command, push the text into the queue, and run it in a loop
    
		// wait here for shutdown debugx
		while(true)DtSleep(1.0);
		return;
    
	}// end C2SIMInterface()

}// end class C2SIMInterface
