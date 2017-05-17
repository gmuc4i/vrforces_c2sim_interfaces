

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
char dateTime[100];
char latitude[100];
char longitude[100];
char unitID[100];
bool foundDateTime = false;
bool foundLatitude = false;
bool foundLongitude = false;
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
	dateTime[0] = '\0';
	latitude[0] = '\0';
	longitude[0] = '\0';
	unitID[0] = '\0';
}

// destructor
C2SIMHandler::~C2SIMHandler()
{
}

// called by C2SIMinterface to start parse of C2SIM document type
void C2SIMHandler::startC2SIMParse(const char* newRootTag)
{	
	// accept root tag for this parse
	strncpy(rootTag, newRootTag, maxCharLength);

	// reset logical flags
	foundRootTag = false;
	foundFinalTag = false;
	foundDateTime = false;
	foundLatitude = false;
	foundLongitude = false;
	foundUnitID = false;
}

// give the parsed values back on request
void C2SIMHandler::returnData(
	char* returnDateTime,
	char* returnLatitude,
	char* returnLongitude,
	char* returnUnitID) {
	strncpy(returnDateTime, dateTime, maxCharLength);
	strncpy(returnLatitude, latitude, maxCharLength);
	strncpy(returnLongitude, longitude, maxCharLength);
	strncpy(returnUnitID, unitID, maxCharLength);
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
// TODO: per dpcumentation this could come in pieces 
// so should be collected before acting on it debugx
void C2SIMHandler::characters(
	const XMLCh* const data,
	const XMLSize_t dataLength) {
	C2SIMHandler::copyXMLCh(dataText, data);
	//std::cout << "TEXT:" << dataText;//debugx
	//std::cout << "| LENGTH:" << dataLength << "\n";//debugx

	// check for overlength data - warn if found
	if (dataLength > maxCharLength)
		std::cout << "WARNING data length " << dataLength << " over config limit of " <<
		maxCharLength << " data truncated begins:" << dataText << "\n";
	
	// look for DateTime, Latitude, Longitude, and UnitID tags and collect their data
	if (!foundRootTag || foundFinalTag || *latestTag == '\0')return;
	if (!foundDateTime && strncmp(latestTag, "DateTime", maxCharLength) == 0) {
		foundDateTime = true;
		strncpy(dateTime, dataText, maxCharLength);
	}
	else if (!foundLatitude && strncmp(latestTag, "Latitude", maxCharLength) == 0) {
		foundLatitude = true;
		strncpy(latitude, dataText, maxCharLength);
	}
	else if (!foundLongitude && strncmp(latestTag, "Longitude", maxCharLength) == 0) {
		foundLongitude = true;
		strncpy(longitude, dataText, maxCharLength);
	}
	else if (!foundUnitID && strncmp(latestTag, "UnitID", maxCharLength) == 0) {
		foundUnitID = true;
		strncpy(unitID, dataText, maxCharLength);
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
