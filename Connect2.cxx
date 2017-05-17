#include "Connect.h"

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
			namespace connect
			{
				//				import static Math.toRadians;
				using com::mak::vrlj::coord::ConstCoordinateTransform;
				using com::mak::vrlj::coord::TopographicCoord;
				using com::mak::vrlj::datatype::DeadReckoningAlgorithm;
				using com::mak::vrlj::datatype::EntityType;
				using com::mak::vrlj::math::ConstTaitBryan;
				using com::mak::vrlj::math::Vector3d;
				using com::mak::vrlj::math::Vector3f;
				using com::mak::vrlj::object::Entity;
				using com::mak::vrlj::object::EntityPublisher;
				using com::mak::vrlj::vl::Clock;
				using com::mak::vrlj::vl::ConnectionType;
				using com::mak::vrlj::vl::ExerciseConnection;
				using com::mak::vrlj::vl::ExerciseConnectionFactory;
				using com::mak::vrlj::vl::ExerciseConnectionInitializer;
				using com::mak::vrlj::vl::ProcessUtil;
				using edu::gmu::c4i::clientlib::BMLClientSTOMP_Lib;
				const std::wstring Connect::DEFAULT_HOSTNAME = L"129.174.95.61";
				const std::wstring Connect::DEFAULT_PORT = L"61613";
				const std::wstring Connect::DEFAULT_TOPIC = L"/topic/BML";
				std::wstring Connect::hostname;
				std::wstring Connect::port;

				void Connect::main(std::vector<std::wstring> &args)
				{
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

				std::wstring Connect::getMessage(BMLClientSTOMP_Lib *c, const std::wstring &hostname, const std::wstring &port)
				{
					std::wstring response = c->connect();
					if (!(response == L"OK"))
					{
						System::err::println(std::wstring(L"Failed to connect to STOMP host \"") + hostname + std::wstring(L"\""));
						System::err::println(response);
						exit(0);
					}
					std::wcout << response << std::endl;

					// If an exception is thrown in trying to get the message, "null" will
					// be returned
					std::wstring message = L"";
					try
					{
						// Using the blocking call here
						message = c->getNext_Block();
					}
					catch (const std::exception &e)
					{
						System::err::println(L"An error occurred in getting STOMP client message");
						e.printStackTrace();
					}
					return message;
				}

				void Connect::setup_VR_Forces()
				{
					// get the unique PID for this process
					// int pid = ProcessUtil.getPid();
					// System.out.println("PID = " + pid);

					int appId = PID & 0xFFFF;
					// System.out.println("appId = " + appId);

					// Set a unique Application ID for items we publish
					initializer->setApplication(appId);

					/*
					* NOTE: The VR-Forces/VR-Link installed on this machine are currently
					* only able to utilize DIS
					*/
					initializer->setDisVersion(7);
					initializer->setRprVersion(2.0);
					initializer->setFederationName(L"MAK-RPR-2.0");
					if (initializer->getConnectionType() == ConnectionType::HLA13)
					{
						initializer->setFedFilename(L"MAK-RPR2-1-1.fed");
					}
					else if (initializer->getConnectionType() == ConnectionType::HLA1516)
					{
						initializer->setFedFilename(L"MAK-RPR2-1-1.xml");
					}
					if (initializer->getConnectionType() == ConnectionType::HLA1516e)
					{
						initializer->setFedFilename(L"RPR_FOM_v2.0_1516-2010.xml");
					}

					initializer->setFederateName(std::wstring(L"vrlj-connect") + std::to_wstring(PID));
				}

				void Connect::start_VR_Forces_interaction()
				{
					//JAVA TO C++ CONVERTER NOTE: The following 'try with resources' block is replaced by its C++ equivalent:
					//ORIGINAL LINE: try (com.mak.vrlj.vl.ExerciseConnection exConn = com.mak.vrlj.vl.ExerciseConnectionFactory.createDefault(initializer); com.mak.vrlj.object.EntityPublisher entityPublisher = exConn.getPublisherFactory().createEntity("talkObject" + PID, new com.mak.vrlj.datatype.EntityType(1, 2, 225, 1, 9, 0, 0));)
					com::mak::vrlj::vl::ExerciseConnection *exConn = com::mak::vrlj::vl::ExerciseConnectionFactory::createDefault(initializer);
					com::mak::vrlj::datatype::EntityType tempVar(1, 2, 225, 1, 9, 0, 0);
					com::mak::vrlj::object::EntityPublisher *entityPublisher = exConn->getPublisherFactory().createEntity(std::wstring(L"talkObject") + std::to_wstring(PID), &tempVar);
					try
					{

						Entity *entity = entityPublisher->getObject();

						// Set visible markings
						entity->setMarkingText(L"ExampleUnit");

						// Set up dead reckoning algorithm
						entity->setDeadReckoningType(DeadReckoningAlgorithm::FPW);

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
						double heading = 0.0;
						double pitch = 0.0;
						double roll = 0.0;
						ConstTaitBryan *geocentricOri = TopographicCoord::getTaitBryan(heading, pitch, roll, geocPos);
						entity->setWorldOrientation(toGeoc->eulerTrans(geocentricOri));

						// Initialize VR-Link time.
						Clock *clock = exConn->getClock();

						// time step in seconds
						double dt = 0.05;
						// starting simulation time in seconds
						double simTime = 0.0;
						double maxSimTime = 25;

						std::wcout << std::wstring(L"Starting simulation movement...") << std::endl;
						while (simTime <= maxSimTime)
						{
							// System.out.println("simTime: " + simTime);
							// Tell VR-Link the current value of simulation time.
							clock->setSimTime(simTime);

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
							delay(static_cast<long long>(1000 * dt));
						}
						std::wcout << std::wstring(L"Simulation movement complete") << std::endl;
					}
					catch (const std::exception &e)
					{
						e.printStackTrace();
					}
				}
			}
		}
	}
}
