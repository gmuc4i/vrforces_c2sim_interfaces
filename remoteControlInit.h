/*******************************************************************************
** Copyright (c) 2005 MaK Technologies, Inc.
** All rights reserved.
*******************************************************************************/
/*******************************************************************************
** $RCSfile: remoteControlInit.h,v $ $Revision: 1.4 $ $State: Exp $
*******************************************************************************/
#ifndef _f18Init_H_
#define _f18Init_H_

#include <vl/exerciseConnInitializer.h>
#include <vlutil/vlString.h>
#include <mtl/mtlEnvironment.h>
#include <matrix/vlVector.h>

class DtMtlEnvironment;

//DtRemoteControlInitializer derives from DtVrlApplicationInitializer and
//configures command line arguments for the remoteControl application.
//DtVrlApplicationInitializer contains
//default behavior for protocol specific parameters, while 
//DtRemoteControlInitializer contains behavior particular to the remoteControl
//application.
//
//parseCmdLine() parses the command line arguments specified in
//this class and DtVrlApplicationInitializer.  loadMtlFile() can be
//called (defined in DtVrlApplicationInitializer) with an MTL file name.
//This will parse the MTL file specified and set those variables registered
class DtRemoteControlInitializer : public DtVrlApplicationInitializer
{
public:
   DtRemoteControlInitializer(int argc, char* argv[]);

   virtual ~DtRemoteControlInitializer();

   //These are parameters used by the remoteControl app that are defined in the 
   //DIS DtVrlApplicationInitializer, but not the HLA version, so define them
   //for HLA here
#if DtHLA
   virtual void setApplicationNumber(int num);
   virtual int applicationNumber() const;

   virtual void setSiteId(int id);
   virtual int siteId() const;
#endif

protected:
   //Register the MTL/Command Line parameters.  Non-virtual
   //since they are called from the constructor
   void initMtl();
   void initCmdLine();

protected:
#if DtHLA
   //This is already in the DIS base-class... For the remoteControl app,
   //make one for HLA
   DtConfigVariable<int> myApplicationNumber;
   DtConfigVariable<int> mySiteId;
#endif
};

#endif

