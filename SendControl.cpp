#include <vl/dataInteraction.h>
#include <vrvControl/DtVrlinkDataInterMarshaller.h>
#include <vrvControl/vrvControlToolkitDataTypes.h>
#include <vl/exerciseConn.h>
#include <vl/exerciseConnInitializer.h>
#include <vlutil/vlKeyboard.h>
#include <stdio.h>
#include <iostream>



#include "remoteControlInit.h"

//VR-Forces headers
#include <vrfMsgTransport/vrfMessageInterface.h>
#include <vrfcontrol/vrfRemoteController.h>

//VR-Link headers
#include <vlutil/vlProcessControl.h>


using std::cout;
using std::endl;
using namespace vrvControlToolkit;
DtTime dt = 0.05;
int keyTick()
{
	char *keyPtr = DtPollBlockingInputLine();
	if (keyPtr && (*keyPtr == 'q' || *keyPtr == 'Q'))
	{
		return -1;
	}
	if (keyPtr && (*keyPtr == 'w' || *keyPtr == 'W'))
	{
		return 1;
	}
	return 0;
}
int main(int argc, char** argv)
{

	//Initial wireframe state
	bool wireframeState = false;

	// Initialize the application and connect
	DtVrlApplicationInitializer appInit(argc, argv, "SendControl");
	appInit.parseCmdLine();

	DtExerciseConn* exConn = new DtExerciseConn(appInit);
	//DtExerciseConn exConn(appInit);
	exConn->setApplicationId(3, 5);
	appInit.setDisVersionToSend(7);
	
	void createEntity(const DtEntityType &type, const DtVector &geocentricPosition,
		DtForceType force, DtReal heading, const DtString &uniqueName = DtString::nullString(),
		const DtString &label = DtString::nullString(), const DtSimulationAddress &addr = DtSimSendToAll,
		bool groundClamp = true);
	
		DtEntityType tankType(1, 1, 225, 1, 1, 3, 0);
		DtVector initialPosition(900.0, 900.0, 0.0);
		DtReal initialHeading = 0.0;

	//Create the controller
	DtVrfRemoteController* controller = new DtVrfRemoteController();

	controller->init(exConn);
	//controller->loadScenario("Ground-db.mtf");
	controller->loadScenario("Ground-db.mtf");

	DtClock* clock = exConn->clock();
	DtTime simTime = 0;
	while (true)
	{
		int key = keyTick();
		if (key == -1)
		{
			break;
		}
		else if (key == 1)
		{
			//send a wireframe control PDU
			wireframeState = !wireframeState;
			DtWireframe_v1 cmd;
			cmd.myState = wireframeState;

			DtDataInteraction data;
			DtVrlinkDataInterMarshaller<DtDataInteraction> marshall;
			marshall.encode(&cmd, &data);
#ifdef DtDIS
			data.setExerciseId(1);
#endif
			exConn->send(data);
		}
		// Tell VR-Link the current value of simulation time.
		clock->setSimTime(simTime);
		// Process any incoming messages.
		exConn->drainInput();
		DtTime sleeptime = (simTime += dt) - clock->elapsedRealTime();
		DtSleep(sleeptime); // returns immediately if negative
	}
}