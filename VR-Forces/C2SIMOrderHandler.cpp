

#include "C2SIMOrderHandler.h"
#include <stdio.h>
#include <iostream>
#include "xercesc/sax/AttributeList.hpp"

using namespace xercesc;

// conversion factor
const static double degreesToRadians = 57.2957795131L;

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

// flags to indicate we've found a particular string
bool foundTaskersIntent = false;
bool foundDateTime = false;
bool foundUnitID = false;
bool foundLatitude = false;
bool foundLongitude = false;
bool foundElevation = false;
bool missingRootTag = false;
bool foundRootTag = false;
bool orderFormatIsIbml = false;
bool orderFormatIsC2sim = false;
bool foundFinalTag = false;
int latitudePointCount = 0;
int longitudePointCount = 0;
int elevationPointCount = 0;

// constructor
C2SIMOrderHandler::C2SIMOrderHandler()
{
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
		latitudes[i] = (char*)malloc(MAXCHARARRAY);
		longitudes[i] = (char*)malloc(MAXCHARARRAY);
		elevations[i] = (char*)malloc(MAXCHARARRAY);
	}
}

// destructor
C2SIMOrderHandler::~C2SIMOrderHandler()
{
}

// return order types
bool C2SIMOrderHandler::getOrderTypeIbml()
{
	return orderFormatIsIbml;
}
bool C2SIMOrderHandler::getOrderTypeC2sim()
{
	return orderFormatIsC2sim;
}

// true if orderRootTag was found in latest parse
bool C2SIMOrderHandler::orderRootTagFound()
{
	return foundRootTag;
}

// return value of taskersIntent
char* C2SIMOrderHandler::getTaskersIntent()
{
	return taskersIntent;
}

// return value of dataTime
char* C2SIMOrderHandler::getDateTime()
{
	return dateTime;
}

// return value of unitId
char* C2SIMOrderHandler::getUnitId()
{
	return unitId;
}

// return array of coordinate points
int C2SIMOrderHandler::getRoutePoints(double x[], double y[], double z[])
{
	// find smallest count of lat/lon/elev points
	int pointCount = latitudePointCount;
	if (longitudePointCount < pointCount)pointCount = longitudePointCount;
	if (orderFormatIsIbml && elevationPointCount < pointCount)pointCount = elevationPointCount;
	if (pointCount == 0)return pointCount;
	
	// if there are no elevations make them zero
	if(elevationPointCount == 0)
		for (int i = 0; i < pointCount; ++i)
			elevations[i] = "0";

	// convert input geodetic coordiantes to geocentric to return
	for (int i = 0; i < pointCount; ++i) {
		DtGeodeticCoord geod(
			std::stod(latitudes[i]) / degreesToRadians,
			std::stod(longitudes[i]) / degreesToRadians,
			std::stod(elevations[i]));
		DtVector geoc = geod.geocentric();

		// check for converted elevation below MSL
		DtGeodeticCoord checkGeod;
		checkGeod.setGeocentric(geoc);
		if (checkGeod.alt() < 0.0e0) {
			std::cout << "negative elevation when recoding coordinate " << i << " (" <<
				latitudes[i] << "," << longitudes[i] << "," << elevations[i] << ")\n";
			DtGeodeticCoord hackGeod(
				std::stod(latitudes[i]) / degreesToRadians,
				std::stod(longitudes[i]) / degreesToRadians,
				5.0e0);
			geoc = geod.geocentric();
		}

		// return geocentric arrays for the points
		x[i] = geoc.x();
		y[i] = geoc.y();
		z[i] = geoc.z();
	}
	return pointCount;
}


// called by C2SIMinterface to start parse of C2SIM document type
void C2SIMOrderHandler::startC2SIMParse()
{
	// reset logical flags
	missingRootTag = false;
	foundRootTag = false;
	orderFormatIsIbml = false;
	orderFormatIsC2sim = false;
	foundFinalTag = false;
	foundTaskersIntent = false;
	foundDateTime = false;
	foundUnitID = false;
	latitudePointCount = 0;
	longitudePointCount = 0;
	elevationPointCount = 0;
}

// transcode XMLCh* to char*, copy that to char* output 
// then release the transcoded copy
// this seems inefficient but its too hard to
// keep track of and release multiple encodings
void C2SIMOrderHandler::copyXMLCh(char* output, const XMLCh* const input) {
	char* handoff = XMLString::transcode(input);
	strncpy(output, handoff, MAXCHARLENGTH);
	XMLString::release(&handoff);
}

// called at beginning of new XML doc
void C2SIMOrderHandler::startDocument()
{
}

// called at end of parsing XML doc
void C2SIMOrderHandler::endDocument()
{
}

void C2SIMOrderHandler::startElement(const XMLCh* const name,
	AttributeList& attributes)
{
	// copy name parameter to startTag
	C2SIMOrderHandler::copyXMLCh(startTag, name);
	
	// copy startTag to latestTag
	strncpy(latestTag, startTag, MAXCHARLENGTH);

}// end C2SIMOrderHandler::startElement


void C2SIMOrderHandler::endElement(const XMLCh* const name)
{
	// copy name parameter to endTag
	C2SIMOrderHandler::copyXMLCh(endTag, name);

	// check whether it is the closing mirror of root tag
	if (!foundFinalTag) {
		if (orderFormatIsIbml && strncmp(endTag, ibmlOrderRootTag, MAXCHARLENGTH) == 0)
			foundFinalTag = true;
		else if (orderFormatIsC2sim && strncmp(endTag, c2simOrderRootTag, MAXCHARLENGTH) == 0)
			foundFinalTag = true;
	}

	// delete latestTag
	latestTag[0] = '\0';

}// end C2SIMOrderHandler::endElement


 // character data
 // TODO: per documentation, this could come in pieces; 
 // all pieces should be collected before acting on it
void C2SIMOrderHandler::characters(
	const XMLCh* const data,
	const XMLSize_t dataLength) {
	C2SIMOrderHandler::copyXMLCh(dataText, data);

	// check for overlength data - warn if found
	if (dataLength > MAXCHARLENGTH)
		std::cout << "WARNING data length " << dataLength << " over config limit of " <<
		MAXCHARLENGTH << " data truncated begins:" << dataText << "\n";

	// first tag offered should be root tag - determine whether IBML or C2SIM
	if (missingRootTag)return;
	if (!foundRootTag) {
		if (strncmp(latestTag, ibmlOrderRootTag, MAXCHARLENGTH) == 0) {
			foundRootTag = true;
			orderFormatIsIbml = true;
			std::cout << "received IBML order - will report in IBML format\n";
			return;
		}
		else if (strncmp(latestTag, c2simOrderRootTag, MAXCHARLENGTH) == 0) {
			foundRootTag = true;
			orderFormatIsC2sim = true;
			std::cout << "received C2SIM order - will report in C2SIM format\n";
			return;
		}
		else missingRootTag = true;
	}

	// if parse conditions not met just return
	if (!foundRootTag || foundFinalTag || *latestTag == '\0')return;

	// look for order element tags and copy their data one tag at a time
	//std::cout << "TAG:" << latestTag << "|  DATA:" << dataText << "|\n";

	// look for tastkersIntent
	if (!foundTaskersIntent) {
		if (orderFormatIsIbml && strncmp(latestTag, ibmlTaskersIntentTag, MAXCHARLENGTH) == 0) {
			foundTaskersIntent = true;
			strncpy(taskersIntent, dataText, MAXCHARLENGTH);
			//std::cout << "PARSE TaskersIntent:" << taskersIntent << "\n";
			return;
		}
	}

	// look for UnitId
	if (!foundUnitID) {
		if (orderFormatIsIbml && strncmp(latestTag, ibmlUnitIdTag, MAXCHARLENGTH) == 0) {
			foundUnitID = true;
			strncpy(unitId, dataText, MAXCHARLENGTH);
			//std::cout << "PARSE IBML09 UnitID:" << unitId << "\n";
			return;
		}
		else if (orderFormatIsC2sim && strncmp(latestTag, c2simUnitIdTag, MAXCHARLENGTH) == 0) {
			foundUnitID = true;
			strncpy(unitId, dataText, MAXCHARLENGTH);
			std::cout << "PARSE C2SIM UnitID:" << unitId << "\n";
			return;
		}
	}

	// look for dateTime
	if (!foundDateTime) {
		if (strncmp(latestTag, ibmlDateTimeTag, MAXCHARLENGTH) == 0) {
			foundDateTime = true;
			strncpy(dateTime, dataText, MAXCHARLENGTH);
			//std::cout << "PARSE DateTime:" << dateTime << "\n";
			return;
		}
		else if (strncmp(latestTag, c2simDateTimeTag, MAXCHARLENGTH) == 0) {
			foundDateTime = true;
			strncpy(dateTime, dataText, MAXCHARLENGTH);
			//std::cout << "PARSE DateTime:" << dateTime << "\n";
			return;
		}
	}

	// look for latitude of next point
    if (strncmp(latestTag, latitudeTag, MAXCHARLENGTH) == 0) {
		if (latitudePointCount >= MAXPOINTS) {
			std::cout << "too many latitudes; deleted " << latitudeTag << "\n";
			return;
		}
		//std::cout << "PARSE latitude " << latitudePointCount << ":" << dataText << "\n";
		strncpy(latitudes[latitudePointCount++], dataText, MAXCHARLENGTH);
		return;
	}

	// look for longitude of next point
	if (strncmp(latestTag, longitudeTag, MAXCHARLENGTH) == 0) {
		if (longitudePointCount >= MAXPOINTS) {
			std::cout << "too many longitudes; deleted " << longitudeTag << "\n";
			return;
		}
		//std::cout << "PARSE longitude " << longitudePointCount << ":" << dataText << "\n";
		strncpy(longitudes[longitudePointCount++], dataText, MAXCHARLENGTH);
		return;
	}

	// look for elevation of next point
	if (strncmp(latestTag, ibmlElevationTag, MAXCHARLENGTH) == 0) {
		if (elevationPointCount >= MAXPOINTS) {
			std::cout << "too many longitudes; deleted " << ibmlElevationTag << "\n";
			return;
		}
		//std::cout << "PARSE elevation " << elevationPointCount << ":" << dataText << "\n";
		strncpy(elevations[elevationPointCount++], dataText, MAXCHARLENGTH);
		return;
	}

	// must be some tag we don't care about
	dataText[0] = '\0';

}// end C2SIMOrderHandler::characters

void C2SIMOrderHandler::fatalError(const SAXParseException& exception)
{
	char* message = XMLString::transcode(exception.getMessage());
	std::cout << "Fatal Error: " << message <<
		" at line: " << exception.getLineNumber() << "\n";
	XMLString::release(&message);
}
