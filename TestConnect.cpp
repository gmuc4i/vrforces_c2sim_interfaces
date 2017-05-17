//This example demonstrates how to use the DtCgf class to create objects
//and task entities, as you would if you were to embed VR-Forces in your own 
//application.
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif
//VR-Forces Headers
#include "vrfcgf/cgf.h"
#include "vrfcgf/cgfDispatcher.h"
#include "vrfcore/simManager.h"
#include "vrftasks/moveToTask.h"
#include "vrfobjcore/vrfObject.h"
//VR-Link headers
#include <vl/exerciseConn.h>
#include <vl/exerciseConnInitializer.h>
#include <vlutil/vlPrint.h>
#include <vlutil/vlProcessControl.h>
#include <vl/hostStructs.h>
#include "vrfutil/vrfVlKeyboard.h"
int main(int argc, char *argv[])
{
	//DIS/RPR simulation address
	DtSimulationAddress simAddress(1, 3001);
	//Create the CGF
	DtCgf* cgf = new DtCgf(simAddress);
	// The following is left so the user can see how to piece together a 
	// simple DIS or HLA exercise connection. Also so you know what execName to 
	// pass for HLA for the -x command. Use this and you don't need anything else, it will use 
	// a default fed file of VR-Link.fed(for HLA13) or VR-Link.xml( for HLA1516/1516e) However to make life easier we're also 
	// going to show you how to use an DtVrlApplicationInitializer so you can parse the command line as well
	//Create the exercise connection
	/*#if DtHLA
	DtString execName           = "VR-Link";     //HLA federation execution name
	DtString federateName       = "simpleCgf";   //HLA federate name
	double rprFomVersion        = 1.0;           //RPR FOM version number
	DtExerciseConn* exerciseConn = new DtExerciseConn(execName, federateName,
	new DtRprFomMapper(rprFomVersion));
	#else //DIS
	int port = 3928;                             //UDP port number
	int exerciseId = 1;                          //DIS exercise ID
	DtExerciseConn* exerciseConn = new DtExerciseConn(port, DtInetAddr("127.255.255.255"),exerciseId,
	simAddress.siteId(), simAddress.applicationId());
	#endif*/
	// Use this to parse the command line then pass the initializer to the exercise connection
	DtVrlApplicationInitializer initializer(argc, argv, "simpleCgf");
#if DtDIS
	initializer.setDisVersionToSend(7);
#endif
	initializer.parseCmdLine();
#if DtDIS
	initializer.setSiteId(simAddress.siteId());
	initializer.setApplicationNumber(simAddress.applicationId());
#endif
	DtExerciseConn* exerciseConn = new DtExerciseConn(initializer);
	//Initialize the CGF
	cgf->init(exerciseConn);
	//Create the dispatcher so this application can interface with
	//front-end
	DtCgfDispatcher* cgfDispatcher = new DtCgfDispatcher(cgf,
		cgf->simManager()->messageInterface());
	//Create a new scenario, using the Ground-db.gdb terrain database and the EntityLevel
	//simulation model set to allow for entity creation
	cgf->newScenario("../userData/terrains/Ground-db.mtf",
		"../data/simulationModelSets/EntityLevel.sms");
	//Tick the cgf to get the terrain loaded
	cgf->tick();

	//Create a tank
	DtEntityType tankType(1, 1, 225, 1, 1, 3, 0);
	DtVector initialPosition(900.0, 900.0, 0.0); //Database coordinates, meters
	DtReal initialHeading = 0.0;                 //Heading in radians
												 //At this point the simulation has not yet been started, so when creating
												 //entities it's best to call createEntity without specifying the
												 //"allowBlocking" parameter to allow it default to true. This will insure
												 //that when using a paging terrain, the necessary pages will immediately
												 //get paged in.
	DtVrfObject* tank = cgf->createEntity(
		tankType,
		DtForceFriendly,
		initialPosition,
		initialHeading);
	//Create a waypoint named "Alpha"

	DtVrfObject* alpha = cgf->createWaypoint(
		DtVector(1300.0, 1000.0, 0.0),
		"Alpha");
	//Tell the tank to move to waypoint "Alpha"
	DtMoveToTask moveToTask;
	moveToTask.setControlPoint(alpha->uuid());
	cgf->assignTask(tank, moveToTask);
	//Put the simulation engine into run mode.
	cgf->run();
	//Used below to insure only one example entity is created at runtime
	bool entityCreatedAtRuntime = false;
	//Main application loop
	while (1)
	{
		//tick the CGF Dispatcher
		cgfDispatcher->tick();

		//tick the CGF
		cgf->tick();
		//Poll & process keyboard input
		char *keyPtr = DtGetInputLine();
		if (keyPtr && *keyPtr == 'q')
		{
			break;
		}
		//Give up the remainder of our time slice.
		DtSleep(0.01);

		//This is an example of creating an entity at runtime
		if (!entityCreatedAtRuntime)
		{
			//Change the initial position so this entity isn't created on top of
			//the first one
			initialPosition.setX(300);
			//In this case when calling createEntity, the allowBlocking flag must
			//be set to false. If the entity being created requires a new tile to
			//be swapped in, the simulation can't block while it gets loaded in
			//without having a negative effect on the models.
			cgf->createEntity(tankType, DtForceFriendly, initialPosition,
				initialHeading, DtString::nullString(), false);
			//reset the flag so only one entity gets created
			entityCreatedAtRuntime = true;
		}
	}
	delete cgf;
	delete exerciseConn;
	return 0;
}