/*********************************************************************************
 ** Copyright (c) 2015 VT MAK
 ** All rights reserved.
 *********************************************************************************/

/*
 * This file is located here:
 * C:\Users\SOFTWARE DEVELOPMENT USER\workspace\VR-Link Program\src\com\mak\vrlj\connect
 */

package com.mak.vrlj.connect;

import static java.lang.Math.toRadians;

import com.mak.vrlj.coord.ConstCoordinateTransform;
import com.mak.vrlj.coord.TopographicCoord;
import com.mak.vrlj.datatype.DeadReckoningAlgorithm;
import com.mak.vrlj.datatype.EntityType;
//import com.mak.vrlj.datatype.StartOfMessage;
import com.mak.vrlj.math.ConstTaitBryan;
import com.mak.vrlj.math.Vector3d;
import com.mak.vrlj.math.Vector3f;
import com.mak.vrlj.object.Entity;
import com.mak.vrlj.object.EntityPublisher;
import com.mak.vrlj.vl.Clock;
import com.mak.vrlj.vl.ConnectionType;
import com.mak.vrlj.vl.ExerciseConnection;
import com.mak.vrlj.vl.ExerciseConnectionFactory;
import com.mak.vrlj.vl.ExerciseConnectionInitializer;
import com.mak.vrlj.vl.ProcessUtil;

import edu.gmu.c4i.clientlib.BMLClientSTOMP_Lib;

/**
 * Example which publishes a single, simple simulation entity and demonstrates
 * various VRLJ features.
 */
public class Connect
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
	private static final double refLatitude = toRadians(35.699760);

	/*
	 * Origin longitude for topographic projection.
	 */
	private static final double refLongitude = toRadians(-121.326577);

	/*
	 * Transform from geocentric to topographic coordinates.
	 */
	private static final ConstCoordinateTransform toGeoc = TopographicCoord.topoToGeocTransform(refLatitude,
			refLongitude);

	/*
	 * Position of entity in topographic coordinates.
	 */
	private static final Vector3d topoPos = new Vector3d();

	/*
	 * Position of entity in geocentric coordinates.
	 */
	private static final Vector3d geocPos = new Vector3d();

	/*
	 * Velocity of entity in topographic coordinates.
	 */
	private static final Vector3f topoVel = new Vector3f();

	/*
	 * Velocity offset in equation of motion.
	 */
	private static final Vector3d offset = new Vector3d();

	private static final ExerciseConnectionInitializer initializer = new ExerciseConnectionInitializer(
			ConnectionType.DIS);

	private static final int PID = ProcessUtil.getPid();

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
	private static final BMLClientSTOMP_Lib STOMP_CLIENT = new BMLClientSTOMP_Lib();

	/*
	 * Information relating to the STOMP server
	 */
	private static final String DEFAULT_HOSTNAME = "129.174.95.61";
	private static final String DEFAULT_PORT = "61613";
	private static final String DEFAULT_TOPIC = "/topic/BML";
	private static String hostname;
	private static String port;
	
	// If you're only testing VR-Forces interaction, this should be "false"
	private static final boolean WITH_SERVER = true;

	public static void main(String[] args)
	{
		setup_VR_Forces();
		
		if(WITH_SERVER)
		{
			setup_STOMP(args);
			final boolean CONTINUE_FOREVER = true;
			while(CONTINUE_FOREVER)
			{
				String message = getMessage(STOMP_CLIENT, hostname, port);
				System.out.println("STOMP Server Message:");
				System.out.println(message);
				start_VR_Forces_interaction();
			}
		}
		else
		{
			start_VR_Forces_interaction();
		}
	}

	private static void setup_STOMP(String[] args)
	{
		STOMP_CLIENT.setHost(DEFAULT_HOSTNAME);
		STOMP_CLIENT.setPort(DEFAULT_PORT);
		STOMP_CLIENT.setDestination(DEFAULT_TOPIC);
	}

	/*
	 * Encapsulates the non-setup logic of Doug Corner's BML STOMP Client
	 * 
	 * This function returns the message from the STOMP server
	 */
	private static String getMessage(BMLClientSTOMP_Lib c, String hostname, String port)
	{
		String response = c.connect();
		if (!(response.equals("OK")))
		{
			System.err.println("Failed to connect to STOMP host \"" + hostname + "\"");
			System.err.println(response);
			System.exit(0);
		}
		System.out.println(response);

		// If an exception is thrown in trying to get the message, "null" will
		// be returned
		String message = null;
		try
		{
			// Using the blocking call here
			message = c.getNext_Block();
		}
		catch (Exception e)
		{
			System.err.println("An error occurred in getting STOMP client message");
			e.printStackTrace();
		}
		return message;
	}

	private static void setup_VR_Forces()
	{
		// get the unique PID for this process
		// int pid = ProcessUtil.getPid();
		// System.out.println("PID = " + pid);

		int appId = PID & 0xFFFF;
		// System.out.println("appId = " + appId);

		// Set a unique Application ID for items we publish
		initializer.setApplication(appId);

		/*
		 * NOTE: The VR-Forces/VR-Link installed on this machine are currently
		 * only able to utilize DIS
		 */
		initializer.setDisVersion(7);
		initializer.setRprVersion(2.0);
		initializer.setFederationName("MAK-RPR-2.0");
		if (initializer.getConnectionType() == ConnectionType.HLA13)
		{
			initializer.setFedFilename("MAK-RPR2-1-1.fed");
		}
		else if (initializer.getConnectionType() == ConnectionType.HLA1516)
		{
			initializer.setFedFilename("MAK-RPR2-1-1.xml");
		}
		if (initializer.getConnectionType() == ConnectionType.HLA1516e)
		{
			initializer.setFedFilename("RPR_FOM_v2.0_1516-2010.xml");
		}

		initializer.setFederateName("vrlj-connect" + PID);
	}

	/*
	 * The rest of this is from the VR-Link "Talk" example.
	 * 
	 * For details, consult this file:
	 * "C:\MAK\vrlink5.2.1\java\examples\talk\src\com\mak\vrlj\example\talk"
	 */
	private static void start_VR_Forces_interaction()
	{
		try (ExerciseConnection exConn = ExerciseConnectionFactory.createDefault(initializer);
				EntityPublisher entityPublisher = exConn.getPublisherFactory().createEntity("talkObject" + PID,
						new EntityType(1, 2, 225, 1, 9, 0, 0));)
		{
			
			Entity entity = entityPublisher.getObject();
			
			// Set visible markings
			entity.setMarkingText("ExampleUnit");

			// Set up dead reckoning algorithm
			entity.setDeadReckoningType(DeadReckoningAlgorithm.FPW);

			/*
			 *  TODO:
			 *  This function will need to be revised to get units to appear in normal coordinates.
			 *  Starting from about here.
			 */
			
			// Set initial topographic position
			topoPos.setXYZ(0.0, 0.0, 0.0);
			toGeoc.transform(topoPos, geocPos);
			entity.setWorldPosition(geocPos);

			// Set initial topographic velocity
			topoVel.setXYZ(40.0f, 20.0f, 0.0f);

			// Set corresponding geocentric velocity vector
			entity.setWorldVelocity(toGeoc.vecTrans(topoVel));

			// Initialize topographic orientation
			double heading = 0.0;
			double pitch = 0.0;
			double roll = 0.0;
			ConstTaitBryan geocentricOri = TopographicCoord.getTaitBryan(heading, pitch, roll, geocPos);
			entity.setWorldOrientation(toGeoc.eulerTrans(geocentricOri));

			// Initialize VR-Link time.
			Clock clock = exConn.getClock();

			// time step in seconds
			double dt = 0.05;
			// starting simulation time in seconds
			double simTime = 0.0;
			double maxSimTime = 25;
			
			System.out.println("Starting simulation movement...");
			while (simTime <= maxSimTime)
			{
				// System.out.println("simTime: " + simTime);
				// Tell VR-Link the current value of simulation time.
				clock.setSimTime(simTime);

				// Process any incoming messages.
				exConn.drainInput();

				// Update our published entity from our topographic model
				entity.setWorldPosition(toGeoc.transform(topoPos));

				// Tick the updated model
				entityPublisher.tick();

				// topographic equation of motion:
				offset.assignFrom(topoVel);
				offset.multiply(dt);
				topoPos.add(offset);

				// Increment the simulation time
				simTime += dt;

				// Sleep till next iteration, in milliseconds.
				Thread.sleep((long) (1000 * dt));
			}
			System.out.println("Simulation movement complete");
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}
}
