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

import java.util.Iterator;

import com.mak.vrlj.coord.ConstCoordinateTransform;
import com.mak.vrlj.coord.TopographicCoord;
import com.mak.vrlj.datatype.DeadReckoningAlgorithm;
import com.mak.vrlj.datatype.EntityType;
import com.mak.vrlj.interaction.FireInteraction;
//import com.mak.vrlj.datatype.StartOfMessage;
import com.mak.vrlj.math.ConstTaitBryan;
import com.mak.vrlj.math.Vector3d;
import com.mak.vrlj.math.Vector3f;
import com.mak.vrlj.object.BaseObject;
import com.mak.vrlj.object.Entity;
import com.mak.vrlj.object.EntityPublisher;
import com.mak.vrlj.object.GlobalObjectDesignator;
import com.mak.vrlj.object.ObjectIdentifier;
import com.mak.vrlj.vl.Clock;
import com.mak.vrlj.vl.ConnectionType;
import com.mak.vrlj.vl.ExerciseConnection;
import com.mak.vrlj.vl.ExerciseConnectionFactory;
import com.mak.vrlj.vl.ExerciseConnectionInitializer;
import com.mak.vrlj.vl.ProcessUtil;

import edu.gmu.c4i.clientlib.BMLClientSTOMP_Lib;


import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.stream.Location;
import javax.xml.parsers.DocumentBuilder;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.Node;
import org.w3c.dom.Element;
import java.io.File;

import java.lang.*;
import java.io.*;
import geotransform.coords.*;
import geotransform.ellipsoids.*;
import geotransform.transforms.*;

/**
 * Example which publishes a single, simple simulation entity and demonstrates
 * various VRLJ features.
 */
public class Connect2
{

	static final int MAX_POINTS = 1; // total number of points to be define in GDC

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
		 /*Initiater initiater = new Initiater();
	        Responder responder = new Responder();

	        initiater.addListener(responder);
	        initiater.sayMove();*/
			
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
			//As the entity is moving very fast so assigning low value velocity to see whether its working or not
			topoVel.setXYZ(0.001f, 0.001f, 0.0f);

			///DtTopoView topoView(esr, DtDeg2Rad(36.0), DtDeg2Rad(-121.0));
			
			// Set corresponding geocentric velocity vector
			
			
			
			
			/*int i; // iterator
			double lat, lon;
			String s1, s2;
			DataInput d = new DataInputStream(System.in);

		        // Gdc_Coord_3d gdc[] = new Gdc_Coord_3d[MAX_POINTS]; // these need to be the same length.
		        // Utm_Coord_3d utm[] = new Utm_Coord_3d[MAX_POINTS];
		        
			Gdc_Coord_3d gdc_point = new Gdc_Coord_3d();
			Utm_Coord_3d utm_point = new Utm_Coord_3d();
		        
		        Gdc_To_Utm_Converter.Init(new WE_Ellipsoid());        

			 
			 
		  
		 gdc_point = new Gdc_Coord_3d(Double.valueOf(s1).doubleValue(), Double.valueOf(s2).doubleValue(),100.0);
			  // convert the points.
		     
			  Gdc_To_Utm_Converter.Convert(gdc_point,utm_point); // with points
		        
			  System.out.println("\nGdc.latitude: " + gdc_point.latitude);
			  System.out.println("Gdc.longitude: " + gdc_point.longitude);
			  System.out.println("Gdc.elevation: " + gdc_point.elevation);
		        
			  System.out.println("\nUtm.x: " + utm_point.x);
			  System.out.println("Utm.y: " + utm_point.y);
			  System.out.println("Utm.z: " + utm_point.z);
			  System.out.println("Utm.zone: " + utm_point.zone);
			  System.out.println("Utm.hemisphere_north: " + utm_point.hemisphere_north);*/
			 // while
			
			
			
			//entity.setWorldVelocity(toGeoc.vecTrans(topoVel));
			//entity.setWorldPosition(new Vector3d(40.500965, 47.136887, 0.0));
			//entityPublisher.tick();
			
			/*entity.setWorldPosition(new Vector3d(1524517.05424356, -4464444.01466548, 4278068.90596911));
			entityPublisher.tick();*/
			
			//topoVel.setXYZ(40.0f, 20.0f, 0.0f);
			//entity.setWorldPosition(new Vector3d(1524517.05424356, -4464444.01466548, 4278068.90596911));
			//entity.setWorldPosition(new Vector3d(utm_point.x, utm_point.y, utm_point.z));
			//entityPublisher.tick();
			
			// Initialize topographic orientation
			//double world = 6.0;
			double heading = 0.0;
			double pitch = 0.0;
			double roll = 0.0;
			//double Yaw = 4.0;
			
			
			/*Location location = new Location(world, heading, pitch, roll);
			entity.setLocation(new Location(w, x, y, z, (float) yaw, (float) pitch));*/
			
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
			System.out.println("Just an Example, Parsing the code until the Simulation is completed");
			
				while (simTime <= maxSimTime)
				{
					//Taking a small xml file
					File fXmlFile = new File("/users/c2sim/AttackOrder.xml");
					DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
					DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
					Document doc = dBuilder.parse(fXmlFile);

					//optional, but recommended
					//read this - http://stackoverflow.com/questions/13786607/normalization-in-dom-parsing-with-java-how-does-it-work
					doc.getDocumentElement().normalize();

					System.out.println("Root element :" + doc.getDocumentElement().getNodeName());

					NodeList nList = doc.getElementsByTagName("WhereLocation");

					System.out.println("----------------------------");

					for (int temp = 0; temp < nList.getLength(); temp++) {

						Node nNode = nList.item(temp);

						//System.out.println("\nCurrent Element :" + nNode.getNodeName());

						if (nNode.getNodeType() == Node.ELEMENT_NODE) {

							Element eElement = (Element) nNode;
							
							//System.out.println("Order ID : " + eElement.getElementsByTagName("OrderID").item(0).getTextContent());
							System.out.println("Sequence : " + eElement.getAttribute("Sequence"));
							System.out.println("Latitude : " + eElement.getElementsByTagName("Latitude").item(0).getTextContent());
							System.out.println("Longitude : " + eElement.getElementsByTagName("Longitude").item(0).getTextContent());
							System.out.println("ElevationAGL : " + eElement.getElementsByTagName("ElevationAGL").item(0).getTextContent());
							System.out.println("TaskersIntent : " + eElement.getElementsByTagName("TaskersIntent").item(0).getTextContent());
							//System.out.println("WhereID : " + eElement.getElementsByTagName("WhereID").item(0).getTextContent());
							//System.out.println("OrderID : " + eElement.getElementsByTagName("OrderID").item(0).getTextContent());
							
							int i; // iterator
							double lat, lon;
							//String s1, s2;
							//DataInput d = new DataInputStream(System.in);

						        // Gdc_Coord_3d gdc[] = new Gdc_Coord_3d[MAX_POINTS]; // these need to be the same length.
						        // Utm_Coord_3d utm[] = new Utm_Coord_3d[MAX_POINTS];
						        
								Gdc_Coord_3d gdc_point = new Gdc_Coord_3d();
								Utm_Coord_3d utm_point = new Utm_Coord_3d();
						        
						        Gdc_To_Utm_Converter.Init(new WE_Ellipsoid());      
						        
						        gdc_point = new Gdc_Coord_3d(Double.valueOf(eElement.getElementsByTagName("Latitude").item(0).getTextContent()).doubleValue(), Double.valueOf(eElement.getElementsByTagName("Longitude").item(0).getTextContent()).doubleValue(), Double.valueOf(eElement.getElementsByTagName("ElevationAGL").item(0).getTextContent()));
							  
						        // convert the points.
						     
							  Gdc_To_Utm_Converter.Convert(gdc_point,utm_point); // Convert with points
						        
							  System.out.println("\nGdc.latitude: " + (Double.valueOf(eElement.getElementsByTagName("Latitude").item(0).getTextContent()).doubleValue()));
							  System.out.println("Gdc.longitude: " + (Double.valueOf(eElement.getElementsByTagName("Longitude").item(0).getTextContent()).doubleValue()));
							  System.out.println("Gdc.elevation: " + (Double.valueOf(eElement.getElementsByTagName("ElevationAGL").item(0).getTextContent()).doubleValue()));
						        
							  System.out.println("\nUtm.x: " + utm_point.x);
							  System.out.println("Utm.y: " + utm_point.y);
							  System.out.println("Utm.z: " + utm_point.z);
							  System.out.println("Utm.zone: " + utm_point.zone);
							  System.out.println("Utm.hemisphere_north: " + utm_point.hemisphere_north);
						
							
					
					// System.out.println("simTime: " + simTime);
					// Tell VR-Link the current value of simulation time.
					clock.setSimTime(simTime);

					// Process any incoming messages.
					exConn.drainInput();
					//entity.setWorldPosition(new Vector3d(utm_point.x, utm_point.y, utm_point.z));
					entity.setMarkingText("Attack");

					// Update our published entity from our topographic model
					entity.setWorldVelocity(toGeoc.vecTrans(topoVel));
					entity.setWorldPosition(new Vector3d(utm_point.x, utm_point.y, utm_point.z));
					//entity.setWorldPosition(toGeoc.transform(topoPos));
					
					

				// Tick the updated model
				entityPublisher.tick();
				
				//Call the Handler class
				Initiater initiater = new Initiater();
		        Responder responder = new Responder();

		        initiater.addListener(responder);
		        initiater.sayAttack();

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
		}
		}
		
		catch (Exception e)
		{
			e.printStackTrace();
		}
	
			
		//Now of no use, but can be used for callbacks and interactions
		
		ExerciseConnection exConn = null;
		Iterator<BaseObject<? extends ObjectIdentifier>> objIter = exConn.getReflectedCollection().values().iterator();
		while(objIter.hasNext())
		{
		   BaseObject<? extends ObjectIdentifier> obj = objIter.next();
		   if(obj instanceof Entity)
		   {
		      Entity e = (Entity) obj;
		      System.out.println(e.toString());
		   }
		}
		
		FireInteraction fire = new FireInteraction();
		GlobalObjectDesignator myId = null;
		fire.setFiringId(myId );
		exConn.send(fire);
	}
}
		
	
