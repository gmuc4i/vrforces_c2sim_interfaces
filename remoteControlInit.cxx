/*******************************************************************************
** Copyright (c) 2005 MaK Technologies, Inc.
** All rights reserved.
*******************************************************************************/
/*******************************************************************************
** $RCSfile: remoteControlInit.cxx,v $ $Revision: 1.4 $ $State: Exp $
*******************************************************************************/

#include "remoteControlInit.h"

#include <matrix/vlVector.h>
#include <matrix/geodeticCoord.h>

//Constructor -- Initialize the base class (will initialize protocol-specific
//parameters), command line arguments and mtl parameters
DtRemoteControlInitializer::DtRemoteControlInitializer(int argc, char* argv[]) :
   DtVrlApplicationInitializer(argc, argv, "remoteControl")
#if DtHLA
   , mySiteId(myHlaConnection, "backendSiteId", 1, "Site ID of the back-end", DtBaseConfigVariable::DtScopeBoth, false, "s")
   , myApplicationNumber(myHlaConnection, "b&ackendAppNum", 2, "Application number of the back-end")
#endif
{
   initMtl();
   initCmdLine();
}

DtRemoteControlInitializer::~DtRemoteControlInitializer()
{
}

//Registers the MTL parameters
void DtRemoteControlInitializer::initMtl()
{
#if DtHLA
   myMtlEnv->registerConfigVar(myApplicationNumber);
   myMtlEnv->registerConfigVar(mySiteId);   
#endif
}

//Initializes the command line parameters
void DtRemoteControlInitializer::initCmdLine()
{
}

//--------------------------------------------------------------
//   ACCESSORS/MODIFIERS
//   Provide a way to inspect/modify the data
//--------------------------------------------------------------

#if DtHLA
void DtRemoteControlInitializer::setApplicationNumber(int num)
{
   myApplicationNumber.setValue(num);
}

int DtRemoteControlInitializer::applicationNumber() const
{
   return myApplicationNumber.value();
}

void DtRemoteControlInitializer::setSiteId(int id)
{
   mySiteId.setValue(id);
}

int DtRemoteControlInitializer::siteId() const
{
   return mySiteId.value();
}
#endif

