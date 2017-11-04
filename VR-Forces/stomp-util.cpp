/*
 * stomp-util.cpp
 * Copyright (c) 2010, Mark Eschbach 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Mark Eschbach nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Mark Eschbach BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Created on: Aug 23, 2010
 *      Author: "Mark Eschbach" &lt;meschbach.com&gt;
 *      $HeadURL: file:///home/gremlin/backups/rcs/svn/software/msg-util/trunk/stomp-util/stomp-util.cpp $
 *      $Id: stomp-util.cpp 2501 2010-11-08 05:29:49Z meschbach $
 */

#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include "stomp-util.h"

/*
 * boost::program options contain the current argument parsing.
 * These are utilized in ::configOptions.
 */
using namespace boost::program_options;

/**
 * Parses the command line arguments and decides on how our application is configured.
 *
 * @param config is a structure describing our configuration
 * @param argc is the array size of argv
 * @param argv is an array of pointers to C-style strings
 * @returns true if the configuration setup, otherwise false
 */
bool configureOptions(struct Configuration& config, int argc, char** argv) {
	bool validConfig;
	/*
	 * Establish our parameters
	 */
	options_description options;
	options.add_options()(
	        "host",
	        value<std::string> (),
	        "STOMP broker host [default: localhost]")("port", value<
	        unsigned short> (), "STOMP port [default: 61613]")(
	        "help",
	        "Displays help message")(
	        "dest",
	        value<std::string> (),
	        "Destination [default: /queue/stomp]")(
	        "auth",
	        value<std::string> (),
	        "Read the given file for login credentials, with the first line user name and the second the password")(
	        "ctrl",
	        value<std::string> (),
	        "Control queue for receive disconnects [default: /topic/stomp-ctrl]");
	/*
	 * Parse our options
	 */
	variables_map args;
	store(parse_command_line(argc, argv, options), args);
	notify(args);
	/*
	 * Have we been asked for help?
	 */
	/*
	 * If the user desires the help message, print that
	 */
	if (args.count("help")) {
		std::cout << options << std::endl;
		validConfig = false;
	} else {
		/*
		 * Decide on what host we will use
		 */
		if (args.count("host")) {
			config.host = args["host"].as<std::string> ();
		} else {
			config.host = "localhost";
		}
		/*
		 * Decide the port to use
		 */
		if (args.count("port")) {
			config.port = args["port"].as<unsigned short> ();
		} else {
			config.port = 61613;
		}
		/*
		 * Decide on our destination
		 */
		if (args.count("dest")) {
			config.destination = args["dest"].as<std::string> ();
		} else {
			config.destination = "/queue/stomp";
		}
		/*
		 * Decide on our control queue
		 */
		if (args.count("ctrl")) {
			config.control = args["ctrl"].as<std::string> ();
		} else {
			config.control = "/topic/stomp-ctrl";
		}
		validConfig = true;
		/*
		 * Check to see if the authentication credentials are specified
		 */
		if (args.count("auth")) {
			std::string authFileName = args["auth"].as<std::string> ();
			std::ifstream fcred(authFileName.c_str());
			fcred.exceptions(std::istream::badbit | std::istream::failbit
			        | std::istream::eofbit);
			std::getline(fcred, config.user);
			std::getline(fcred, config.password);
			config.useLogin = true;
		}
	}
	return validConfig;
}
