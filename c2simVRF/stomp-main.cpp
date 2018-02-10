/*
 * stomp-main.cpp
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
 *      $HeadURL: file:///home/gremlin/backups/rcs/svn/software/msg-util/trunk/stomp-util/stomp-main.cpp $
 *      $Id: stomp-main.cpp 2501 2010-11-08 05:29:49Z meschbach $
 */

#include <boost/asio.hpp>


#include <iostream>


#include "stomp-util.h"


using namespace mee::stomp;


/**
 * Application entry point
 */

int main(int argc, char** argv) {

	char result = -1;

	try {

		/*
		 * Configure our options
		 */

		struct Configuration config;

		if (configureOptions(config, argc, argv)) {
			/*
			 * Set our return status
			 */

			result = 0;

			/*
			 * Establish the connection
			 */

			std::stringstream buf;

			buf << config.port;


			boost::asio::ip::tcp::iostream stream;

			stream.connect(config.host, buf.str()	);


			if (stream) {

				StompClient client(stream);

				client.connect();

				performStompTask(config, client);

				client.disconnect();

			} else {

				std::cerr

				        << "Failed to connect to remote (Socket API only returned failure) to "

				        << config.host << ":" << config.port

				        << std::endl;

			}
		}else{
#ifdef DEBUG

			std::cout << "Configuration returned false" << std::endl;
#endif /* DEBUG */

		}

	}catch(StompException e){

		std::cerr << "Exception: " << e.what() << std::endl;
	}catch(std::exception &e){

		std::cerr << "Exception: " << e.what() << std::endl;
	} catch (...) {

		std::cerr << "An unexpected exception has occured." << std::endl;

	}

	return result;

}

