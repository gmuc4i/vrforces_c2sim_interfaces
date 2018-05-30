/*******************************************************************************
** Copyright (c) 2002 MAK Technologies, Inc.
** All rights reserved.
*******************************************************************************/
/*******************************************************************************
** $RCSfile: main.cxx,v $ $Revision: 1.13 $ $State: Exp $
*******************************************************************************/
//** MODIFIED 17May2018 by GMU C4I & Cyber Lab for c2simVRF v 1.1

// command line: --disVersion 7 --deviceAddress 127.0.0.1 --disPort 3000 -a 3001 -s 1 -x 1

#include <iostream>
#include "textIf.h"
#include "remoteControlInit.h"

// VR-Forces headers
#include <vrfMsgTransport/vrfMessageInterface.h>
#include <vrfcontrol/vrfRemoteController.h>

// IP address and port of C2SIM (BML) server
std::string serverAddress = "10.2.10.30";
std::string stompPort = "61613"; // standard server STOMP port
std::string restPort = "8080";
std::string clientId = "vrforces";
std::string skipInitialize = "0";
std::string useIbml = "0";

// provides STOMP interface and build VRForces commands
#include "C2SIMinterface.h"

// Thread functionality
#include <thread>

static int timesCalled = 0;

int main(int argc, char** argv)
{
	// extract our 5 arguments: 
	// IP address, REST port number, STOMP port number, clientID, skipInitialize
	std::cout << "Starting VR-Forces C2SIM Interface\n";
	if (argc < 2)
		std::cout << "using default server IP address:" << serverAddress << "\n";
	else
		serverAddress = argv[1];
	if (argc < 3)
		std::cout << "using default REST port:" << restPort << "\n";
	else
		restPort = argv[2];
	if (argc < 4)
		std::cout << "using default STOMP port:" << stompPort << "\n";
	else
		stompPort = argv[3];
	if (argc < 5)
		std::cout << "using default client ID:" << clientId << "\n";
	else
		clientId = argv[4];
	if (argc < 6)
		std::cout << "defaulting to require initialization sequence\n";
	else
		skipInitialize = argv[5];
	if (argc < 7)
		std::cout << "defaulting to C2SIMv0.6.8 schema\n";
	else
		useIbml = argv[6];

	// arguments for VRForces
	char* vrfArgv[13] =
		{ "bin64\\c2simVRF", "--disVersion", "7", "--deviceAddress", "127.0.0.1",
		"--disPort", "3000", "-a", "3002", "-s", "1", "-x", "1" };
	std::cout << "VR-Forces arguments:";
	for (int i = 1; i < 13; i+=2)std::cout << vrfArgv[i] << " " << vrfArgv[i+1] << "|";
	std::cout << "\n";
	
	// Create initializer object used to provide initialization data to the
	// exercise connection, configured through command line arugments.
	DtRemoteControlInitializer appInitializer(13, vrfArgv);
	appInitializer.parseCmdLine();
   
	// Create the controller
	DtVrfRemoteController* controller = new DtVrfRemoteController();
 
	// Create an exercise connection
	DtExerciseConn* exConn = new DtExerciseConn(appInitializer);
	controller->init(exConn);

	// Create a C2SIM controller to read orders and translate them for VRF
	C2SIMinterface* c2simInterface =
		new C2SIMinterface(controller, serverAddress, stompPort,
			useIbml == "1");

	// Create our text interface so we can enter text commands.
	DtTextInterface* textIf = 
		new DtTextInterface(controller, serverAddress, restPort, clientId,
			useIbml == "1");

	// Start a thread to read from STOMP and generate VRForces commands
	std::thread t1(&C2SIMinterface::readStomp, textIf, c2simInterface,
		skipInitialize == "1", clientId);

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
