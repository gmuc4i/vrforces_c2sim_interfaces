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
*----------------------------------------------------------------*/

// modified 21May18 by JMP to handle new C2SIM_Message schema

#include "C2SIMxmlHandler.h"
#include <stdio.h>
#include <iostream>
#include "xercesc/sax/AttributeList.hpp"

using namespace xercesc;

// conversion factor
const static double degreesToRadians = 57.2957795131L;

// root of the XML document
std::string rootTag = "";

// storage for order data

// strings to manipulate XML parsing
char latestTag[MAXCHARARRAY];
char startTag[MAXCHARARRAY];
char endTag[MAXCHARARRAY];
char previousTag[MAXCHARARRAY];
char dataText[MAXCHARARRAY];

// strings extracted from XML
char taskersIntent[MAXCHARARRAY] = "";
char dateTime[MAXCHARARRAY] = "";
char unitId[MAXCHARARRAY] = "";
char latitude[MAXCHARARRAY];
char longitude[MAXCHARARRAY];
char elevation[MAXCHARARRAY];
char* latitudes[MAXPOINTS]; // each element is a latitude string
char* longitudes[MAXPOINTS];// each element is a longitude string
char* elevations[MAXPOINTS];// each element is an elevation string

// flags to indicate we've found a particular order string
bool foundC2simMessage = false;
bool skipThisDocument = false;
bool foundRootTag = false;
bool foundFinalTag = false;
static bool foundInitUnitTag = false;
static bool foundTaskTag = false;
bool foundPerformingEntity = false;
int latitudePointCount = 0;
int longitudePointCount = 0;
int elevationPointCount = 0;
bool parsingC2simOrder = false;
bool parsingIbmlOrder = false;
bool parsingMilitaryOrg = false;
bool parsingControlMessage = false;
bool parseIbml;

// Tasks for Orders
static Task* orderTasks;
int lastTaskIndex = -1;

// Military Organization data for Units
static Unit* militaryOrgUnits;
int lastUnitIndex = -1;

// Simulation Control data
// share, reset, start, stop, pause
static char* controlState = new char[10];

// constructor
C2SIMxmlHandler::C2SIMxmlHandler(bool useIbmlRef)
{
	// copy parameter value
	parseIbml = useIbmlRef;

	// initialize strings to zero length
	latestTag[0] = '\0';
	startTag[0] = '\0';
	endTag[0] = '\0';
	previousTag[0] = '\0';
	dataText[0] = '\0';
	taskersIntent[0] = '\0';
	dateTime[0] = '\0';
	unitId[0] = '\0';
	for (int i = 0; i < MAXPOINTS; ++i) {
		latitudes[i] = new char[MAXCHARARRAY];
		longitudes[i] = new char[MAXCHARARRAY];
		elevations[i] = new char[MAXCHARARRAY];
	}
	strncpy(controlState, "STOPPED", sizeof(controlState));

}// end constructor

// destructor
C2SIMxmlHandler::~C2SIMxmlHandler()
{
}// end destructor

// returns root tag found in latest parse
// if no root tag found this will be empty string
std::string C2SIMxmlHandler::getRootTag()
{
	return rootTag;
}

// return value of taskersIntent
char* C2SIMxmlHandler::getTaskersIntent()
{
	return taskersIntent;
}

// return value of dataTime
char* C2SIMxmlHandler::getDateTime()
{
	return dateTime;
}

// return value of unitId
char* C2SIMxmlHandler::getUnitId()
{
	return unitId;
}


// returns the count of Units from Military_Organization message
int C2SIMxmlHandler::getNumberOfUnits() {

	return lastUnitIndex + 1;

}// end C2SIMxmlHandler::getNumberOfUnits()


// returns the count of Units from Military_Organization message
int C2SIMxmlHandler::getNumberOfTasks() {

	return lastTaskIndex + 1;

}// end C2SIMxmlHandler::getNumberOfTasks()


// return controlAction from last C2SIM_Simulation_Control message
char* C2SIMxmlHandler::getControlState() {
	return controlState;
}


// called by C2SIMinterface to start parse 
// resets for a new C2SIM document
void C2SIMxmlHandler::startC2SIMParse(Unit* units, Task* tasks)
{
	// set local value of units and tasks
	militaryOrgUnits = units;
	orderTasks = tasks;
	
	// reset root tag
	rootTag = "";

	// reset logical flags and counts
	foundC2simMessage = false;
	skipThisDocument = false;
	foundRootTag = false;
	foundFinalTag = false;
	parsingC2simOrder = false;
	parsingIbmlOrder = false;
	parsingMilitaryOrg = false;
	parsingControlMessage = false;
}

// transcode XMLCh* to char*, copy that to char* output 
// then release the transcoded copy
// this seems inefficient but its too hard to
// keep track of and release multiple encodings
void C2SIMxmlHandler::copyXMLCh(char* output, const XMLCh* const input) {
	char* handoff = XMLString::transcode(input);
	strncpy(output, handoff, MAXCHARLENGTH);
	XMLString::release(&handoff);
}

// called at beginning of new XML doc
void C2SIMxmlHandler::startDocument()
{
}

// called at end of parsing XML doc
void C2SIMxmlHandler::endDocument()
{
}

void C2SIMxmlHandler::startElement(const XMLCh* const name,
	AttributeList& attributes)
{
	// copy name parameter to startTag
	C2SIMxmlHandler::copyXMLCh(startTag, name);
	//std::cout << "START:" << startTag << "|\n";//debugx
	
	// copy startTag to latestTag
	strncpy(latestTag, startTag, MAXCHARLENGTH);
	
	// check for root tag

	// special cases for C2SIM_MilitaryOrganization
	if (foundC2simMessage &&
		strncmp(latestTag, militaryOrgRootTag, MAXCHARLENGTH) == 0) {
		foundRootTag = true;
		rootTag = startTag;
		parsingMilitaryOrg = true;
		std::cout << "received C2SIM_MilitaryOrganization message\n";
	}
	// parser may swallow tags without data - force the issue
	else if (strncmp(latestTag, initUnitTag, MAXCHARLENGTH) == 0) {
		characters(L"", 0);
	}
	// all cases but C2SIM_MilitaryOrganization
	else if (!foundRootTag && !skipThisDocument) {

		// ignore report messages
		if ((strncmp(startTag, c2simReportRootTag, MAXCHARLENGTH) == 0) ||
			(strncmp(startTag, ibmlReportRootTag, MAXCHARLENGTH) == 0)) {
			rootTag = "";
			skipThisDocument = true;
		}

		// look for C2SIM message
		else if (!foundC2simMessage && !parseIbml &&
			strncmp(startTag, c2simMessageTag, MAXCHARLENGTH) == 0) {
			foundC2simMessage = true;
			foundRootTag = true;
			rootTag = startTag;
		}
		// or IBML09 Order
		else if (parseIbml && strncmp(latestTag, ibmlOrderRootTag, MAXCHARLENGTH) == 0) {
			foundRootTag = true;
			rootTag = startTag;
			parsingIbmlOrder = true;
			std::cout << "received IBML09 order - will report in IBML09 format\n";
		}
		// otherwise we don't have a root we can use
		else {
			std::cerr << "received unexpected root tag:" << startTag << "\n";
			rootTag = "";
			skipThisDocument = true;
		}
	}
}// end C2SIMxmlHandler::startElement


void C2SIMxmlHandler::endElement(const XMLCh* const name)
{
	// copy name parameter to endTag
	C2SIMxmlHandler::copyXMLCh(endTag, name);

	// check whether it is the closing mirror of root tag
	// if so, skip any other input (e.g. translated order from server)
	if (!foundFinalTag) {
		if (parsingMilitaryOrg) {
				if(strncmp(endTag, militaryOrgRootTag, MAXCHARLENGTH) == 0) {
				foundFinalTag = true;
				skipThisDocument = true;
			}
		}
		else if (parseIbml) {
			if (strncmp(endTag, ibmlOrderRootTag, MAXCHARLENGTH) == 0) {
				foundFinalTag = true;
				skipThisDocument = true;
			}
		} 
		else if (strncmp(endTag, c2simOrderRootTag, MAXCHARLENGTH) == 0) {
			foundFinalTag = true;
			skipThisDocument = true;
		}	
	}

	// delete latestTag
	latestTag[0] = '\0';

}// end C2SIMxmlHandler::endElement


 // character data
 // TODO: per documentation, this could come in pieces; 
 // all pieces should be collected before acting on it
void C2SIMxmlHandler::characters(
	const XMLCh* const data,
	const XMLSize_t dataLength) {

	C2SIMxmlHandler::copyXMLCh(dataText, data);

	// check for overlength data - warn if found
	if (dataLength > MAXCHARLENGTH)
		std::cerr << "WARNING data length " << dataLength << " over config limit of " <<
		MAXCHARLENGTH << " data truncated begins:" << dataText << "\n";
	if (skipThisDocument)return;
	//std::cout << "TAG:" << latestTag << "|DATA:" << dataText << "|\n"; // debugx

	// look for C2SIM order or control tag inside C2SIM message
	if (foundC2simMessage) {
		
		// C2SIM order inside C2SIM_Message
		if (strncmp(latestTag, c2simOrderRootTag, MAXCHARLENGTH) == 0) {
			rootTag = startTag;
			parsingC2simOrder = true;
			std::cout << "received C2SIM order - will report in C2SIM format\n";
			return;
		}
		// Control inside C2SIM_Message
		if (strncmp(latestTag, controlStateTag, MAXCHARLENGTH) == 0) {
			rootTag = controlRootTag;
			parsingControlMessage = true;
			strncpy(controlState, dataText, MAXCHARLENGTH);
			std::cout << "received C2SIM_Simulation_Control message\n";
			return;
		}
		// C2SIM_MilitaryOrganization inside C2SIM_Message
		if (strncmp(latestTag, militaryOrgRootTag, MAXCHARLENGTH) == 0) {
			rootTag = startTag;
			parsingMilitaryOrg = true;
			std::cout << "received C2SIM_MilitaryOrganization message\n";
			return;
		}
	}
	
	// if parse conditions not met just return
	if (!foundRootTag || foundFinalTag || *latestTag == '\0')return;
	
	// look for order element tags and copy their data one tag at a time

	// starting here we parse the various possible XML inputs
	// for our purposes C2SIm and IBML09 are semantically equivalent
	// so we store the results in a single set of variables

	// C2SIM Order
	if (!parseIbml && parsingC2simOrder) {
		
		// check that we have room for another task 
		if (lastTaskIndex == MAXTASKS - 1) {
			std::cerr << "too many taskss for allocated storage - stopping at "
				<< orderTasks[lastUnitIndex].unitId << "\n";
			skipThisDocument = true;
			return;
		}
		Task* thisTask;
		int dataSize;

		// look for Task tag that starts a new task
		if (strncmp(latestTag, taskTag, MAXCHARLENGTH) == 0) {
			
			// increment Units count	
			lastTaskIndex++;
			thisTask = &orderTasks[lastTaskIndex];

			// reset parse flags and counts for data elements
			thisTask->performingEntity = NULL;
			thisTask->unitId = NULL;
			thisTask->dateTime = NULL;
			thisTask->latitudePointCount = 0;
			thisTask->longitudePointCount = 0;
			thisTask->elevationPointCount = 0;
			foundTaskTag = true;
			foundPerformingEntity = false;

			// fill the lat/lon/elev string pointer with NULL
			for (int point = 0; point < MAXPOINTS; ++point) {
				thisTask->latitudes[point] = "0";
				thisTask->longitudes[point] = "0";
				thisTask->elevations[point] = "0";
			}
			return;
		}

		// don't start parsing Order before Task tag
		if (!foundTaskTag)return;
		thisTask = &orderTasks[lastTaskIndex];

		// look for PerformingEntity to find the right UnitID
		if (!foundPerformingEntity) {
			if (strncmp(latestTag, c2simPerformingEntityTag, MAXCHARLENGTH) == 0) {
				foundPerformingEntity = true;
				return;
			}
		}

		// look for the UnitId that follows PerformingEntity
		if (thisTask->unitId == NULL) {
			if (foundPerformingEntity && 
				strncmp(latestTag, c2simUnitIdTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisTask->unitId = new char[dataSize];
				strncpy(thisTask->unitId, dataText, dataSize);
				return;
			}
		}

		// look for dateTime
		if (thisTask->dateTime == NULL) {
			if (strncmp(latestTag, c2simDateTimeTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisTask->dateTime = new char[dataSize];
				strncpy(thisTask->dateTime, dataText, dataSize);
				return;
			}
		}

		// look for latitude of next point
		if (strncmp(latestTag, latitudeTag, MAXCHARLENGTH) == 0) {
			if (thisTask->latitudePointCount >= MAXPOINTS) {
				std::cerr << "too many latitudes; deleted " << latitudeTag << "\n";
				return;
			}
			thisTask->latitudes[thisTask->latitudePointCount++] = dataText;
			return;
		}

		// look for longitude of next point
		if (strncmp(latestTag, longitudeTag, MAXCHARLENGTH) == 0) {
			if (thisTask->longitudePointCount >= MAXPOINTS) {
				std::cerr << "too many longitudes; deleted " << longitudeTag << "\n";
				return;
			}
			thisTask->longitudes[thisTask->longitudePointCount++] = dataText;
			return;
		}

		// look for elevation of next point
		if (strncmp(latestTag, elevationTag, MAXCHARLENGTH) == 0) {
			if (thisTask->elevationPointCount >= MAXPOINTS) {
				std::cerr << "too many elevations; deleted " << elevationTag << "\n";
				return;
			}
			thisTask->elevations[thisTask->elevationPointCount++] = dataText;
			return;
		}

		// must be some tag we don't care about
		dataText[0] = '\0';

		return;

	}// end if(foundC2simMessage)

	// IBML09 order
	if (parseIbml && parsingIbmlOrder) {

		// check that we have room for another task 
		if (lastTaskIndex == MAXTASKS - 1) {
			std::cerr << "too many taskss for allocated storage - stopping at "
				<< orderTasks[lastUnitIndex].unitId << "\n";
			skipThisDocument = true;
			return;
		}
		Task* thisTask;
		int dataSize;

		// look for Task tag that starts a new task
		if (strncmp(latestTag, taskTag, MAXCHARLENGTH) == 0) {

			// increment Units count	
			lastTaskIndex++;
			thisTask = &orderTasks[lastTaskIndex];

			// reset parse flags and counts for data elements
			thisTask->performingEntity = NULL;
			thisTask->unitId = NULL;
			thisTask->dateTime = NULL;
			thisTask->latitudePointCount = 0;
			thisTask->longitudePointCount = 0;
			thisTask->elevationPointCount = 0;
			foundTaskTag = true;

			// fill the lat/lon/elev string pointer with NULL
			for (int point = 0; point < MAXPOINTS; ++point) {
				thisTask->latitudes[point] = "0";
				thisTask->longitudes[point] = "0";
				thisTask->elevations[point] = "0";
			}

			return;
		}

		// don't start parsing Order before Task tag
		if (!foundTaskTag)return;
		thisTask = &orderTasks[lastTaskIndex];

		// look for the UnitId 
		if (thisTask->unitId == NULL) {
			if (strncmp(latestTag, ibmlUnitIdTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisTask->unitId = new char[dataSize];
				strncpy(thisTask->unitId, dataText, dataSize);
				return;
			}
		}

		// look for dateTime
		if (thisTask->dateTime == NULL) {
			if (strncmp(latestTag, ibmlDateTimeTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisTask->dateTime = new char[dataSize];
				strncpy(thisTask->dateTime, dataText, dataSize);
				return;
			}
		}

		// look for latitude of next point
		if (strncmp(latestTag, latitudeTag, MAXCHARLENGTH) == 0) {
			if (thisTask->latitudePointCount >= MAXPOINTS) {
				std::cerr << "too many latitudes; deleted " << latitudeTag << "\n";
				return;
			}
			thisTask->latitudes[thisTask->latitudePointCount++] = dataText;
			return;
		}

		// look for longitude of next point
		if (strncmp(latestTag, longitudeTag, MAXCHARLENGTH) == 0) {
			if (thisTask->longitudePointCount >= MAXPOINTS) {
				std::cerr << "too many longitudes; deleted " << longitudeTag << "\n";
				return;
			}
			thisTask->longitudes[thisTask->longitudePointCount++] = dataText;
			return;
		}

		// look for elevation of next point
		if (strncmp(latestTag, elevationTag, MAXCHARLENGTH) == 0) {
			if (thisTask->elevationPointCount >= MAXPOINTS) {
				std::cerr << "too many longitudes; deleted " << elevationTag << "\n";
				return;
			}
			thisTask->elevations[thisTask->elevationPointCount++] = dataText;
			return;
		}

		// must be some tag we don't care about
		dataText[0] = '\0';
		return;

	}// end if (useIbml)

	// Military_Organization message - only one instance per run
	if (parsingMilitaryOrg) {

		// check that we have room for another unit
		if (lastUnitIndex == MAXINIT-1) {
			std::cerr << "too many units for allocated storage - stopping at "
				<< militaryOrgUnits[lastUnitIndex].name << "\n";
			skipThisDocument = true;
			return;
		}
		Unit* thisUnit;
		int dataSize;
		
		// look for Unit tag that starts a new unit
		if (strncmp(latestTag, initUnitTag, MAXCHARLENGTH) == 0) {
		
			// increment Units count	
			lastUnitIndex++;
			thisUnit = &militaryOrgUnits[lastUnitIndex];

			// reset parse flags for data elements
			thisUnit->id = NULL;
			thisUnit->name = NULL;
			thisUnit->opStatusCode = NULL;
			thisUnit->strengthPercent = NULL;
			thisUnit->hostilityCode = NULL;
			thisUnit->echelon = NULL;
			thisUnit->superiorUnit = NULL;
			thisUnit->latitude = NULL;
			thisUnit->longitude = NULL;
			thisUnit->elevationAgl = NULL;
			thisUnit->forceSide = NULL;
			thisUnit->submitter = NULL;
			foundInitUnitTag = true;
			return;
		}
		
		// don't start parsing Military_Organization before Unit tag
		if (!foundInitUnitTag)return;
		thisUnit = &militaryOrgUnits[lastUnitIndex];
		
		// look for ID
		if (thisUnit->id == NULL) {
			if (strncmp(latestTag, initIdTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->id = new char[dataSize];
				strncpy(thisUnit->id, dataText, dataSize);
				return;
			}
		}

		// look for Name
		if (thisUnit->name == NULL) {
			if (strncmp(latestTag, initNameTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->name = new char[dataSize];
				strncpy(thisUnit->name, dataText, dataSize);
				return;
			}
		}

		// look for OperationalStatusCode
		if (thisUnit->opStatusCode == NULL) {
			if (strncmp(latestTag, initOpStatusTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->opStatusCode = new char[dataSize];
				strncpy(thisUnit->opStatusCode, dataText, dataSize);
				return;
			}
		}

		// look for StrengthPercentage
		if (thisUnit->strengthPercent == NULL) {
			if (strncmp(latestTag, initStrengthTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->strengthPercent = new char[dataSize];
				strncpy(thisUnit->strengthPercent, dataText, dataSize);
				return;
			}
		}

		// look for HostilityCode
		if (thisUnit->hostilityCode == NULL) {
			if (strncmp(latestTag, initHostilityTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->hostilityCode = new char[dataSize];
				strncpy(thisUnit->hostilityCode, dataText, dataSize);
				return;
			}
		}

		// look for echelon
		if (thisUnit->echelon == NULL) {
			if (strncmp(latestTag, initEchelonTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->echelon = new char[dataSize];
				strncpy(thisUnit->echelon, dataText, dataSize);
				return;
			}
		}

		// look for SuperiorUnit
		if (thisUnit->superiorUnit == NULL) {
			if (strncmp(latestTag, initSuperiorUnittag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->superiorUnit = new char[dataSize];
				strncpy(thisUnit->superiorUnit, dataText, dataSize);
				return;
			}
		}
		
		// look for Latitude
		if (thisUnit->latitude == NULL) {
			if (strncmp(latestTag, initLatitudeTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->latitude = new char[dataSize];
				strncpy(thisUnit->latitude, dataText, dataSize);
				return;
			}
		}
		
		// look for Longitude
		if (thisUnit->longitude == NULL) {
			if (strncmp(latestTag, initLongitudeTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->longitude = new char[dataSize];
				strncpy(thisUnit->longitude, dataText, dataSize);
				return;
			}
		}
		
		// look for ElevationAGL
		if (thisUnit->elevationAgl == NULL) {
			if (strncmp(latestTag, initElevationAglTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->elevationAgl = new char[dataSize];
				strncpy(thisUnit->elevationAgl, dataText, dataSize);
				return;
			}
		}
		
		// look for SymbolIdentifier
		if (thisUnit->symbolId == NULL) {
			if (strncmp(latestTag, initSymbolIdTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->symbolId = new char[dataSize];
				strncpy(thisUnit->symbolId, dataText, dataSize);
				return;
			}
		}

		// look for SymbolIdentifier
		if (thisUnit->forceSide == NULL) {
			if (strncmp(latestTag, initForceSideTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->forceSide = new char[dataSize];
				strncpy(thisUnit->forceSide, dataText, dataSize);
				return;
			}
		}
		
		// look for Submitter
		if (thisUnit->submitter == NULL) {
			if (strncmp(latestTag, initSubmitterTag, MAXCHARLENGTH) == 0) {
				dataSize = strlen(dataText) + 1;
				thisUnit->submitter = new char[dataSize];
				strncpy(thisUnit->submitter, dataText, dataSize);
				return;
			}
		}

		// must be some tag we don't care about
		dataText[0] = '\0';
		return;

	}// end if(parsingMilitaryOrg)
	
	// control message (not used at present - state captured under C2SIM_Message)
	if (parsingControlMessage) {
		if (strncmp(latestTag, controlStateTag, MAXCHARLENGTH) == 0) {
			int dataSize = strlen(dataText) + 1;
			strncpy(controlState, dataText, dataSize);
			return;
		}
	}//end if(parsingControlMessage)

}// end C2SIMxmlHandler::characters

void C2SIMxmlHandler::fatalError(const SAXParseException& exception)
{
	char* message = XMLString::transcode(exception.getMessage());
	std::cerr << "Fatal Error: " << message <<
		" at line: " << exception.getLineNumber() << "\n";
	XMLString::release(&message);
}
