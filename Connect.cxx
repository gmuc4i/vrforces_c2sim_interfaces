#include <iostream>
#include "Connect.h"
#include "textIf.h"
#include "remoteControlInit.h"
#include <vl/dataInteraction.h>
#include <vrvControl/DtVrlinkDataInterMarshaller.h>
#include <vrvControl/vrvControlToolkitDataTypes.h>
#include <vl/exerciseConn.h>
#include <vl/exerciseConnInitializer.h>
#include <vlutil/vlKeyboard.h>

//#include <jni.h>
//#include <v1/exerciseConnDIS.h>








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









//VR-Forces headers
#include <vrfMsgTransport/vrfMessageInterface.h>
#include <vrfcontrol/vrfRemoteController.h>

//VR-Link headers
#include <vl/exerciseConn.h>
#include <vlutil/vlProcessControl.h>

namespace com
{
	namespace mak
	{
		namespace vrlj
		{
			namespace Connect
			{
				void setup_VR_Forces();
				//				import static Math.toRadians;




				const std::wstring Connect::DEFAULT_HOSTNAME = L"129.174.95.61";
				const std::wstring Connect::DEFAULT_PORT = L"61613";
				const std::wstring Connect::DEFAULT_TOPIC = L"/topic/BML";
				std::wstring Connect::hostname;
				std::wstring Connect::port;

				void Connect::main(std::vector<std::wstring> &args)
				{


					//DtVrlApplicationInitializer appInit(argc, DtArgv, "Connect");
					//appInit.parseCmdLine();
					//DtExerciseConn exConn(appInit);
					//exConn.setApplicationId(3, 5);


					setup_VR_Forces();

					if (WITH_SERVER)
					{
						setup_STOMP(args);
						constexpr bool CONTINUE_FOREVER = true;
						while (CONTINUE_FOREVER)
						{
							std::wstring message = getMessage(STOMP_CLIENT, hostname, port);
							std::wcout << std::wstring(L"STOMP Server Message:") << std::endl;
							std::wcout << message << std::endl;
							start_VR_Forces_interaction();
						}
					}
					else
					{
						start_VR_Forces_interaction();
					}
				}

				void Connect::setup_STOMP(std::vector<std::wstring> &args)
				{
					STOMP_CLIENT->setHost(DEFAULT_HOSTNAME);
					STOMP_CLIENT->setPort(DEFAULT_PORT);
					STOMP_CLIENT->setDestination(DEFAULT_TOPIC);
				}

				//std::wstring Connect::getMessage(BMLClientSTOMP_Lib *c, const std::wstring &hostname, const std::wstring &port)
				//{
				//	std::wstring response = c->connect();
				//	if (!(response == L"OK"))
				//	{
				//		std::wcout << std::wstring(L"Failed to connect to STOMP host \"") + hostname + std::wstring(L"\"");
				//		//std::wcout << std::wstring(L"Simulation movement complete") << std::endl;
				//		std::wcout << response << std::endl;
				//		exit(0);
				//	}
				//	std::wcout << response << std::endl;

				//	// If an exception is thrown in trying to get the message, "null" will
				//	// be returned
				//	std::wstring message = L"";
				//	try
				//	{
				//		// Using the blocking call here
				//		message = c->getNext_Block();
				//	}
				//	catch (const std::exception &e)
				//	{
				//		//System::err::println(L"An error occurred in getting STOMP client message");
				//		//e.printStackTrace();
				//	}
				//	//return message;
				//}

				void setup_VR_Forces()
				{
					// get the unique PID for this process
					// int pid = ProcessUtil.getPid();
					// System.out.println("PID = " + pid);

					//int appId = PID & 0xFFFF;
					// System.out.println("appId = " + appId);

					// Set a unique Application ID for items we publish

					//initializer->setApplication(appId);

					/*
					* NOTE: The VR-Forces/VR-Link installed on this machine are currently
					* only able to utilize DIS
					*/
					//DtVrlApplicationInitializer initializer;
					/*dtvrlapplicationinitializer initializer("");
					initializer.setdisversiontosend(7);
					initializer.setdisversion(7);
					initializer.setrprversion(2.0);
					initializer.setfederationname(l"mak-rpr-2.0");
					if (initializer.getconnectiontype() == connectiontype::hla13)
					{
					initializer.setfedfilename(l"mak-rpr2-1-1.fed");
					}
					else if (initializer.getconnectiontype() == connectiontype::hla1516)
					{
					initializer->setfedfilename(l"mak-rpr2-1-1.xml");
					}
					if (initializer->getconnectiontype() == connectiontype::hla1516e)
					{
					initializer->setfedfilename(l"rpr_fom_v2.0_1516-2010.xml");
					}

					initializer->setfederatename(std::wstring(l"vrlj-connect") + std::to_wstring(pid));
					}	*/
				}
				int start_VR_Forces_interaction(int argc, char** argv)
				{
					DtRemoteControlInitializer appInitializer(argc, argv);
					appInitializer.parseCmdLine();
					DtVrfRemoteController* controller = new DtVrfRemoteController();
					DtExerciseConn* exConn = new DtExerciseConn(appInitializer);

					controller->init(exConn);
					//controller->loadScenario("Ground-db.mtf");
					controller->loadScenario("Ground-db2222222.mtf");
					//Create our text interface so we can enter text commands.
					DtTextInterface* textIf = new DtTextInterface(controller);

					//Create a tank
					DtEntityType tankType(1, 1, 225, 1, 1, 3, 0);
					DtVector initialPosition(900.0, 900.0, 0.0);
					DtReal initialHeading = 0.0;

					try
					{



						void createEntity(const DtEntityType &type, const DtVector &geocentricPosition,
							DtForceType force, DtReal heading, const DtString &uniqueName = DtString::nullString(),
							const DtString &label = DtString::nullString(), const DtSimulationAddress &addr = DtSimSendToAll,
							bool groundClamp = true);




						DtEntityType tankType(1, 1, 225, 1, 1, 3, 0);
						DtVector initialPosition(900.0, 900.0, 0.0);
						DtReal initialHeading = 0.0;


						createEntity(DtEntityType(1, 1, 225, 1, 1, 3, 0), // a tank


							Entity *entity = entityPublisher->getObject();

						// Set visible markings
						entity->setMarkingText(L"Attack");

						// Set up dead reckoning algorithm
						//entity->setDeadReckoningType(DeadReckoningAlgorithm::FPW);

						/*
						*  TODO:
						*  This function will need to be revised to get units to appear in normal coordinates.
						*  Starting from about here.
						*/

						// Set initial topographic position
						topoPos->setXYZ(0.0, 0.0, 0.0);
						toGeoc->transform(topoPos, geocPos);
						entity->setWorldPosition(geocPos);


						// Set initial topographic velocity
						topoVel->setXYZ(40.0f, 20.0f, 0.0f);

						// Set corresponding geocentric velocity vector
						entity->setWorldVelocity(toGeoc->vecTrans(topoVel));


						// Initialize topographic orientation
						double heading = 57.0;
						double pitch = -54.0;
						double roll = -180.0;

						/*double heading = -17.0;
						double pitch = 5.0;
						double roll = 18.0;*/

						//ConstTaitBryan *geocentricOri = TopographicCoord::getTaitBryan(heading, pitch, roll, geocPos);
						entity->setWorldOrientation(toGeoc->eulerTrans(geocentricOri));

						// Initialize VR-Link time.
						DtClock* Clock = exConn->clock();
						//Clock *clock = exConn->getClock();
						//heading--;
						// time step in seconds
						double dt = 0.05;
						// starting simulation time in seconds
						double simTime = 0.0;
						double maxSimTime = 25;
						//double totaltime=50;
						std::wcout << std::wstring(L"Starting simulation movement...") << std::endl;


						while (simTime <= maxSimTime)
						{
							// System.out.println("simTime: " + simTime);
							// Tell VR-Link the current value of simulation time.
							Clock->setSimTime(simTime);

							// Process any incoming messages.
							exConn->drainInput();

							// Update our published entity from our topographic model
							entity->setWorldPosition(toGeoc->transform(topoPos));

							// Tick the updated model
							entityPublisher->tick();

							// topographic equation of motion:
							offset->assignFrom(topoVel);
							offset->multiply(dt);
							topoPos->add(offset);

							// Increment the simulation time
							simTime += dt;

							// Sleep till next iteration, in milliseconds.
							DtTime sleeptime = (simTime += dt) - Clock->elapsedRealTime();
							DtSleep(sleeptime); // returns immediately if negative
						}
					}
					catch (const std::exception &e)
					{
					}
				}
			}
		}
	}
}
//std::wcout << std::wstring(L"Simulation movement complete") << std::endl;
