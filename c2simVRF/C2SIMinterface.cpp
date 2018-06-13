/*----------------------------------------------------------------*
|     Copyright 2018 Networking and Simulation Laboratory         |
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
VR-Forces C2SIM interface v1.3
To run demo on Sandbox: 
1. start VR Forces; at Config panel clock Launch;
on Startup panel run Bogaland9; wait until icon shows on map
(this can be slow)
2. vrforces4.5\bin64\c2simVRF; wait for READY FOR C2SIM ORDERS
3. send an order by C2SIM/RESTclient/REST-moveOrder.bat
4. if order is for unit not previously introduced, new icon will be made
   at first set of coordinates in order
5. icon should move on route from order and reports should be emitted
6. to see reports run C2SIM/STOMPclient/STOMP.bat or run BMLC2GUI
7. accepts IBML09 or C2SIMv0.6.8 schema and produces report 
   from same schema, selected by parameter 6 of main
   to use IBML09 see command-line parameters under main.cxx
8. can be run without initialization phase, in which case
   initial position of object is first (or only) geocoordinate
   in the task
*/

// updated to VRForces4.5 by JMP 12May18 by adding DtUUID()

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
#include <thread>
#include "xerces_utils.h"

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
C2SIMxmlHandler* c2simXmlHandler;
ErrorHandler* errHandler;
std::string stompServerAddress;
std::string stompServerPort;
static string vrfTerrain;
const static double degreesToRadians = 57.2957795131L;
int numberOfObjects = 0;
int orderCount = 0;
bool useIbml;
#define MAX_OBJECTS 100
std::string objectNames[MAX_OBJECTS];
mee::stomp::StompClient* client;

// constructor
C2SIMinterface::C2SIMinterface(
	DtVrfRemoteController* controller,
	std::string serverAddressRef,
	std::string stompPortRef,
	bool useIbmlRef)
{
	// pick up parameters from main()
	stompServerAddress = serverAddressRef;
	stompServerPort = stompPortRef;
	useIbml = useIbmlRef;

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
	c2simXmlHandler = new C2SIMxmlHandler(useIbml);
	errHandler = (ErrorHandler*)c2simXmlHandler;
	parser->setDocumentHandler(c2simXmlHandler);
	parser->setErrorHandler(errHandler);

}// end constructor

// destructor
C2SIMinterface::~C2SIMinterface() {
	delete parser;
	delete c2simXmlHandler;
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


// check object name - is it one we have already passed to VR-Forces?
boolean C2SIMinterface::isNewObject(string objectName)
{
	// range check
	if (numberOfObjects >= MAX_OBJECTS) {
		std::cerr << "error - too many different objects (" << MAX_OBJECTS << ")\n";
		return false;
	}
	
	// scan objects already seen
	for (int i = 0; i < numberOfObjects; ++i) {
		if (objectName == objectNames[i])
			return false;
	}

	// insert new object in array
	objectNames[numberOfObjects++] = objectName;
	return true;
}

// convert string coordinates to double
void C2SIMinterface::convertCoordinates(
	string lat, string lon, string elev,
	double &x, double &y, double &z) 
{
	// convert input geodetic coordinates to geocentric to return
	if (elev == NULL)elev = "0.0e0";
	DtGeodeticCoord geod(
		std::stod(lat) / degreesToRadians,
		std::stod(lon) / degreesToRadians,
		std::stod(elev));
	DtVector geoc = geod.geocentric();

	// check for converted elevation below MSL
	DtGeodeticCoord checkGeod;
	checkGeod.setGeocentric(geoc);
	if (checkGeod.alt() < 0.0e0) {
		std::cerr << "negative elevation set to 5.0m when recoding coordinate (" <<
			lat << "," << lon << "," << elev << ")\n";
		DtGeodeticCoord hackGeod(
			std::stod(lat) / degreesToRadians,
			std::stod(lon) / degreesToRadians,
			5.0e0);
		geoc = geod.geocentric();
	}

	// return geocentric 
	x = geoc.x();
	y = geoc.y();
	z = geoc.z();
}
// end C2SIMinterface::convertCoordinates()


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

// create AH-64 Apache helicopter  TODO: greater variety of platforms
void createAH64(DtTextInterface* textIf,
	DtVector vec,
	std::string objectId,
	std::string hostilityCode) {
	std::cout << "CREATEAH64 obj:" << objectId 
	  << "|hostility:" << hostilityCode << "\n";//debugx
	if (hostilityCode == "HO")
		textIf->controller()->createEntity(
		DtTextInterface::vrfObjectCreatedCb, (void*)"C2SIM",
		DtEntityType(1, 2, 225, 20, 1, 1, 0), vec,
		DtForceOpposing, 90.0, objectId);
	else
		textIf->controller()->createEntity(
		DtTextInterface::vrfObjectCreatedCb, (void*)"C2SIM",
		DtEntityType(1, 2, 225, 20, 1, 1, 0), vec,
		DtForceFriendly, 90.0, objectId);

	std::cout << "created AH-64 name:" << objectId 
		<< " hostility:" << hostilityCode << "\n";
}


// create a tank object in VR-Forces
void createTank(
	DtTextInterface* textIf, 
	DtVector vec, 
	std::string objectId,
	std::string hostilityCode) {

	if (hostilityCode == "HO")
		textIf->controller()->createEntity(
			DtTextInterface::vrfObjectCreatedCb, (void*)"C2SIM",
			DtEntityType(1, 1, 225, 1, 1, 3, 0), vec,
			DtForceOpposing, 90.0, objectId);
	else
		textIf->controller()->createEntity(
		DtTextInterface::vrfObjectCreatedCb, (void*)"C2SIM",
		DtEntityType(1, 1, 225, 1, 1, 3, 0), vec,
		DtForceFriendly, 90.0, objectId);

	std::cout << "created TANK name:" << objectId 
		<< " hostility:" << hostilityCode << "\n";
}


// create an aggregated object in VR-Forces
int theNextAggregateId = 0;
void createScoutUnit(
	DtTextInterface* textIf, 
	DtVector vec, 
	std::string objectId,
	std::string hostilityCode) {
	
	DtString pltName = objectId;// "Tank_Plt_" + DtString(theNextAggregateId++);

	unsigned long requestId = textIf->controller()->generateRequestId();
	DtObjectType oType(DtObjectTypePseudoAggregate,
		//DtEntityType(11, 1, 0, 14, 30, 0, 0));
		DtEntityType(11, 1, 225, 13, 3, 0, 1));

	DtList vertices;
	DtVector *vecPtr = new DtVector(vec);
	vertices.add(vecPtr);

	if (hostilityCode == "HO")
		textIf->controller()->sendVrfObjectCreateMsg(
			DtSimSendToAll, requestId, pltName, oType, vertices,
			DtString::nullString(), DtAppearance::nullAppearance(),
			DtForceOpposing, false, false, 0.0, true);
	else
		textIf->controller()->sendVrfObjectCreateMsg(
		DtSimSendToAll, requestId, pltName, oType, vertices,
		DtString::nullString(), DtAppearance::nullAppearance(),
		DtForceFriendly, false, false, 0.0, true);

	std::cout << "created aggregate name:" << objectId 
		<< " hostility:" << hostilityCode << "\n";

	delete vecPtr;
}


 // thread function to listen for incoming STOMP message; parse the message,
 // and use the result to send a command to VRForces
void C2SIMinterface::readStomp(
	DtTextInterface* textIf, 
	C2SIMinterface* c2simInterface,
	bool skipInitialize,
	std::string clientId) {

	HRESULT hr = CoInitialize(NULL);
	mee::stomp::StompFrame inputFrame;
	mee::stomp::StompClient stompClient(stompStream);
	bool inInitializePhase;
	bool setInInitializePhase = true;
	Unit* units = new Unit[MAXINIT];
	Task* tasks = new Task[MAXTASKS];
	int numberOfUnits = 0;
	
	// connect to STOMP server
	std::cout << "connecting STOMP stream to " 
		<< stompServerAddress << ":" << stompServerPort << "\n";
	try {
		// Establish the connection and associate it with StompClient
		std::stringstream buf;
		buf << stompServerPort;
		stompStream.connect(stompServerAddress.c_str(), buf.str());
		if (stompStream) {
			client = new mee::stomp::StompClient(stompStream);
			client->connect(false, "", "");
			client->subscribe("/topic/BML", (mee::stomp::AcknowledgeMode)1, "COMMENT");
		}
		else {
			std::cerr
				<< stompServerAddress << ":" << stompServerPort
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
	if (SUCCEEDED(hr)) {
		try{
			// wait a second for VR-Forces to start
			// TODO: figure out how to synchronize this
			DtSleep(1.0);
			if (!skipInitialize)
				std::cout << "READY FOR C2SIM INITIALIZATION\n";
			else 
				std::cout << "CONFIGURED TO SKIP INITIALIZATION\n";

			// loop reading XML documents, 
			// parsing them into VR-Forces controls 
			bool showReadyForOrder = true;
			while (true) {
				inInitializePhase = setInInitializePhase;
				if ((!inInitializePhase || skipInitialize) && showReadyForOrder){
					std::cout << "READY FOR C2SIM ORDER\n";
					showReadyForOrder = false;
				}

				// read a STOMP message and parse it
				stompClient.receiveFrame(inputFrame);
				//std::cout << inputFrame.message.str();

				// TODO: detect reports via simple string search and do not parse them

				// use SAX to parse XML from a STOMP frame
				c2simXmlHandler->C2SIMxmlHandler::startC2SIMParse(units, tasks);
				try {
					// run the parser, which will callback
					// our XML handler, from XML in memory
					std::string xmlString = inputFrame.message.str();
					int xmlSize = xmlString.size();
					char* xmlCstr = new char[xmlSize + 1];
					strncpy(xmlCstr, xmlString.c_str(), xmlSize);
					MemBufInputSource xmlInMemory(
						(const XMLByte*)xmlCstr,
						xmlSize,
						"xmlInMemory",
						false);
					parser->parse(xmlInMemory);
					delete xmlCstr;
					inputFrame.message.str(std::string(""));
				}
				catch (const XMLException& toCatch) {
					char* message = XMLString::transcode(toCatch.getMessage());
					std::cerr << "Exception message is: \n" << message << "\n";
					XMLString::release(&message);
				}
				catch (const SAXParseException& toCatch) {
					char* message = XMLString::transcode(toCatch.getMessage());
					std::cerr << "Exception message is: \n" << message << "\n";
					XMLString::release(&message);
				}
				catch (...) {
					std::cerr << "unexpected Exception in SAX parsing\n";
				}

				// check that we parsed something; if not go to next order
				string parsedRootTag = c2simXmlHandler->getRootTag();
				if (parsedRootTag.length() == 0)continue;
				
				// ignore non-order C2SIM messages
				// (orders will have root tag C2SIM_Order)
				if (parsedRootTag == "C2SIM_Message"){
					showReadyForOrder = false;
					continue;// to while(true)
				}
				if (parsedRootTag == "BMLReport"){
					showReadyForOrder = false;
					continue;// to while(true)
				}

				// check for simulation control message
				if (parsedRootTag == "C2SIM_Simulation_Control") {
					string controlState = c2simXmlHandler->getControlState();
					std::cout << "received simulation control state:" << controlState << "\n";

					// pass the control on to VR-Forces
					if (controlState == "RUNNING") {
						textIf->controller()->run();
						textIf->setStarted(true);
					}
					else if (controlState == "PAUSED") {
						textIf->controller()->pause();
						textIf->setStarted(false);
					}
					else if (controlState == "STOPPED") {
						textIf->setTimeToQuit(true);
						textIf->setStarted(false);
						Sleep(10000);
						break;
					}
					// set value for inInitializePhase at  
					// start of next STOMP receiveFrame
					else if (controlState == "UNINITIALIZED") {
						setInInitializePhase = true;
					}
					else if (controlState == "INITIALIZED") {
						setInInitializePhase = false;
					}
					continue;
				}
				std::cout << "completed parse of document with root tag " << parsedRootTag << "\n";
				showReadyForOrder = true;

				// if in the initialize phase the input can only be Military_Organization
				if (inInitializePhase && !skipInitialize) {
					if (parsedRootTag != "C2SIM_MilitaryOrganization") {
						std::cerr << "received unexpected root tag:" << parsedRootTag << "\n";
						continue;
					}
					// extract values from Military_Organization message
					numberOfUnits = c2simXmlHandler->C2SIMxmlHandler::getNumberOfUnits();
					if (numberOfUnits < 1) {
						std::cout << "no units found - can't run simulation\n";
						Sleep(10000);
						break;
					}
					std:: cout << "initialization message contains " << numberOfUnits << " units\n";

					// initialize in VR-Forces units that match this Submitter
					int numberInitialized = 0;
					for (int unitNumber = 0; unitNumber < numberOfUnits; ++unitNumber) {
						
						// determine what name we'll use for it
						Unit newUnit = units[unitNumber];
						std::string newObjectId;
						if (newUnit.name != NULL)
							newObjectId = newUnit.name;
						else {
							std::cerr << "position " << (unitNumber + 1)
								<< " in Military_Organization has no name - omitting it\n";
							continue;
						}
						if (newUnit.submitter == NULL) {
							std::cout << "unit " << newUnit.name
								<< " missing Submitter - cannot create VRF object\n";
							continue;// to while(true)
						}
						std::string submitterString = std::string(newUnit.submitter);
						if (submitterString != clientId){
							std::cout << "unit " << newUnit.name
								<< " Submitter " << submitterString 
								<< " does not match our clientId "
								<< clientId << " cannot create VRF object\n";
							continue;// to while(true)
						}
						if (newUnit.hostilityCode == NULL) {
							std::cout << "unit " << newUnit.name
								<< " missing Hostility - cannot create VRF object\n";
							continue;// to while(true)
						}

						// instantiate the object in VR-Forces
						if (isNewObject(newObjectId)) {

							// check that coordinates were provided
							if ((newUnit.latitude == NULL) || (newUnit.longitude == NULL)) {
								std::cout << "missing latitude/longitude for unit " << newObjectId
									<< " - ommiting it\n";
								continue;
							}

							// convert coordinate to geocentric and create the entity
							double x, y, z;
							if (newUnit.elevationAgl == NULL)
								newUnit.elevationAgl = "0.0e0";
							c2simInterface->convertCoordinates(
								newUnit.latitude, newUnit.longitude, newUnit.elevationAgl, x, y, z);
							DtVector newUnitVec(x, y, z);
							std::string symbolIdString = std::string(newUnit.symbolId);
							if (symbolIdString.substr(2,4) == "APMH")
								createAH64(textIf, newUnitVec, newObjectId, newUnit.hostilityCode);
							else if (strcmp(newUnit.echelon, "SQUAD") == 0)
								createScoutUnit(textIf, newUnitVec, newObjectId, newUnit.hostilityCode);
							else
								createTank(textIf, newUnitVec, newObjectId, newUnit.hostilityCode);
							numberInitialized++;

						}// end if (newUnit.submitter == clientId)
					}
					std::cout << "initialized " << numberInitialized << " units\n";
					// next line probably this will beat the server
					// message that causes the same effect
					inInitializePhase = false;
					continue;// to end of while(true) 

				}// end if (inInitializePhase...

				// not control or initialization; must be an order
				// loop through all tasks from the order
				++orderCount;
				int taskCount = c2simXmlHandler->getNumberOfTasks();
				std::cout << "order contains " << taskCount << " tasks\n";
				for (int taskNumber = 0; taskNumber < taskCount; ++taskNumber) {

					// find parameters of the task
					Task* thisTask = &tasks[taskNumber];

					// Start a thread to execute the task
					string taskId;
					if (orderCount > 1)
						taskId << "Order" << orderCount << "_Task" << (taskNumber + 1);
					else
						taskId << "Task" << (taskNumber + 1);
					std::thread t2(&C2SIMinterface::executeTask, taskId, textIf,
						skipInitialize, thisTask, numberOfUnits, units);
					t2.detach();
					
				}//end for (taskNumber...

			}// end while(true)

			// tell VRForces to shutdown
			textIf->setTimeToQuit(true);
			cout << "C2SIM VRF interface quitting\n";
		}
		catch (...) {
			std::cerr << "unanticipated Exception in interface\n";
		}
	}// end if (SUCCEEDED)

}// end readStomp()


// thread function to execute a C2SIM Task in VR-Forces
void C2SIMinterface::executeTask(
	string taskId,
	DtTextInterface* textIf,
	bool skipInitialize,
	Task* thisTask,
	int numberOfUnits,
	Unit* units) 
{
	// extract parameters from this task
	string dateTime = thisTask->dateTime;
	string unitId = thisTask->unitId;
	int numberOfPoints = 0;

	// find smallest count of lat/lon/elev points
	numberOfPoints = thisTask->latitudePointCount;
	if (thisTask->longitudePointCount < numberOfPoints)
		numberOfPoints = thisTask->longitudePointCount;
	if (useIbml && (thisTask->elevationPointCount > 0) &&
		(thisTask->elevationPointCount < numberOfPoints))
		numberOfPoints = thisTask->elevationPointCount;
	if (numberOfPoints == 0) {
		std::cout << "no location given - can't execute order\n";
		return;
	}

	// get arrays of geocentric point coordinates
	double x[MAXPOINTS];
	double y[MAXPOINTS];
	double z[MAXPOINTS];
	for (int pointNumber = 0; pointNumber < numberOfPoints; ++pointNumber) {
		C2SIMinterface::convertCoordinates(
			thisTask->latitudes[pointNumber],
			thisTask->longitudes[pointNumber],
			thisTask->elevations[pointNumber],
			x[pointNumber],
			y[pointNumber],
			z[pointNumber]);

	}// end for (int pointNumber...

	// confirm Unit was initialized (if not, skip this task)
	int unitIndex;
	if (!skipInitialize) {	
		for (unitIndex = 0; unitIndex < numberOfUnits; ++unitIndex)
		if (unitId == units[unitIndex].name)break;
		if (unitIndex >= numberOfUnits)return;
	}
	cout << "C2SIMinterface got task for unitID:" << unitId
		<< " dateTime:" << dateTime << " coordinates:"
		<< thisTask->latitudes[0] << "/" << thisTask->longitudes[0]
		<< " actionTask:" << thisTask->actionTaskActivityCode << "\n";
	std::cout << "starting to create VRF waypoints\n";

	// previously we generated a "new" VRF command at this point
	// now we're expecting the user to do that on VR-Forces GUI
	// during startup, giving more flexibility to Sandbox users

	// make an object to run on it
	// TODO: more objects
	int firstWaypoint = 0;
	if (skipInitialize)
	if (isNewObject(unitId)) {
		DtVector vec(x[0], y[0], z[0]);
		if (unitId == "USA1" || unitId == "USA2")// debugx hack for CWIX
			createScoutUnit(textIf, vec, unitId, "AFR");
		else
			createTank(textIf, vec, unitId, "AFR");
		firstWaypoint = 1;
	}
	
	// create waypoints for all remaining numberOfPoints
	// TODO: replace this with createRoute()
	string pointNames[MAXPOINTS];
	for (int pointNumber = firstWaypoint; pointNumber < numberOfPoints; ++pointNumber) {

		// combine coordinates to a DtVector
		DtVector vec(x[pointNumber], y[pointNumber], z[pointNumber]);
		
		// name a point name ending in i (up to 99)
		std::ostringstream oss;
		if (numberOfPoints > 1)
			oss << taskId << "_Pt" << pointNumber;
		else
			oss << taskId;
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
	DtSleep(5.);//TODO: (is there a race here?)
	std::cout << "starting to issue VRF unit commands\n";

	// send rules of engagement to the units
	if (thisTask->actionTaskActivityCode != "ATTACK")
		textIf->controller()->
			setRulesOfEngagement(DtUUID(unitId), "fire-when-fired-upon", DtSimSendToAll);

	// run the tank to each waypoint and wait until it gets there
	textIf->controller()->run(); // server run command comes too late - do it here

	// when skipInitialize we've already used the first point as initial location
	int pointNumber = 0;
	if (skipInitialize)pointNumber = 1;
	for (; pointNumber < numberOfPoints; ++pointNumber) {
		std::cout << "object " << unitId << "actiontask " << thisTask->actionTaskActivityCode
			<< " coordinates " << thisTask->latitudes[0] << "/" << thisTask->longitudes[0] << "\n";
		textIf->controller()->moveToWaypoint(DtUUID(unitId), DtUUID(pointNames[pointNumber]));
		while (textIf->getTaskNumberCompleted() < pointNumber)
			DtSleep(.1);
	}

}// end executeTask()
