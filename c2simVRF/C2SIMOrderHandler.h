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


class C2SIMOrderHandler : public HandlerBase
{
	// globals

	// size of strings and arrays
#define MAXCHARLENGTH 99 // somewhat arbitrary
#define MAXCHARARRAY 100
#define MAXPOINTS 100 // points in route

	// description of XML order
	char* orderRootTag = "OrderPushIBML";
	char* taskersIntentTag = "TaskersIntent";
	char* dateTimeTag = "DateTime";
	char* unitIdTag = "UnitID";
	char* latitudeTag = "Latitude";
	char* longitudeTag = "Longitude";
	char* elevationTag = "ElevationAGL";

public:
	C2SIMOrderHandler();
	~C2SIMOrderHandler();
	void startC2SIMParse();
	char* C2SIMOrderHandler::getTaskersIntent();
	char* C2SIMOrderHandler::getDateTime();
	char* C2SIMOrderHandler::getUnitId();
	int C2SIMOrderHandler::getRoutePoints(double x[], double y[], double z[]);
	bool C2SIMOrderHandler::orderRootTagFound();
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