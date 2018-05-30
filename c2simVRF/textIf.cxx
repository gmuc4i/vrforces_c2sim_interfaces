/*******************************************************************************
** Copyright (c) 2014 MAK Technologies, Inc.
** All rights reserved.
*******************************************************************************/

// updated to VRForces4.5 by JMP 12May18 by adding DtUUID()

// from Doug Reece for reportCallback
#include "vrfmsgs/reportMessage.h"
#include "vrftasks/taskCompleteReport.h"
#include "vrftasks/textReport.h"
#include "vrfMsgTransport/radioMessageListener.h"

// from Doug Reece for spotReportCallback
#include <vrftasks/radioMessageTypes.h>
#include <vrftasks/simReportCollection.h>
#include <vrftasks/spotReport.h>
#include <vrfMsgTransport/radioMessageListener.h>
#include <vrfmsgs/reportMessage.h>

#include "textIf.h"
#include <vlutil/vlPrint.h>
#include <vl/hostStructs.h>
#include <vlpi/disEnums.h>

#include "vrfutil/backendMap.h"
#include "vrfutil/backendMapEntry.h"
#include "vlutil/vlProcessControl.h"

#include "vrfplan/plan.h"
#include "vrfplan/planBlock.h"
#include "vrfplan/ifStmt.h"
#include "vrfplan/ifBlock.h"
#include "vrfutil/ceResourceOperator.h"
#include "vrfutil/evaluator.h"
#include "vrftasks/setHeadingRequest.h"
#include "vrftasks/setSpeedRequest.h"
#include "vrftasks/moveToTask.h"
#include "vrftasks/waitDurationTask.h"
#include "vrfplan/setDataRequestStmt.h"
#include "vrfplan/taskStmt.h"
#include "vrfMsgTransport/vrfMessageInterface.h"

#include "vrfmsgs/ifResourceMonitorRequest.h"
#include "vrfmsgs/simInterfaceMessage.h"
#include "vrfmsgs/ifResourceMonitorResponse.h"
#include "vrfmsgs/ifRequestSnapshots.h"
#include "vrfmsgs/ifSnapshotsResponse.h"

#include <vl/startResumeInteraction.h>
#include <vl/stopFreezeInteraction.h>
#include "C2SIMinterface.h"
#include "RestClient.h"

#include <time.h>

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if DT_HAVE_UNISTD_H
#include <unistd.h>
#endif
#if !WIN32
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#endif
#if WIN32
#include <io.h>
#include <conio.h>
#endif


#if !WIN32
#define UNBLOCK() setBlock(0, 0)
#define RESTORE() setBlock(0, 1)
#else
#define UNBLOCK()
#define RESTORE()
#endif

bool textIfUseIbml = false;
bool simStarted = false;
void DtTextInterface::setStarted(bool value){ simStarted = value; }

const char* terrainDir = "..\\userData\\terrains\\";
const char* scenariosDir = "..\\userData\\scenarios\\";
static bool monitorResourcesCallbackAdded = false;
std::string restServerAddress;
std::string restPortNumber;
std::string clientIDCode;
static int taskNumberCompleted = 0;


//-------------------------------------------------------
//Initialize command chart
//-------------------------------------------------------

typedef void (*CmdFunc)(char *, DtTextInterface *a);

struct Cmd
{
    char *name;
    CmdFunc func;
};

Cmd allCmds[] = {
  "list",                     DtTextInterface::listCmd,
  "load",                     DtTextInterface::loadCmd,
  "new",                      DtTextInterface::newCmd,
  "save",                     DtTextInterface::saveCmd,
  "create",                   DtTextInterface::createCmd,
  "set",                      DtTextInterface::setCmd,
  "task",                     DtTextInterface::taskCmd,
  "monitor",                  DtTextInterface::monitorResourcesCmd,
  "unmonitor",                DtTextInterface::removeMonitorResourcesCmd,
  "aggregate",                DtTextInterface::aggregateCmd,
  "delete",                   DtTextInterface::deleteCmd,
  "close",                    DtTextInterface::closeCmd,
  "rewind",                   DtTextInterface::rewindCmd,
  "bigroute",                 DtTextInterface::bigRouteCmd,
  "run",                      DtTextInterface::runCmd,
  "simrun",                   DtTextInterface::simrunCmd,
  "simpause",                 DtTextInterface::simpauseCmd,
  "listsnapshots",            DtTextInterface::listSnapshotsCmd,
  "takesnapshot",             DtTextInterface::takeSnapshotCmd,
  "turnonsnapshots",          DtTextInterface::turnOnSnapshotsCmd,
  "turnoffsnapshots",         DtTextInterface::turnOffSnapshotsCmd,
  "rollback",                 DtTextInterface::rollbackToSnapshotCmd,
  "pause",                    DtTextInterface::pauseCmd,
  "timemultiplier",           DtTextInterface::timemultiplierCmd,
  "quit",                     DtTextInterface::quitCmd,
  "q",                        DtTextInterface::quitCmd,
  "help",                     DtTextInterface::helpCmd,
  "h",                        DtTextInterface::helpCmd,
  "?",                        DtTextInterface::helpCmd,
};

int nCmds = DtNUMBER(allCmds);

unsigned int DtTextInterface::theNextAggregateId = 1;
unsigned int DtTextInterface::theNextSubordinateId = 1;

// pieces of ibml general status report
std::string ibml09GSRpart1(
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<bml:BMLReport xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \n"
	"xmlns:jc3iedm=\"urn:int:nato:standard:mip:jc3iedm:3.1a:oo:2.0\" \n"
	"xmlns:bml=\"http://netlab.gmu.edu/IBML\" \n"
	"xmlns:msdl=\"http://netlab.gmu.edu/JBML/MSDL\"> \n"
	"<bml:Report>\n"
	"<bml:CategoryOfReport>StatusReport</bml:CategoryOfReport>\n"
	"<bml:TypeOfReport>GeneralStatusReport</bml:TypeOfReport>\n"
	"<bml:StatusReport><bml:GeneralStatusReport>\n"
	"<bml:ReporterWho><bml:UnitID>\n");
std::string ibml09GSRpart2(
	"</bml:UnitID></bml:ReporterWho> "
	"<bml:Hostility>FR</bml:Hostility> "
    "<bml:Executer>\n"
	"<bml:UnitID>");
std::string ibml09GSRpart3(
	"</bml:UnitID></bml:Executer>\n"
	"<bml:OpStatus>MOPS</bml:OpStatus>\n"
	"<bml:WhereLocation><bml:GDC> "
	"<bml:Latitude>");
std::string ibml09GSRpart4(
	"</bml:Latitude>\n"
	"<bml:Longitude>");
std::string ibml09GSRpart5(
	"</bml:Longitude>\n"
	"<bml:ElevationAGL>0</bml:ElevationAGL> "
	"</bml:GDC></bml:WhereLocation>\n"
    "<bml:When>20070101000000.000</bml:When>\n"
    "<bml:ReportID>192</bml:ReportID>\n"
    "<bml:Credibility>\n"
	"<bml:Source>HUMINT</bml:Source>\n"
	"<bml:Reliability>A</bml:Reliability>\n"
    "<bml:Certainty>RPTFCT</bml:Certainty>\n"
	"</bml:Credibility>\n"
	"</bml:GeneralStatusReport></bml:StatusReport></bml:Report></bml:BMLReport>");

// pieces of C2SIM position report
std::string c2simPositionPart1(
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<CWIX_Position_Report xmlns=\"http://www.sisostds.org/schemas/c2sim/1.0\" \n"
	"xmlns:xsi = \"http://www.w3.org/2001/XMLSchema-instance\" \n"
	"xsi:schemaLocation = \"http://www.sisostds.org/schemas/c2sim/1.0 C2SIM_Experimental.xsd\"> \n"
	"<ActorEntity><Name>");
std::string c2simPositionPart2(
	"</Name></ActorEntity><ReportingEntity><ActorEntityID>");
std::string c2simPositionPart3(
	"</ActorEntityID></ReportingEntity><ReportingTime>");
std::string c2simPositionPart4(
	"</ReportingTime><Location><Latitude>");
std::string c2simPositionPart5(
	"</Latitude><Longitude>");
std::string c2simPositionPart6(
	"</Longitude></Location>"
	"<HealthStatus><OperationalStatusCode>");
std::string c2simPositionPart7(
	"</OperationalStatusCode>");// removed <StrengthPercentage> 24May18
std::string c2simPositionPart8(
	"<HostilityCode>");
std::string c2simPositionPart9(
	"</HostilityCode></HealthStatus></CWIX_Position_Report>");

// send REST message
void DtTextInterface::sendRest(
	boolean formatIsC2sim, 
	std::string restServerAddress,
	std::string restPort,
	std::string clientID,
	std::string report) {
	
	// make a RestClient and use it to send transaction
	RestClient* restClient = new RestClient(restServerAddress);
	restClient->setPort(restPort);
	restClient->setSubmitter(clientID);
	std::string restResponse = restClient->bmlRequest(report);

	// output the status from response
	std::string afterStatus = restResponse.substr(restResponse.find("<status>",0));
	std::string status = afterStatus.substr(0, afterStatus.find("</status>", 0)+9);
	std::cout << "RESPONSE:" << status << "\n";
}

//////////////////////////////////////////////////////////////////////////
// Report callbacks from Doug Reece
//
// These two functions handle 3 report types; they could be factored into
// 3 functions, or combined into 1

void reportCallback(const DtVrfObjectMessage* msg, void* usr)
{
	if (msg)
	{
		DtReportMessage* reportMsg = (DtReportMessage*)msg;
		DtSimReportMessageType msgType = reportMsg->contentType();
		DtString completedTypeString("task-completed-report");
		DtString textTypeString("text-report");

		if (msgType.string() == completedTypeString)
		{
			DtTaskCompleteReport* taskCompleteReport = 
				dynamic_cast<DtTaskCompleteReport*>(reportMsg->report());

			if (taskCompleteReport)
			{
				printf("Task complete message from %s\n",
					msg->transmitter().markingText().c_str());
				printf("   Task type completed: %s\n",
					taskCompleteReport->taskCompleted().string().c_str());
				taskNumberCompleted++;
			}
		}
		else if (msgType.string() == textTypeString)
		{
			DtTextReport* textReport = dynamic_cast<DtTextReport*>(
				reportMsg->report());
			if (textReport)
			{
				char charReport[120];
				strncpy(charReport, textReport->text().c_str(), 119);
				charReport[119] = '\0';
				char* keyword;
				char* name;
				char* latChars;
				char* lonChars;

				// The expected BLUFOR tracking report:
				// TRACKING "entity name" <latitude in deg> <lon in deg>
				// Note that there must be 1 and only 1 space between 
				// TRACKING and the quoted entity name.

				keyword = strtok(charReport, " ");
				if (strcmp(keyword, "TRACKING") == 0)
				{
					name = strtok(NULL, "\"");
					latChars = strtok(NULL, " ");
					lonChars = strtok(NULL, " ");
					std::cout << "position report for " << name << " " << latChars << "/" << lonChars;
					if (!textIfUseIbml)
						std::cout << " format C2SIM\n";
					else
						std::cout << " format IBML09\n";
					
					// send a report - this is incomplete - it only does taskerIntent, unitID, and position
					std::string nameString = std::string(name);
					std::string latString = std::string(latChars);
					std::string lonString = std::string(lonChars);
					
					// choose report format matching order received, only after started
					if (simStarted)
					if (!textIfUseIbml) {
						DtTextInterface::sendRest(true, restServerAddress, restPortNumber, clientIDCode, 
							c2simPositionPart1 + nameString + c2simPositionPart2 + nameString +
							c2simPositionPart3 + "2018-02-07T06:00:00Z" + // reporting time
							c2simPositionPart4 + latString +
							c2simPositionPart5 + lonString +
							c2simPositionPart6 + "OP" + // operational status
							c2simPositionPart7 +        // strength percentage remove 24May18
							c2simPositionPart8 + "FR" + // hostility code
							c2simPositionPart9);
					}
					else {
						DtTextInterface::sendRest(false, restServerAddress, restPortNumber, clientIDCode,
							ibml09GSRpart1 + nameString + ibml09GSRpart2 + nameString +
							ibml09GSRpart3 + latString + ibml09GSRpart4 + lonString + ibml09GSRpart5);
					}
				}
			}
		}
	}// end if (msg)
}// end reportCallback


void DtTextInterface::spotReportCallback(//const 
	DtVrfObjectMessage * msg, void * usr)
{
	const DtReportMessage* reportMsg = static_cast<const DtReportMessage*>(msg);
	// Make local copy so we can change the contact sources without messing with
	// the message that came though the callback.
	DtSimReportCollection* spotReports = static_cast<DtSimReportCollection*>(reportMsg->report()->clone(""));

	std::vector<DtSimReport*> reports = spotReports->reports();
	std::vector<DtSimReport*>::const_iterator iter = reports.begin();

	while (iter != reports.end())
	{
		DtSpotReport* spotRep = dynamic_cast<DtSpotReport*>(*iter);

		if (spotRep && spotRep->contactForce() == DtForceOpposing)
		{
			printf("Spotted: %s\n", spotRep->contact().markingText().c_str());
			DtEntityType contactType(spotRep->contactType());
			printf("  Kind: %d, domain: %d, category %d\n",
				contactType.kind(), contactType.domain(), contactType.category());
		}

		++iter;
	}

	delete spotReports;
}

// provide C2SIMinterface with number of task completed
int DtTextInterface::getTaskNumberCompleted() {
	return taskNumberCompleted;
}

// End report callbacks

//////////////////////////////////////////////////////////////////////////

void DtTextInterface::helpCmd(char* s, DtTextInterface* a)
{
   if (!strncmp(s, "create", 6))
   {
      DtInfo("\n"
             "\tcreate waypoint <\"name\"> <p1>\n"
             "\tcreate route <\"name\"> <p1 p2 ... pn>\n"
             "\tcreate tank <p1>\n"
             "\tcreate aggregate <p1>\n"
             "\t\twhere px has form x,y,z\n");
   }
   else if (!strncmp(s, "set", 3))
   {
      DtInfo("\n"
             "\tset plan <\"name\">\n"
             "\tset label <objectId> <\"label\">\n"
             "\tset location <\"name\"> <pt>\n"
             "\tset fuel <\"name\"> <double>\n"
             "\tset target <\"name\"> <\"name\">\n"
             "\tset restore <\"name\"> \n");
   }
   else if (!strncmp(s, "task", 4))
   {
      DtInfo("\n"
             "\ttask patrolRoute <\"name\"> <\"route\">\n"
             "\ttask moveToPoint <\"name\"> <\"waypoint\">\n"
             "\ttask follow <\"name\"> <\"leader\">\n"
             "\ttask wait <\"name\"> <time>\n");
   }
   else if (!strncmp(s, "monitor", 4))
   {
      DtInfo("\nmonitor <\"resource\" | \"ALL\"> <\"entity name\"> [update time in seconds]\n");
   }
   else if (!strncmp(s, "unmonitor", 4))
   {
      DtInfo("\nunmonitor <\"resource\" | \"ALL\"> <\"entity name\">\n");
   }
   else if (!strncmp(s, "simrun", 4))
   {
      DtInfo("simrun [site:host]\n");
   }
   else if (!strncmp(s, "turnonsnapshots", 4))
   {
      DtInfo("turnonsnapshots <simulation time> <number to keep in memory>\n");
   }
   else if (!strncmp(s, "simpause", 4))
   {
      DtInfo("simpause [site:host]\n");
   }
   else if (!strncmp(s, "timemultiplier", 4))
   {
      DtInfo("timemultiplier number\n");
   }
   else if (!strncmp(s, "aggregate", 4))
   {
      DtInfo("\naggregate <name1> <name2> <name3>\n");
   }
   else
   {
      DtInfo("\n"
          "\tload <\"filename\">\n"
          "\tsave <\"filename\">\n"
          "\tnew <\"terrain db filename\">\n"
          "\tclose\n"
          "\trewind\n"
          "\trun\n"
          "\tpause\n"
          "\tsimrun\n"
          "\tsimpause\n"
          "\tlistsnapshots\n"
          "\ttakesnapshot\n"
          "\tturnonsnapshots <simulation time> <number to keep>\n"
          "\tturnoffsnapshots\n"
          "\trollback <simulation time>\n"
          "\ttimemultiplier <number>\n"
          "\tlist\n"
          "\tcreate [ waypoint | route | tank  | aggregate]\n"
          "\tdelete [ <object_name> | <object_id> ]\n"
          "\tset [ plan | label | location | fuel | target | restore ]\n"
          "\ttask [ patrolRoute | moveToPoint | follow | wait ]\n"
          "\taggregate <name1> <name2> <name3>\n"
          "\tmonitor <\"resource\" | \"ALL\"> <\"entity name\"> [update time in seconds]\n"
          "\tunmonitor <\"resource\" | \"ALL\"> <\"entity name\">\n"
          "\thelp <command_name>\n" 
          "\tquit\n");
   }
}

void DtTextInterface::quitCmd(char* s, DtTextInterface* a)
{
   a->setTimeToQuit(true);
}

void DtTextInterface::listSnapshotsCmd(char* s, DtTextInterface* a)
{
   a->listSnapshots();
}

void DtTextInterface::takeSnapshotCmd(char* s, DtTextInterface* a)
{
   a->controller()->takeSnapshot();
   DtInfo << "Snapshot taken" << std::endl;
}

void DtTextInterface::turnOnSnapshotsCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif

   if (!strcmp(str, "")) 
   {
      helpCmd("turnonsnapshots", NULL);
   }
   else
   {
      char *t1, *t2;
      t1 = strtok(str, " ");

      if (t1)
      {
         t2 = strtok(NULL, " ");

         if (t2)
         {
            a->controller()->setPeriodicSnapshotSettings(true, std::max(1, atoi(t1)), 
               std::max(1, atoi(t2)));
         }

         helpCmd("turnonsnapshots", NULL);
      }
   }
}

void DtTextInterface::turnOffSnapshotsCmd(char* s, DtTextInterface* a)
{
   a->controller()->setPeriodicSnapshotSettings(false, -1, -1);
}

void DtTextInterface::rollbackToSnapshotCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif
   if (!strcmp(str, "")) return;

   a->controller()->rollbackToSnapshot(atof(str));
}

void DtTextInterface::listCmd(char* s, DtTextInterface* a)
{
   int i = 0;
   DtInfo("Listing known backends\n");
   DtList backends = a->controller()->backends();
   for (DtListItem* item = backends.first(); item; item = item->next())
   {
      DtSimulationAddress* addr = (DtSimulationAddress*) item->data();
      DtInfo("\t%d) %s\n", ++i, addr->string());
      if (i == 5) DtInfo("\n");
   }

   DtInfo("Current state: %s\n", DtControlTypeString(
      a->controller()->backendsControlState()));
}

void DtTextInterface::loadCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif
   if (!strcmp(str, "")) return;

   char *name;
   name = strtok(str, "\"");
   a->controller()->loadScenario(
      DtFilename(DtString(scenariosDir) + DtString(name)), missingBackends, 
      a);
}

bool DtTextInterface::missingBackends(DtBackendMap* map, 
   void* usr)
{
   DtWarn("Cannot load scenario due to missing backends:\n");
   DtListItem* item;
   for (item = map->first(); item; item = item->next())
   {
      DtBackendMapEntry* entry = (DtBackendMapEntry*)item->data();
      if (entry)
      {
         DtWarn("\t%s\n", entry->fromAddress().string());
      }
   }
   
   return false;
}

void DtTextInterface::bigRouteCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif

   int iPoints = 0;

   if (strlen(str) == 0)
   {
      //iPoints = 800;
	  iPoints = 2800;
   }
   else
   {
      iPoints = atoi(str);
   }

   a->bigRoute(iPoints);
}

void DtTextInterface::bigRoute(int iPoints)
{
   srand(time(NULL));
   DtList verts;

   for (int i = 0; i < iPoints; i++)
   {
      /*verts.add(new DtVector((double)((double)rand() / (double)RAND_MAX) * 2000.,
         (double)((double)rand() / (double)RAND_MAX) * 2000., 100.));*/

	  verts.add(new DtVector((double)((double)rand() / (double)RAND_MAX) * 2000.,
		  (double)((double)rand() / (double)RAND_MAX) * 6000., 100.));
   }

   controller()->createRoute(verts);

   DtListItem* item = verts.first();

   while (item)
   {
      delete (DtVector*)item->data();

      item = item->next();
   }
}

void DtTextInterface::saveCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif
   if (!strcmp(str, "")) return;

   if (!strstr(str, ".scn"))
      strcat(str, ".scn");

   char *name;
   name = strtok(str, "\"");
   a->controller()->saveScenario(
      DtFilename(DtString(scenariosDir) + DtString(name)));
}

void DtTextInterface::rewindCmd(char* str, DtTextInterface* a)
{
   a->controller()->rewind();
}

void DtTextInterface::closeCmd(char* s, DtTextInterface* a)
{
   a->controller()->unsubscribeAllPlans();
   a->controller()->removeAllPlanStatementCallbacks();
   a->controller()->removeAllPlanCompleteCallbacks();
   a->controller()->closeScenario();
}

void DtTextInterface::newCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif
   if (!strcmp(str, "")) 
   {
      DtInfo("Give me a terrain database, please!\n> ");
   }
   else
   {
      a->controller()->unsubscribeAllPlans();
      a->controller()->removeAllPlanStatementCallbacks();
      a->controller()->removeAllPlanCompleteCallbacks();
      char *name;
	  name = strtok(str, "\"");

	  // produce message for nasty omission
	  if (name[strlen(name) - 1] == '\n')
		  std::cout << "MISSING FINAL QUOTE FOR new " << name << "\n";

	  DtFilename terrainFilename(DtString(terrainDir) + DtString(name));
	  a->controller()->newScenario(terrainFilename, terrainFilename);
   }
}

void DtTextInterface::createCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif
   if (!strcmp(str, "")) 
   {
      DtInfo("Create what?!\n");
      helpCmd("create", NULL);
   }
   else if (!strncmp(str, "waypoint", 8))
   {
      a->createWaypoint(str);
   }
   else if (!strncmp(str, "route", 5))
   {
      a->createRoute(str);
   }
   else if (!strncmp(str, "tank", 4))
   {
      a->createTank(str);
   }
   else if (!strncmp(str,"aggregate",9))
   {
      a->createAggregate(str);
   }
}

void DtTextInterface::createAggregate(char* str)
{   
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.
   char *point;
   point = strtok(&str[9], " ");

   if (!point)
   {
      DtInfo("\tusage: aggregate <geocentric_location>\n"); 
      return;
   }

   DtVector vec;
   sscanf(point, "%lf,%lf,%lf", &vec[0], &vec[1], &vec[2]);
   //Here we are creating the indivual parts. We register with the vrfObjectCreatedCb callback because
   //we can't add the subordinates to our aggregate until they've actually been created. Timing can be tricky here if the indivual is 
   //created before the aggregate. In this simple example we create the aggregate entity first then it's subordinates.
   
   //When manaully creating aggregates and their components this timing must be considered. One means is to check if the aggregate has been created
   //and if not, keep a list of the individuals and in a callback once the desired aggregate has been created then add them to the aggregate's 
   //organization. 

   //Create the aggregate entity at desired position with name "tankPlt"
   DtString pltName = "Tank_Plt_" + DtString(theNextAggregateId++);

   /*controller()->createAggregate(
	   vrfObjectCreatedCb, 
	   (void*)myController,
	   DtEntityType(11, 1, 225, 3, 0, 0, 0), vec,
	   DtForceFriendly, 0,pltName);*/

   controller()->createAggregate(
	   vrfObjectCreatedCb,
	   (void*)myController,
	   DtEntityType(1, 2, 225, 1, 9, 0, 0), vec,
	   DtForceFriendly, 0, pltName);

   //Now create the individual tanks that will make up this aggregate
   DtString subName = "Plt_Sub_";

   /*controller()->createEntity(
      vrfObjectCreatedCb, (void*)myController,
      DtEntityType(1, 1, 225, 1, 1, 3, 0), vec,
      DtForceFriendly, 90.0,subName + DtString(theNextSubordinateId++));*/

   controller()->createEntity(
	   vrfObjectCreatedCb, (void*)myController,
	   DtEntityType(1, 2, 225, 1, 9, 0, 0), vec,
	   DtForceFriendly, 90.0, subName + DtString(theNextSubordinateId++));

   
   //Change the offet from each other to create an intial triangle formation
   vec[DtX]+=10;
   controller()->createEntity(
	   vrfObjectCreatedCb, (void*)myController,
	   DtEntityType(1, 2, 225, 1, 9, 0, 0), vec,
	   DtForceFriendly, 90.0,subName + DtString(theNextSubordinateId++));
   
   vec[DtY]+=10;
   controller()->createEntity(
	   vrfObjectCreatedCb, (void*)myController,
	   DtEntityType(1, 2, 225, 1, 9, 0, 0), vec,
	   DtForceFriendly, 90.0,subName + DtString(theNextSubordinateId++));
}

void DtTextInterface::resourcesProcessedCb(DtSimMessage* msg, void* usr)
{
   DtIfServerRequestResponse* s = (DtIfServerRequestResponse*) 
      ((DtSimInterfaceMessage*) msg)->interfaceContent();

   DtString str;
   if (s->type() == DtResourceMonitorResponseMessageType)
   {
      s->printDataToString(str);
      DtInfo("%s\n", str.string());
   }
}

void DtTextInterface::monitorResourcesCmd(char * str, DtTextInterface* a)
{
   if (!strcmp(str, "")) 
   {
      DtInfo("Monitor what?!\n");
      helpCmd("monitor", NULL);
   }
   else
   {
      char *res;
      char *entity;
      char *time;
      char* temp;
      int iTime = 10;

      res = strtok(str, "\"");
      temp = strtok(NULL, "\"");
      entity = strtok(NULL, "\"");
      time = strtok(NULL, "\"");

      DtIfResourceMonitorRequest req;

      req.setUUID(DtUUID(entity));

      if (time)
      {
         iTime = atoi(time);
      }

      req.setMonitorPeriod(iTime);
      req.setMonitorBy(DtCondExprSimTimeType);

      if (strcmp(res, "ALL"))
      {
         req.addMonitoredItem(res);
      }

      if (!monitorResourcesCallbackAdded)
      {
         a->controller()->vrfMessageInterface()->addMessageCallback(
            DtResourceMonitorResponseMessageType, resourcesProcessedCb, a);

         monitorResourcesCallbackAdded = true;
      }

      a->controller()->vrfMessageInterface()->createAndDeliverMessage(DtSimSendToAll, req);
   }
}

void DtTextInterface::removeMonitorResourcesCmd(char * str, DtTextInterface* a)
{
   if (!strcmp(str, "")) 
   {
      DtInfo("unmonitor what?!\n");
      helpCmd("unmonitor", NULL);
   }
   else
   {
      char *res;
      char *entity;
      char* temp;

      res = strtok(str, "\"");
      temp = strtok(NULL, "\"");
      entity = strtok(NULL, "\"");
      temp = strtok(NULL, "\"");

      DtIfResourceMonitorRequest req;

      req.setUUID(DtUUID(entity));
      req.setMonitorPeriod(10);
      req.setMonitorBy(DtCondExprSimTimeType);

      if (strcmp(res, "ALL"))
      {
         req.addMonitoredItem(res);
      }

      req.setStopMonitoring(true);

      a->controller()->vrfMessageInterface()->createAndDeliverMessage(DtSimSendToAll, req);
   } 
}

void DtTextInterface::aggregateCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif

   if (!strcmp(str, "")) 
   {
      DtInfo("Aggregate what?!\n");
      helpCmd("aggregate", NULL);
   }
   else
   {
      char *e1, *e2, *e3, *temp;
      e1 = strtok(str, "\"");
      temp = strtok(NULL, "\"");
      e2 = strtok(NULL, "\"");
      temp = strtok(NULL, "\"");
      e3 = strtok(NULL, "\"");
      temp = strtok(NULL, "\"");
      
      DtList entities;
      if (e1) entities.add(new DtString(e1));
      if (e2) entities.add(new DtString(e2));
      if (e3) entities.add(new DtString(e3));

      a->controller()->createAggregate(
         DtEntityType(11, 1, 0, 3, 2, 0, 0), 
         DtVector(10.0, 10.0, 10.0),
         DtForceFriendly, 0.0, &entities);

      //empty list of entities
      DtListItem* next = NULL;
      for (DtListItem* item = entities.first(); item; item = next)
      {
         next = item->next();
         delete (DtString*) entities.remove(item);      
      }
   }
}

void DtTextInterface::setCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif

   if (!strcmp(str, "")) 
   {
      DtInfo("Set what?!\n");
      helpCmd("set", NULL);
   }
   else if (!strncmp(str, "plan", 4))
   {
      a->setPlan(str);
   }
   else if (!strncmp(str, "label", 5))
   {
      a->setLabel(str);
   }
   else if (!strncmp(str, "location", 8))
   {
      a->setLocation(str);
   }
   else if (!strncmp(str, "fuel", 4))
   {
      a->setFuel(str);
   }
   else if (!strncmp(str, "target", 6))
   {
      a->setTarget(str);
   }
   else if (!strncmp(str, "restore", 7))
   {
      a->setRestore(str);
   }
}

void DtTextInterface::taskCmd(char* str, DtTextInterface* a)
{
#if WIN32
   str[strlen(str)]='\0';
#else
   str[strlen(str)-1]='\0';
#endif
   if (!strcmp(str, "")) 
   {
      DtInfo("Task what?!\n");
      helpCmd("task", NULL);
   }
   else if (!strncmp(str, "patrolRoute", 11))
   {
      a->patrolRoute(str);
   }
   else if (!strncmp(str, "moveToPoint", 11))
   {
      a->moveToPoint(str);
   }
   else if (!strncmp(str, "follow", 6))
   {
      a->follow(str);
   }
   else if (!strncmp(str, "wait", 4))
   {
      a->wait(str);
   }
}   

void DtTextInterface::deleteCmd(char* s, DtTextInterface* a)
{
#if WIN32
   s[strlen(s)]='\0';
#else
   s[strlen(s)-1]='\0';
#endif
 
   if (strchr(s, ':'))
   {
      a->controller()->deleteObject(DtUUID(s));
   }
   else
   {
      char* strname = strtok(s, "\"");
      a->controller()->deleteObject(DtUUID(strname));
   }
}

void DtTextInterface::runCmd(char* s, DtTextInterface* a)
{
   a->controller()->run();
}

void DtTextInterface::simrunCmd(char* s, DtTextInterface* a)
{   
   DtStartResumeInteraction ds;
   
#if WIN32
   s[strlen(s)]='\0';
#else
   s[strlen(s)-1]='\0';
#endif

   //if no arguments, send it to all backends
   if (strcmp(s, "") == 0) 
   {
      ds.setReceiverId(DtEntityIdentifier(DtSimSendToAll,0));
   }
   else
   {
      //get the backend id
      if (strchr(s, ':'))
      {         
         ds.setReceiverId(DtEntityIdentifier(DtSimulationAddress(s),0));
      }
      else
      {
         DtWarn << "Start who? (use site:host)";
         return;
      }
   }   
   ds.setSenderId(DtEntityIdentifier(a->controller()->simulationAddress(),0));
   a->controller()->vrfMessageInterface()->exerciseConn()->sendStamped(ds);
}

void DtTextInterface::pauseCmd(char* s, DtTextInterface* a)
{    
   a->controller()->pause();
}

void DtTextInterface::simpauseCmd(char* s, DtTextInterface* a)
{ 
   DtStopFreezeInteraction ds;

#if WIN32
   s[strlen(s)]='\0';
#else
   s[strlen(s)-1]='\0';
#endif

   //if no arguments, send it to all backends
   if (strcmp(s, "") == 0) 
   {
      ds.setReceiverId(DtEntityIdentifier(DtSimSendToAll,0));
   }
   else
   {
      //get the backend id site:host
      if (strchr(s, ':'))
      {         
         ds.setReceiverId(DtEntityIdentifier(DtSimulationAddress(s),0));
      }
      else
      {
         DtWarn << "Stop who? (use site:host)";
         return;
      }
   }   
   ds.setSenderId(DtEntityIdentifier(a->controller()->simulationAddress(),0));
   a->controller()->vrfMessageInterface()->exerciseConn()->sendStamped(ds);
}

void DtTextInterface::timemultiplierCmd(char* str, DtTextInterface* a)
{ 
   char* t;
   t = strtok(str, " ");
   double multiplier = atof(t);

   if (multiplier == 0.0)
   {  
      DtInfo("\tusage: timemultiplier <number>\n"); 
      return;
   }

   a->controller()->setTimeMultiplier(multiplier);
}

#ifdef _PowerUX
int strcasecmp(const char *s1, const char *s2)
{
     char *s1Temp = strdup(s1);
     char *s2Temp = strdup(s2);

     for (int strIndex= 0; strIndex< strlen(s1Temp); strIndex++)
        if ((64 < s1Temp[strIndex]) && (s1Temp[strIndex] < 91))
             s1Temp[strIndex] += 32;
     for (int strIndex= 0; strIndex< strlen(s2Temp); strIndex++)
        if ((64 < s2Temp[strIndex]) && (s2Temp[strIndex] < 91))
             s2Temp[strIndex] += 32;
     int retVal = (strcmp(s1Temp,s2Temp));
     free (s1Temp);
     free (s2Temp);
     return retVal;

}

#endif

#if WIN32
static char nbdf_buff[128];
static int nbdf_ptr=0;    
char *non_blocking_dos_fgets(char *buff, int size, FILE *f)
{
   while (_kbhit())
   {
      nbdf_buff[nbdf_ptr] = _getche();
      if (  nbdf_buff[nbdf_ptr] == '\r'
         || nbdf_buff[nbdf_ptr] == '\n'
         || nbdf_ptr >= size )
      {
         nbdf_buff[nbdf_ptr] = 0; //terminate string
         strcpy(buff,nbdf_buff);
         nbdf_ptr=0;
         DtInfo("\n");
         return(buff);
      }
      else if ( nbdf_buff[nbdf_ptr] == '\b')
      {
         //Write out a blank space to erase whatever was there
         _putch(' ');
         //Move back before the space
         _putch('\b');
         --nbdf_ptr;
      }
      else
      {
         ++nbdf_ptr;            
      }
    }
    return NULL;
}
#endif

#define LASTCHAR(s) (s[strlen(s)-1])
#define STERMINATE(s) do{ char *t=(s); if (*t) (s)++, *t='\0';} while(0)

/* returns pointer to next token separated by a sep character
 * it's similar to strtok, but does not use internal static memory,
 * and leaves nextp pointing to end of word
 * note that gettok modifies the contents of the
 * strings passed to it by replacing separators with '\0' */
char *gettok(char **nextp, char *sep)
{
    char *start, *end;
    start = *nextp;

	while (*start && strchr(sep, *start))
		start++;

    if (!*start)
        return NULL;
    end = start;
	
    while (*end && !strchr(sep, *end) )
        end++;
    *nextp = end;

    STERMINATE(*nextp);
    return start;
}


#if !WIN32
void setBlock(int fd, int on)
{
    static int blockf, nonblockf;
    static int first = 1;

    if (first) {
        first = 0;
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1)
            DtWarnPerror("fcntl(F_GETFL) failed");
        /* could use FNDELAY instead of FNONBLK, but then feof()
           will be confused */
        blockf = flags & ~O_NONBLOCK;
        nonblockf = flags | O_NONBLOCK;

    }

    if (fcntl(0, F_SETFL, on ? blockf : nonblockf) < 0)
        DtWarnPerror("fcntl(F_SETFL) failed");
}
#endif

Cmd *lookupCmd(char *cmdname)
{
    if (!cmdname)
        return NULL;
    for (int j=0; j<nCmds; j++)
#if !WIN32
        if (!strcasecmp(cmdname, allCmds[j].name))
#else
        if (!_stricmp(cmdname, allCmds[j].name))
#endif
            return allCmds+j;
    return NULL;
}

// constructor
DtTextInterface::DtTextInterface(
	DtVrfRemoteController* controller,
	std::string serverAddressRef,
	std::string restPortRef,
	std::string clientIDRef,
	bool useIbmlRef) :
  kbInterest(0), myTimeToQuit(0), 
    myController(controller)
{
   restServerAddress = serverAddressRef;
   restPortNumber = restPortRef;
   clientIDCode = clientIDRef;
   textIfUseIbml = useIbmlRef;

   DtInfo("Scenario directory: %s\n", scenariosDir); 
   DtInfo("Terrain directory: %s\n", terrainDir);
        
   myController->addBackendDiscoveryCallback(backendAddedCb, NULL);
   myController->addBackendRemovalCallback(backendRemovedCb, NULL);

   myController->addBackendLoadedCallback(backendLoadedCb, NULL);
   myController->addScenarioLoadedCallback(ScenarioLoadedCb, NULL);

   myController->addBackendSavedCallback(backendSavedCb, NULL);
   myController->addScenarioSavedCallback(ScenarioSavedCb, NULL);

   kbInterest++;
   //prompt();
   
   //from Doug Reece
   DtVrfObjectMessageExecutive* msgExec = myController->objectMessageExecutive();
   // This is necessary to capture the taskComplete report messages, which go 
   // through a VRF internal message passing mechanism, but not the radio simulation
   msgExec->addMessageCallbackByCategory(
	   DtReportMessageType, 
	   (DtVrfObjectMessageCallbackFcn)reportCallback,
	   this);
   
   // And this is necessary to get the text and spot reports, which go through the radio
   // mechanism. 
   myController->radioMessageListener()->
	   addMessageCallback(//DtTextReportType, 
	   (DtVrfObjectMessageCallbackFcn)reportCallback, (void*)0);
   //myController->radioMessageListener()->
   //  addMessageCallback(//DtSimReportCollectionType, 
   //  (DtVrfObjectMessageCallbackFcn)spotReportCallback, (void*)0);
}

DtTextInterface::~DtTextInterface()
{
   myController->removeBackendDiscoveryCallback(backendAddedCb, NULL);
   myController->removeBackendRemovalCallback(backendRemovedCb, NULL);

   myController->removeBackendLoadedCallback(backendLoadedCb, NULL);
   myController->removeScenarioLoadedCallback(ScenarioLoadedCb, NULL);

   myController->removeBackendSavedCallback(backendSavedCb, NULL);
   myController->removeScenarioSavedCallback(ScenarioSavedCb, NULL);
}


void DtTextInterface::setTimeToQuit(bool yesNo)
{
   myTimeToQuit = yesNo;
}

bool DtTextInterface::timeToQuit() const
{
   return myTimeToQuit;
}

DtVrfRemoteController* DtTextInterface::controller() const
{
   return myController;
}

void DtTextInterface::readCommand()
{
/*
	char buff[256];
   int discardNext = 0;

   UNBLOCK();

   // this while replaces the previous fetch from stdin
   // while (non_blocking_dos_fgets(buff, sizeof(buff), stdin))
   while (C2SIMinterface::non_blocking_C2SIM_fgets(buff, sizeof(buff)) != NULL)
   {
	  int discardThis = discardNext;
      discardNext = (LASTCHAR(buff) != 10);
      if(!discardThis)
      {
         // RESTORE/UNBLOCK are not necessary if you know you
         // won't terminate from within processCmd 
         RESTORE();
         processCmd(buff);
         if(!kbInterest)
         {
            return;
         }
         UNBLOCK();
      }
   }
   RESTORE();
   if(feof(stdin))
   {
      DtInfo("EOF\n");
      exit(0);
   }*/
}

void DtTextInterface::prompt()
{
	if (kbInterest)
       DtInfo("process new command >");
	fflush(stdout);
}

void DtTextInterface::processCmd(char *buff)
{
    char *cmdname = gettok(&buff, "\t \n");
    Cmd *cmd = lookupCmd(cmdname);
	if (cmd)
		(*cmd->func)(buff, this);

    else if (cmdname)
      DtWarn("VRF Remote controller: no such command: \"%s\"\n", cmdname);
	prompt();
}

void DtTextInterface::backendAddedCb(const DtSimulationAddress& addr, void* usr)
{
   DtInfo("\nBackend %s Added\n> ", addr.string());
}

void DtTextInterface::backendRemovedCb(const DtSimulationAddress& addr, void* usr)
{
   DtInfo("\nBackend %s Removed\n> ", addr.string());
}

void DtTextInterface::backendLoadedCb(const DtSimulationAddress& addr, void* usr)
{
   DtInfo("\nBackend %s Loaded\n> ", addr.string());
}

void DtTextInterface::ScenarioLoadedCb(void* usr)
{
   DtInfo("\nScenario Loaded\n> ");
}

void DtTextInterface::backendSavedCb(const DtSimulationAddress& addr, DtSaveResult result, void* usr)
{
   DtInfo("\nBackend %s Saved (%s)\n> ", 
      addr.string(), result == DtSuccess? "success" : "failure");
}

void DtTextInterface::ScenarioSavedCb(void* usr)
{
   DtInfo("\nScenario Saved\n> ");
}

void DtTextInterface::vrfObjectCreatedCb(
   const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid, void* usr)
{
   static DtUUID aggId;

   //We want to check for the creation of the aggregate entity and keep a hold of it's entity id so that
   //when it's subordinates are created we can add them to the aggregate's organization
   DtString search = name;

   if( search.findString("Tank_Plt") >=0 )
   {
      DtInfo("\n aggregate has been created with name and id: \"%s\", %s\n> ", 
         name.string(), id.string());
		aggId = uuid;
   }
   else
   {
      DtInfo("\n entity has been created with name and id: \"%s\", %s\n> ", 
         name.string(), id.string());
   }

	//If the name of our new entity is one of our desired subordinates we will add it to the tank platoon
   DtString sub = name;
   if ( sub.findString("Plt_Sub") >= 0 )
   {
      //We grab hold of the remote controller and add our individual to the aggregate
      DtVrfRemoteController* pVrfRemoteCtrl = (DtVrfRemoteController*)(usr);
      //This entity belongs to our aggregate, add it to the platoon
      if ( pVrfRemoteCtrl )
      {
		   pVrfRemoteCtrl->addToOrganization(uuid, aggId);
         DtInfo("\n %s has been added to aggregate with id:  %s\n> ",name.string(),aggId.string());
      }
   }
}

void DtTextInterface::createWaypoint(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *point, *name;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");
   point = strtok(NULL, " ");

   if (!point || !name)
   {
      DtInfo("\tusage: create waypoint <\"name\"> <pt>\n"); 
      return;
   }

   DtVector vec;
   sscanf(point, "%lf,%lf,%lf", &vec[0], &vec[1], &vec[2]);

   controller()->createWaypoint(vrfObjectCreatedCb, (void*)"waypoint", vec, 
      name);
}

void DtTextInterface::createRoute(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name, *point;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");

   if (!name)
   {
      DtInfo("\tusage: create route <\"name\"> <p1 p2 ... pn>\n"); 
      return;
   }

   //create list of vertices
   DtList vertices;
   while ((point = strtok(NULL, " ")) != NULL)
   {        
      double x, y, z;
      sscanf(point, "%lf,%lf,%lf", &x, &y, &z);
      DtVector* vec = new DtVector(x, y, z);
      vertices.add(vec);
   }

   controller()->createRoute(vrfObjectCreatedCb, (void*)"route", vertices, 
      name);

   //empty list of vertices
   DtListItem* next = NULL;
   for (DtListItem* item = vertices.first(); item; item = next)
   {
      next = item->next();
      delete (DtVector*) vertices.remove(item);      
   }
}

void DtTextInterface::createTank(char* str)
{   
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *point;
   point = strtok(&str[4], " ");

   if (!point)
   {
      DtInfo("\tusage: tank <geocentric_location>\n"); 
      return;
   }

   DtVector vec;
   sscanf(point, "%lf,%lf,%lf", &vec[0], &vec[1], &vec[2]);

   controller()->createEntity(
      vrfObjectCreatedCb, (void*)"tank",
      DtEntityType(1, 1, 225, 1, 1, 3, 0), vec,
      DtForceFriendly, 90.0);
}

void DtTextInterface::patrolRoute(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name, *route;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");
   route = strtok(NULL, "\"");  //get past whitespace
   route = strtok(NULL, "\"");

   if (!name || !route)
   {  
      DtInfo("\tusage: task patrolRoute <\"name\"> <\"route\">\n"); 
      return;
   }

   controller()->patrolAlongRoute(DtUUID(name), DtUUID(route));
}

void DtTextInterface::moveToPoint(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name, *point;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");
   point = strtok(NULL, "\"");  //get past whitespace
   point = strtok(NULL, "\"");

   if (!name || !point)
   {  
      DtInfo("\tusage: task moveToPoint <\"name\"> <\"waypoint\">\n"); 
      return;
   }
   controller()->moveToWaypoint(DtUUID(name), DtUUID(point));
}

void DtTextInterface::follow(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name, *leader;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");
   leader = strtok(NULL, "\""); //get past white space
   leader = strtok(NULL, "\"");

   if (!name || !leader)
   {  
      DtInfo("\tusage: task follow <name> <leader>\n"); 
      return;
   }
   DtVector offset(10.0, 0.0, 0.0);
   controller()->followEntity(DtUUID(name), DtUUID(leader), offset);
}

void DtTextInterface::wait(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name, *t;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");
   t = strtok(NULL, " ");

   if (!name || !t)
   {  
      DtInfo("\tusage: task wait <\"name\"> <time>\n"); 
      return;
   }

   DtTime time = atof(t);
   controller()->waitDuration(DtUUID(name), time);
}


void DtTextInterface::setPlan(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name = strtok(str, " ");
   name = strtok(NULL, "\"");
   if (!name)
   {
      DtInfo("\tusage: set plan <\"entity name\">\n"); 
      return;
   }

   //Create a plan
   //This plan will contain four statements:
   //1. Set heading to 2.094395 radians
   //2. Set speed to 5 m/s
   //3. Wait 20 seconds
   //4. Move to waypoint "alpha"

   DtPlan plan;
   plan.init();

   DtWaitDurationTask waitTask;
   waitTask.setSecondsToWait(20.0);

   DtMoveToTask moveTask;

   //! Marking text will work as well as the UUID for the alpha waypoint.  To find the UUID
   //! in code:
   //!
   //! DtUUID alphaUUID = DtUUID::markingTextResolutionManager().mapMarkingTextToUUID("alpha");
   moveTask.setControlPoint(DtUUID("alpha"));

   DtSetHeadingRequest headingReq;
   headingReq.setHeading(2.094395);

   DtSetSpeedRequest speedReq;
   speedReq.setSpeed(5);

   DtSetDataRequestStmt stmt1, stmt2;
   DtTaskStmt stmt3, stmt4;

   stmt1.setSetDataRequest(&headingReq);
   stmt2.setSetDataRequest(&speedReq);
   stmt3.setTask(&waitTask);
   stmt4.setTask(&moveTask);

   //Create a conditional expression.
   //This is just a simple example of a condition that will always be
   //true when the entity is first created -- it checks to see if the fuel
   //is non-zero.
   DtCeResourceOperator conditionalExpression;
   conditionalExpression.setResource("fuel");
   conditionalExpression.setOper(">");
   conditionalExpression.setComparisonValue(0.0);

   //Create an if statement using the conditional expression
   DtIfStmt ifStatement;
   ifStatement.setCondExpr(&conditionalExpression);
   ifStatement.thenBlock()->add(stmt1.clone());
   ifStatement.thenBlock()->addToEnd(stmt2.clone());
   ifStatement.thenBlock()->addToEnd(stmt3.clone());
   ifStatement.thenBlock()->addToEnd(stmt4.clone());

   DtPlanBlock* block = plan.mainBlock();
   block->addToStart(ifStatement.clone());

   //Assign the plan to the given entitiy
   controller()->assignPlanByName(DtUUID(name), plan);

   //Let's subscribe to the given entity's plan messages. 
   //This will let us know whether or "assignPlan" command 
   //was successfull.  Also, it will also notify us if any
   //changes were made to the plan.
   controller()->subscribePlan(DtUUID(name), planCb, NULL);

   //Let's register a callback for current statement
   //messages.  Our callback will be invoked whenever the entity
   //starts a new statement in the plan.
   controller()->addPlanStatementCallback(DtUUID(name), planStmtCb, NULL);

   //Let's also register a callback for when the entity completes
   //the plan.  Inside our callback, we unsubscribe to this entity's
   //plan messages and unregister our current statement callback.
   controller()->addPlanCompleteCallback(DtUUID(name), planCompleteCb, controller());
}

void DtTextInterface::planCb(const DtUUID& name, 
   const std::vector<DtSimStatement*>& stmts, bool append, void* usr)
{
   //Let's print the statements in the plan.
   //Because plans can be large, their statements may arrive in separate plan 
   //messages.  The append flag let's us know whether these statements belong to 
   //a plan we already have.

   if (append)
   {
      DtInfo("\nAdditional plan statements for %s.\n", name.string());
   }
   else
   {
      DtInfo("\nNew plan statements for %s.\n", name.string());
   }

   int i = 0;
   std::vector<DtSimStatement*>::const_iterator iter = stmts.begin();

   while (iter != stmts.end())
   {
      DtSimStatement* statement = *iter;
	   DtInfo("\t%d) %s\n", ++i, statement->stringRep().string());
   }
   DtInfo("\n> ");
}

void DtTextInterface::planStmtCb(const DtUUID& id, DtPlanStatus status,
   void* usr)
{
   //Let's print the statement that 'id' is executing.  A statementId = 0
   //indicates that the 'id' hasn't started its plan yet.
   if (status.currentStatementId > 0) 
   {
      DtInfo("\n%s executing statement %d\n> ", id.string(),
        status.currentStatementId);
   }
}

void DtTextInterface::planCompleteCb(const DtUUID& id, void* usr)
{
   //Let's print that 'id' has completed the plan.
   DtInfo("\n%s has completed its plan!\n> ", id.string());

   //Because the plan we assigned is now complete, we no longer need the
   //plan callbacks we registered.  Let's unregister them here.
   DtVrfRemoteController* controller = (DtVrfRemoteController*) usr;
   controller->unsubscribePlan(id, planCb, NULL);
   controller->removePlanCompleteCallback(id, planCompleteCb, NULL);
   controller->removePlanStatementCallback(id, planStmtCb, NULL);
}

void DtTextInterface::setLabel(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *objId, *label;
   objId = strtok(str, " ");
   objId = strtok(NULL, " ");
   label = strtok(NULL, "\"");

   if (!objId || !label)
   {
      DtInfo("\tusage: set label <object id> <\"label\">\n"); 
      return;
   }

   controller()->setLabel(DtUUID(objId), label);
}

void DtTextInterface::setLocation(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name, *point;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");
   point = strtok(NULL, " ");

   if (!name || !point)
   {
      DtInfo("\tusage: set location <\"name\"> <pt>\n"); 
      return;
   }

   DtVector vec;
   sscanf(point, "%lf,%lf,%lf", &vec[0], &vec[1], &vec[2]);

   controller()->setLocation(DtUUID(name), vec);
}

void DtTextInterface::setFuel(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name, *amount;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");
   amount = strtok(NULL, " ");

   if (!name || !amount)
   {  
      DtInfo("\tusage: set fuel <\"name\"> <double>\n"); 
      return;
   }

   double fa = atof(amount);
   controller()->setResource(DtUUID(name), "fuel", fa);
}

void DtTextInterface::setTarget(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name, *target;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");
   target = strtok(NULL, "\"");  //get past whitespace
   target = strtok(NULL, "\"");

   if (!name || !target)
   {
      DtInfo("\tusage: set target <\"name\"> <\"name\">\n"); 
      return ;
   }

   controller()->setTarget(DtUUID(name), DtUUID(target));
}

void DtTextInterface::setRestore(char* str)
{
   //Let's parse the command string and then call
   //the corresponding method on DtVrfRemoteController.

   char *name;
   name = strtok(str, " ");
   name = strtok(NULL, "\"");

   if (!name)
   {
      DtInfo("\tusage: set restore <\"name\"> \n"); 
      return;
   }
   controller()->restore(DtUUID(name));
}

void DtTextInterface::scenarioSnapshotResponseCb(DtSimMessage* msg, void* usr)
{
   static_cast<DtTextInterface*>(usr)->controller()->vrfMessageInterface()->removeMessageCallback(
         DtSnapshotsResponseMessageType, scenarioSnapshotResponseCb, usr);
   DtSimInterfaceMessage* iface = dynamic_cast<DtSimInterfaceMessage*>(msg);
   DtIfSnapshotsResponse* rsp = dynamic_cast<DtIfSnapshotsResponse*>(
      iface->interfaceContent());

   DtIfSnapshotsResponse::SnapshotStatus::const_iterator iter = rsp->snapshots().begin();

   DtInfo << "\nSnapshots Available: ";

   if (!rsp->snapshots().size())
   {
      DtInfo << "None" << std::endl;
      return;
   }

   DtInfo << std::endl;

   while (iter != rsp->snapshots().end())
   {
      DtInfo << "   Simulation Time: " << floor(iter->second.simTime) << std::endl;
      ++iter;
   }
}

// Get the snapshots only from the first available back-end.  Should (hopefully) be representative of what all back-ends
// have
void DtTextInterface::listSnapshots()
{
   if (controller()->backendListener()->backends().count())
   {
      controller()->vrfMessageInterface()->removeMessageCallback(
         DtSnapshotsResponseMessageType, scenarioSnapshotResponseCb, this);
      controller()->vrfMessageInterface()->addMessageCallback(
         DtSnapshotsResponseMessageType, scenarioSnapshotResponseCb, this);

      DtIfRequestSnapshots request;
      controller()->vrfMessageInterface()->createAndDeliverMessage(static_cast<DtBackend*>
         (controller()->backendListener()->backendList()->first()->data())->address(), request);
   }
}


