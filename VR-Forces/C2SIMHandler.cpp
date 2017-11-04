

#include "C2SIMHandler.h"
#include <stdio.h>
#include <iostream>
#include "xercesc/sax/AttributeList.hpp"

using namespace xercesc;

int maxCharLength = 99; // somewhat arbitrary
char rootTag[100];
char latestTag[100];
char startTag[100];
char endTag[100];
char previousTag[100];
char dataText[100];
char taskersIntent[100];
char dateTime[100];
char latitude1[100];
char longitude1[100]; 
char elevationAgl1[100];
char latitude2[100];
char longitude2[100];
char elevationAgl2[100];
char latitude3[100];
char longitude3[100];
char elevationAgl3[100];

char unitID[100];
bool foundTaskersIntent = false;
bool foundDateTime = false;
bool foundLatitude1 = false;
bool foundLongitude1 = false;
bool foundElevationAgl1 = false;
bool foundLatitude2 = false;
bool foundLongitude2 = false;
bool foundElevationAgl2 = false;
bool foundLatitude3 = false;
bool foundLongitude3 = false;
bool foundElevationAgl3 = false;
bool foundUnitID = false;
bool foundRootTag = false;
bool foundFinalTag = false;

// constructor
C2SIMHandler::C2SIMHandler()
{
	// initialize strings to zero length
	rootTag[0] = '\0';
	latestTag[0] = '\0';
	startTag[0] = '\0';
	endTag[0] = '\0';
	previousTag[0] = '\0';
	dataText[0] = '\0';
	taskersIntent[0] = '\0';
	dateTime[0] = '\0';
	latitude1[0] = '\0';
	longitude1[0] = '\0';
	elevationAgl1[0] = '\0';
	latitude2[0] = '\0';
	longitude2[0] = '\0';
	elevationAgl2[0] = '\0';
	latitude3[0] = '\0';
	longitude3[0] = '\0';
	elevationAgl3[0] = '\0';
	unitID[0] = '\0';
}

// destructor
C2SIMHandler::~C2SIMHandler()
{
}

// called by C2SIMinterface to start parse of C2SIM document type
void C2SIMHandler::startC2SIMParse(char* newRootTag)
{	
	// accept root tag for this parse
	strncpy(rootTag, newRootTag, maxCharLength);

	// reset logical flags
	foundRootTag = false;
	foundFinalTag = false;
	foundTaskersIntent = false;
	foundDateTime = false;
	foundLatitude1 = false;
	foundLongitude1 = false;
	foundElevationAgl1 = false;
	foundLatitude2 = false;
	foundLongitude2 = false;
	foundElevationAgl2 = false;
	foundLatitude3 = false;
	foundLongitude3 = false;
	foundElevationAgl3 = false;
	foundUnitID = false;
}

// give the parsed values back on request
bool C2SIMHandler::returnData(
	char* rootElement,
	char* returnTaskersIntent,
	char* returnDateTime,
	char* returnLatitude1,
	char* returnLongitude1,
	char* returnElevationAgl1,
	char* returnLatitude2,
	char* returnLongitude2,
	char* returnElevationAgl2,
	char* returnLatitude3,
	char* returnLongitude3,
	char* returnElevationAgl3,
	char* returnUnitID) {
	if (!foundRootTag)return false;
	strncpy(returnTaskersIntent, taskersIntent, maxCharLength);
	strncpy(returnDateTime, dateTime, maxCharLength);
	strncpy(returnLatitude1, latitude1, maxCharLength);
	strncpy(returnLongitude1, longitude1, maxCharLength);
	strncpy(returnElevationAgl1, elevationAgl1, maxCharLength);
	strncpy(returnLatitude2, latitude2, maxCharLength);
	strncpy(returnLongitude2, longitude2, maxCharLength);
	strncpy(returnElevationAgl2, elevationAgl2, maxCharLength);
	strncpy(returnLatitude3, latitude3, maxCharLength);
	strncpy(returnLongitude3, longitude3, maxCharLength);
	strncpy(returnElevationAgl3, elevationAgl3, maxCharLength);
	strncpy(returnUnitID, unitID, maxCharLength);
	return true;
}

// transcode XMLCh* to char*, copy that to char* output 
// then release the transcoded copy
// this seems inefficient but its too hard to
// keep track of and release multiple encodings
void C2SIMHandler::copyXMLCh(char* output, const XMLCh* const input) {
	char* handoff = XMLString::transcode(input);
	strncpy(output, handoff, maxCharLength);
	XMLString::release(&handoff);
}

// called at beginning of new XML doc
void C2SIMHandler::startDocument()
{
}

// called at end of parsing XML doc
void C2SIMHandler::endDocument()
{
}

void C2SIMHandler::startElement(const XMLCh* const name, 
	AttributeList& attributes)
{
	// copy name parameter to startTag
	C2SIMHandler::copyXMLCh(startTag, name);
	
	// copy startTag to latestTag
	strncpy(latestTag, startTag, maxCharLength);

	// check whether this element starts a new document
	if (!foundRootTag) {
		if (strncmp(startTag, rootTag, maxCharLength) == 0)
			foundRootTag = true;
	}
}// end C2SIMHandler::startElement

void C2SIMHandler::endElement(const XMLCh* const name)
{
	// copy name parameter to endTag
	C2SIMHandler::copyXMLCh(endTag, name);

	// check whether it is the closing mirror of root tag
	if (!foundFinalTag) {
		if (strncmp(endTag, rootTag, maxCharLength) == 0)
			foundFinalTag = true;
	}

	// delete latestTag
	latestTag[0] = '\0';


}// end C2SIMHandler::endElement


// character data
// TODO: per documentation, this could come in pieces; 
// all pieces should be collected before acting on it
void C2SIMHandler::characters(
	const XMLCh* const data,
	const XMLSize_t dataLength) {
	C2SIMHandler::copyXMLCh(dataText, data);

	// check for overlength data - warn if found
	if (dataLength > maxCharLength)
		std::cout << "WARNING data length " << dataLength << " over config limit of " <<
		maxCharLength << " data truncated begins:" << dataText << "\n";
	
	// look for TaskersIntent, DateTime, Latitude, Longitude, ElevationAGL, and UnitID tags and collect their data
	if (!foundRootTag || foundFinalTag || *latestTag == '\0')return;
	if (!foundTaskersIntent && strncmp(latestTag, "TaskersIntent", maxCharLength) == 0) {
		foundTaskersIntent = true;
		strncpy(taskersIntent, dataText, maxCharLength);
	}
	else if (!foundUnitID && strncmp(latestTag, "UnitID", maxCharLength) == 0) {
		foundUnitID = true;
		strncpy(unitID, dataText, maxCharLength);
	}
	else if (!foundLatitude1 && strncmp(latestTag, "Latitude", maxCharLength) == 0) {
		foundLatitude1 = true;
		strncpy(latitude1, dataText, maxCharLength);
	}
	else if (!foundLongitude1 && strncmp(latestTag, "Longitude", maxCharLength) == 0) {
		foundLongitude1 = true;
		strncpy(longitude1, dataText, maxCharLength);
	}
	else if (!foundElevationAgl1 && strncmp(latestTag, "ElevationAGL", maxCharLength) == 0) {
		foundElevationAgl1 = true;
		strncpy(elevationAgl1, dataText, maxCharLength);
	}
	else if (!foundLatitude2 && strncmp(latestTag, "Latitude", maxCharLength) == 0) {
		foundLatitude2 = true;
		strncpy(latitude2, dataText, maxCharLength);
	}
	else if (!foundLongitude2 && strncmp(latestTag, "Longitude", maxCharLength) == 0) {
		foundLongitude2 = true;
		strncpy(longitude2, dataText, maxCharLength);
	}
	else if (!foundElevationAgl2 && strncmp(latestTag, "ElevationAGL", maxCharLength) == 0) {
		foundElevationAgl2 = true;
		strncpy(elevationAgl2, dataText, maxCharLength);
	}
	else if (!foundLatitude3 && strncmp(latestTag, "Latitude", maxCharLength) == 0) {
		foundLatitude3 = true;
		strncpy(latitude3, dataText, maxCharLength);
	}
	else if (!foundLongitude3 && strncmp(latestTag, "Longitude", maxCharLength) == 0) {
		foundLongitude3 = true;
		strncpy(longitude3, dataText, maxCharLength);
	}
	else if (!foundElevationAgl3 && strncmp(latestTag, "ElevationAGL", maxCharLength) == 0) {
		foundElevationAgl3 = true;
		strncpy(elevationAgl3, dataText, maxCharLength);
	}
	else if (!foundDateTime && strncmp(latestTag, "DateTime", maxCharLength) == 0) {
		foundDateTime = true;
		strncpy(dateTime, dataText, maxCharLength);
	}
	else dataText[0] = '\0';

}// end C2SIMHandler::characters

void C2SIMHandler::fatalError(const SAXParseException& exception)
{
	char* message = XMLString::transcode(exception.getMessage());
	std::cout << "Fatal Error: " << message <<
		" at line: " << exception.getLineNumber() << "\n";
	XMLString::release(&message);
}
