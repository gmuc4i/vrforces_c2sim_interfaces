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

#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/sax/AttributeList.hpp"
#include <matrix/geodeticCoord.h>

using namespace xercesc;

// size of strings and arrays
#define MAXCHARLENGTH 99 // somewhat arbitrary
#define MAXCHARARRAY 100
#define MAXPOINTS 100   // points in route
#define MAXINIT 100     // Units in MilitaryOrganzation xml
#define MAXTASKS 50     // Tasks in an order

// data from a Military_Organization Unit
struct Unit {
	char* id;
	char* name;
	char* opStatusCode;
	char* strengthPercent;
	char* hostilityCode;
	char* echelon;
	char* superiorUnit;
	char* latitude;
	char* longitude;
	char* elevationAgl;
	char* symbolId;
	char* forceSide;
	char* submitter;
};

// data from an order (C2SIM_Order or OrderPushIBML)
struct Task {
	char* performingEntity;
	char* unitId;
	char* dateTime;
	std::string latitudes[MAXPOINTS];
	std::string longitudes[MAXPOINTS];
	std::string elevations[MAXPOINTS];
	int latitudePointCount;
	int longitudePointCount;
	int elevationPointCount;
};

class C2SIMxmlHandler : public HandlerBase
{
	// globals

	// description of XML order
	char* c2simMessageTag = "C2SIM_Message";
	char* ibmlOrderRootTag = "OrderPushIBML";
	char* c2simOrderRootTag = "C2SIM_Order";
	char* militaryOrgRootTag = "C2SIM_MilitaryOrganization";
	char* taskTag = "Task";
	char* ibmlTaskersIntentTag = "TaskersIntent";
	char* ibmlDateTimeTag = "DateTime";
	char* c2simDateTimeTag = "StartTime";
	char* ibmlUnitIdTag = "UnitID";
	char* c2simPerformingEntityTag = "PerformingEntity";
	char* c2simUnitIdTag = "Name";
	char* latitudeTag = "Latitude";
	char* longitudeTag = "Longitude";
	char* elevationTag = "ElevationAGL";

	// description of xml Military_Organization
	char* initUnitTag = "Unit";
	char* initIdTag = "ID";
	char* initNameTag = "Name";
	char* initOpStatusTag = "OperationalStatusCode";
	char* initStrengthTag = "StrengthPercentage";
	char* initHostilityTag = "HostilityCode";
	char* initEchelonTag = "echelon";
	char* initSuperiorUnittag = "SuperiorUnit";
	char* initLatitudeTag = "Latitude";
	char* initLongitudeTag = "Longitude";
	char* initElevationAglTag = "ElevationAGL";
	char* initSymbolIdTag = "SymbolIdentifier";
	char* initForceSideTag = "ForceSide";
	char* initSubmitterTag = "Submitter";

	// description of C2SIM_ControlMessage
	char* controlRootTag = "C2SIM_Simulation_Control";
	char* controlStateTag = "state";

	// C2SIM reports to ignore
	char* c2simReportRootTag = "CWIX_Position_Report";
	char* ibmlReportRootTag = "BMLReport";

public:
	C2SIMxmlHandler(bool useIbmlRef);
	~C2SIMxmlHandler();
	void startC2SIMParse(Unit* units, Task* tasks);
	char* C2SIMxmlHandler::getTaskersIntent();
	char* C2SIMxmlHandler::getDateTime();
	char* C2SIMxmlHandler::getUnitId();
	void C2SIMxmlHandler::assembleRoutePoints();
	std::string C2SIMxmlHandler::getRootTag();
	bool C2SIMxmlHandler::getOrderTypeIbml();
	bool C2SIMxmlHandler::getOrderTypeC2sim();
	int C2SIMxmlHandler::getNumberOfUnits();
	int C2SIMxmlHandler::getNumberOfTasks();
	char* C2SIMxmlHandler::getControlState();
	void startElement(const XMLCh* const, AttributeList&);
	void fatalError(const SAXParseException&);
	void startDocument();
	void endDocument();
	void characters(const XMLCh* const, const XMLSize_t);
	void endElement(const XMLCh* const name);
	void setRootTag(const char*);

private:
	void copyXMLCh(char* output, const XMLCh* const input);
};