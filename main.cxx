/*******************************************************************************
** Copyright (c) 2002 MAK Technologies, Inc.
** All rights reserved.
*******************************************************************************/
/*******************************************************************************
** $RCSfile: main.cxx,v $ $Revision: 1.13 $ $State: Exp $
*******************************************************************************/

#include "textIf.h"
#include "remoteControlInit.h"

// VR-Forces headers
#include <vrfMsgTransport/vrfMessageInterface.h>
#include <vrfcontrol/vrfRemoteController.h>

// VR-Link headers
#include <vl/exerciseConn.h>
#include <vlutil/vlProcessControl.h>

// provides STOMP interface and build VRForces commands
#include "C2SIMInterface.h"

// Thread functionality
#include <thread>

static int timesCalled = 0;

int main(int argc, char** argv)
{
   //Create initializer object used to provide initialization data to the
   //exercise connection, configured through command line arugments.
   DtRemoteControlInitializer appInitializer(argc, argv);
   appInitializer.parseCmdLine();
   
   //Create the controller
   DtVrfRemoteController* controller = new DtVrfRemoteController();

   //Create an exercise connection
   DtExerciseConn* exConn = new DtExerciseConn(appInitializer);
   controller->init(exConn);

   //Create our text interface so we can enter text commands.
   DtTextInterface* textIf = new DtTextInterface(controller);

   // start a thread to read from STOMP and generate VRForces commands
   C2SIMinterface* c2siminterface = new C2SIMinterface();
   std::thread t1(&C2SIMinterface::readStomp, controller, textIf);

   /*Processing: read stdin and call drainInput.
     Calling drainInput() ensures that the controller receives
     necessary feedback from VR-Forces backend applications.
	*/
   
   while (!textIf->timeToQuit())
   { 
	  textIf->readCommand();
      exConn->clock()->setSimTime(exConn->clock()->elapsedRealTime());
      exConn->drainInput();
      DtSleep(0.1);
   }

   // shutdown
   t1.join();
   delete textIf;
   delete controller;
   delete exConn;

   return 0;
}
