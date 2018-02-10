/*******************************************************************************
** Copyright (c) 2014 MAK Technologies, Inc.
** All rights reserved.
*******************************************************************************/

#ifndef DtTextInterface_H_
#define DtTextInterface_H_

#include <vlutil/vlUtil.h>
#include "vrfcontrol/vrfRemoteController.h"

//This class provides a text interface for controlling VR-Forces over the network.
//This interface currently supports the following actions:
//
//1. load <filename> 
//     load a scenario file

//2. save <filename>
//     save the current scenario

//3. new <db filename>
//     create a new scenario

//4. close
//     close the current scenario

//5. rewind
//     rewind the current scenario to its start state

//6. run
//     sends a message interface request to run the current scenario

//6a. simrun
//     sends a sim mgmt start PDU to run the current scenario

//7. simpause
//     sends a sim mgmt stop PDU to pause the current scenario

//7a. pause
//     sends a sim mgmt pause request to pause the current scenario

//8. list
//     list all the backends that are currently running

//9. create [ waypoint | route | tank | aggregate]
//     create a VR-Forces waypoint, route, tank, or tank aggregate

//10. delete [ <object_name> | <object_id> ]
//     delete a VR-Forces object by name or by object ID

//11. set [ plan | label | location | fuel | target | restore ]
//     set a VR-Forces object attribute

//12. task [ patrolRoute | moveToPoint | follow | wait ]
//     task a VR-Forces entity

//13. aggregate "name1" "name2" "name3"
//     aggregate 3 VR-Forces entities together

//14. timemultiplier
//     sets the time multiplier on the exercise clock

//15. quit
//     quit the application

// global callback from Doug Reece for C2SIM reports
static void reportCallback(const DtVrfObjectMessage* msg, void* usr);

class DtTextInterface
{

public:

  //Process entered text
  void readCommand();

  //Set/Get time to quit
  virtual void setTimeToQuit(bool yesNo);
  virtual bool timeToQuit() const;

  //Get remote vrf controller pointer
  virtual DtVrfRemoteController* controller() const;

public:

	//constructor
	DtTextInterface(DtVrfRemoteController* controller, std::string serverAddress);

	//destructor
	virtual ~DtTextInterface();

	int getTaskNumberCompleted();

  //Functions for parsing command strings and calling
  //the corresponding methods on DtVrfRemoteController.

  virtual void createWaypoint(char* str);
  virtual void createRoute(char* str);
  virtual void createTank(char* str);
  virtual void createAggregate(char* str);
  virtual void setPlan(char* str);
  virtual void setLabel(char* str);
  virtual void setLocation(char* str);
  virtual void setFuel(char* str);
  virtual void setTarget(char* str);
  virtual void setRestore(char* str);
  virtual void patrolRoute(char* str);
  virtual void moveToPoint(char* str);
  virtual void follow(char* str);
  virtual void wait(char* str);
  virtual void bigRoute(int iPoints);
  virtual void listSnapshots();

public:

   void DtTextInterface::spotReportCallback(//const 
		DtVrfObjectMessage* msg, void * usr);

   //-------------------------------------------------------
   //Text callback functions
   //-------------------------------------------------------

   //Called in response to "list"
   static void listCmd(char* s, DtTextInterface* a);

   //Called in response to "load"
   static void loadCmd(char* s, DtTextInterface* a);
   //Called by DtVrfRemoteController::loadScenario when back-ends are missing
   static bool missingBackends(DtBackendMap* map, void* usr);

   //Called in response to "new"
   static void newCmd(char* s, DtTextInterface* a);

   //Called in response to "save"
   static void saveCmd(char* s, DtTextInterface* a);

   //Called in response to "create"
   static void createCmd(char* s, DtTextInterface* a);

   //Called in response to "set"
   static void setCmd(char* s, DtTextInterface* a);

   //Called in response to "task"
   static void taskCmd(char* s, DtTextInterface* a);

   //Called in response to "aggregate"
   static void aggregateCmd(char* s, DtTextInterface* a);

   //Called in response to "delete"
   static void deleteCmd(char* s, DtTextInterface* a);

   //Called in response to "close"
   static void closeCmd(char* s, DtTextInterface* a);

   //Called in response to "monitor"
   static void monitorResourcesCmd(char* s, DtTextInterface* a);

   //Called in response to "unmonitor"
   static void removeMonitorResourcesCmd(char* s, DtTextInterface* a);

   //Called in response to "rewind"
   static void rewindCmd(char* s, DtTextInterface* a);

   //Called in response to "run"
   static void runCmd(char* s, DtTextInterface* a);

   //Called in response to "pause"
   static void pauseCmd(char* s, DtTextInterface* a);

   //Called in response to "simrun"
   static void simrunCmd(char* s, DtTextInterface* a);

   //Called in response to "simpause"
   static void simpauseCmd(char* s, DtTextInterface* a);

   //Called in response to "timemultipler" 
   static void timemultiplierCmd(char* s, DtTextInterface* a);

   //Called in response to "quit" or "q"
   static void quitCmd(char* s, DtTextInterface* a);

   //Called in response to "help" or "?" or "h"
   static void helpCmd(char* s, DtTextInterface* a);

   //Called in response to "help" or "?" or "h"
   static void bigRouteCmd(char* s, DtTextInterface* a);

   //Called in response to "listsnapshots"
   static void listSnapshotsCmd(char* s, DtTextInterface* a);

   //Called in response to "takesnapshot"
   static void takeSnapshotCmd(char* s, DtTextInterface* a);

   //Called in response to "turnonsnapshots"
   static void turnOnSnapshotsCmd(char* s, DtTextInterface* a);

   //Called in response to "turnoffsnapshots"
   static void turnOffSnapshotsCmd(char* s, DtTextInterface* a);

   //Called in response to "rollback"
   static void rollbackToSnapshotCmd(char* s, DtTextInterface* a);

   //-------------------------------------------------------
   //DtVrfRemoteController callback functions
   //-------------------------------------------------------

   //Backend arrival/departure callbacks
   static void backendAddedCb(const DtSimulationAddress& addr, void* usr);
   static void backendRemovedCb(const DtSimulationAddress& addr, void* usr);

   //Backend loaded/saved callbacks
   static void backendLoadedCb(const DtSimulationAddress& addr, void* usr);
   static void backendSavedCb(const DtSimulationAddress& addr, 
      DtSaveResult result, void* usr);

   //Scenario loaded/saved callbacks
   static void ScenarioLoadedCb(void* usr);
   static void ScenarioSavedCb(void* usr);

   //VR-Forces object created callback
   static void vrfObjectCreatedCb(
      const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid, void* usr);

   //Plan callbacks 
   static void planCb(const DtUUID& name, 
      const std::vector<DtSimStatement*>& statements, bool append, void* usr);
   static void planStmtCb(const DtUUID& id, DtPlanStatus status, void* usr);
   static void planCompleteCb(const DtUUID& id, void* usr);

   //Callback used to process monitor resource responses
   static void resourcesProcessedCb(DtSimMessage* msg, void* usr);

   //Callback for list of current snapshots
   static void scenarioSnapshotResponseCb(DtSimMessage* msg, void* usr);

   //Callback used to receive object console messages from the remote control
   //API.
   static void objectConsoleMessageCb(const DtEntityIdentifier& id,
      int notifyLevel, const DtString& message, void* usr);

protected:

   //Present commmand line interface.
   void prompt();

public:
   //Parse user input, call corresponding function
   void processCmd(char *buff);

protected:

   bool myTimeToQuit;
   DtVrfRemoteController* myController;
   DtSelectParam sparam;
   static unsigned int theNextAggregateId;
   static unsigned int theNextSubordinateId;

private:
   int kbInterest;
};


#endif

