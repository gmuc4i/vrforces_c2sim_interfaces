/*******************************************************************************
** Copyright (c) 2002 MAK Technologies, Inc.
** All rights reserved.
*******************************************************************************/
/*******************************************************************************
** $RCSfile: main.cxx,v $ $Revision: 1.13 $ $State: Exp $
*******************************************************************************/
//** MODIFIED 8Feb2018 by GMU C4I & Cyber Lab 

// alternate command line: --disVersion 7 --deviceAddress 127.0.0.1 --disPort 3000 -a 3001 -s 1 -x 1

#include <iostream>
#include "textIf.h"
#include "remoteControlInit.h"

// VR-Forces headers
#include <vrfMsgTransport/vrfMessageInterface.h>
#include <vrfcontrol/vrfRemoteController.h>

// VR-Link headers
#include <vl/exerciseConn.h>
#include <vlutil/vlProcessControl.h>

// IP address of C2SIM (BML) server
std::string serverAddress = "10.0.0.69";// "10.2.10.30";

// provides STOMP interface and build VRForces commands
#include "C2SIMinterface.h"

// Thread functionality
#include <thread>

static int timesCalled = 0;

int main(int argc, char** argv)
{
   std::cout << "Starting VR-Forces C2SIM Interface\n";
   std::cout << argc << " " << argv[1];
   std::cout << "Terrain:" << argv[2];
   
   //Create initializer object used to provide initialization data to the
   //exercise connection, configured through command line arugments.
   DtRemoteControlInitializer appInitializer(argc, argv);
   appInitializer.parseCmdLine();
   
   //Create the controller
   DtVrfRemoteController* controller = new DtVrfRemoteController();
 
   //Create an exercise connection
   DtExerciseConn* exConn = new DtExerciseConn(appInitializer);
   controller->init(exConn);

   // create a C2SIM controller to read orders and translatre them for VRF
   C2SIMinterface* c2simInterface =
	   new C2SIMinterface(controller, serverAddress);

   //Create our text interface so we can enter text commands.
   DtTextInterface* textIf = 
	   new DtTextInterface(controller, serverAddress);

   // start a thread to read from STOMP and generate VRForces commands
   std::thread t1(&C2SIMinterface::readStomp, textIf, c2simInterface);

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
