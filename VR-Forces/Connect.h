#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

/// <summary>
///*******************************************************************************
/// ** Copyright (c) 2015 VT MAK
/// ** All rights reserved.
/// ********************************************************************************
/// </summary>

/*
* This file is located here:
* C:\Users\SOFTWARE DEVELOPMENT USER\workspace\VR-Link Program\src\com\mak\vrlj\connect
*/

namespace com
{
	namespace mak
	{
		namespace vrlj
		{
			namespace Connect
			{

				//JAVA TO C++ CONVERTER TODO TASK: The Java 'import static' statement cannot be converted to C++:
				//				import static Math.toRadians;

				//using com::mak::vrlj::COORD::ConstCoordinateTransform;
				//using com::mak::vrlj::COORD::DtCon
			//using com::mak::vrlj::coord::TopographicCoord;
				////////import com.mak.vrlj.datatype.StartOfMessage;
				//////using com::mak::vrlj::math::Vector3d;
				//////using com::mak::vrlj::math::Vector3f;
				//////using com::mak::vrlj::vl::ConnectionType;
				//////using com::mak::vrlj::vl::ExerciseConnectionInitializer;
				//////using com::mak::vrlj::vl::ProcessUtil;

				//using edu::gmu::c4i::clientlib::BMLClientSTOMP_Lib;

				/// <summary>
				/// Example which publishes a single, simple simulation entity and demonstrates
				/// various VRLJ features.
				/// </summary>
				class Connect
				{

					/*
					* =====================
					*
					* VR-FORCES EXAMPLE VARIABLES
					*
					* =====================
					*/

					/*
					* Origin latitude for topographic projection.
					*/
				 const double refLatitude = toRadians(36.004755);

					/*
					* Origin longitude for topographic projection.
					*/
					const double refLongitude = toRadians(-122.994539);

					/*
					* Transform from geocentric to topographic coordinates.
					*/
					//DtCoordTransform *const  toGeoc = Coordinate_System::topoToGeocTransform(refLatitude, refLongitude);
					//ConstCoordinateTransform *const toGeoc = TopographicCoord::topoToGeocTransform(refLatitude, refLongitude);

					/*
					* Position of entity in topographic coordinates.
					*/
					DtVector3D* const topoPos = new DtVector3D();

					//Vector3d *const topoPos = new Vector3d();

					//static Vector3d *const topoPos1 = new Vector3d();

					//static Vector3d *const topoPos2 = new Vector3d();

					/*
					* Position of entity in geocentric coordinates.
					*/

					DtVector3D* const geocPos = new DtVector3D();
					
					//static Vector3d *const geocPos = new Vector3d();

					//static Vector3d *const geocPos1 = new Vector3d();

					/*
					* Velocity of entity in topographic coordinates.
					*/

					DtVector3D* const topoVel = new DtVector3D();
					//static Vector3f *const topoVel = new Vector3f();


					/*
					* Velocity offset in equation of motion.
					*/
					DtVector3D* const offset = new DtVector3D();

					//static Vector3d *const offset = new Vector3d();

					DtExerciseConnInitializer* initializer = new  DtExerciseConnInitializer();

					//ExerciseConnectionInitializer *const initializer = new ExerciseConnectionInitializer(ConnectionType::DIS);

					//static const int PID = ProcessUtil::getPid();
					
					/*
					* =====================
					*
					* BML STOMP CLIENT VARS
					*
					* =====================
					*/

					/*
					* Allows access to the BML Client STOMP Library
					*/
					//BMLClientSTOMP_Lib *const STOMP_CLIENT = new BMLClientSTOMP_Lib();
					
					/*
					* Information relating to the STOMP server
					*/
					static const std::wstring DEFAULT_HOSTNAME;
					static const std::wstring DEFAULT_PORT;
					static const std::wstring DEFAULT_TOPIC;
					static std::wstring hostname;
					static std::wstring port;

					// If you're only testing VR-Forces interaction, this should be "false"
					static constexpr bool WITH_SERVER = true;

					static void main(std::vector<std::wstring> &args);

					static void setup_STOMP(std::vector<std::wstring> &args);

					/*
					* Encapsulates the non-setup logic of Doug Corner's BML STOMP Client
					*
					* This function returns the message from the STOMP server
					*/
					//static std::wstring getMessage(BMLClientSTOMP_Lib *c, const std::wstring &hostname, const std::wstring &port);

					static void setup_VR_Forces();

					/*
					* The rest of this is from the VR-Link "Talk" example.
					*
					* For details, consult this file:
					* "C:\MAK\vrlink5.2.1\java\examples\talk\src\com\mak\vrlj\example\talk"
					*/
					static void start_VR_Forces_interaction();
				};

			}
		}
	}
}
