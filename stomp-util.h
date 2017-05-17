/*
 * stomp-util.h
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
 *      $HeadURL: file:///home/gremlin/backups/rcs/svn/software/msg-util/trunk/stomp-util/stomp-util.h $
 *      $Id: stomp-util.h 2501 2010-11-08 05:29:49Z meschbach $
 */

#ifndef STOMP_UTIL_H_
#define STOMP_UTIL_H_

#include "StompClient.h"

/**
 * The Configuration represents our program configuration
 * to complete its goal.
 */
struct Configuration {
	std::string host;
	unsigned short port;
	std::string destination;
	std::string control;

	bool useLogin;
	std::string user;
	std::string password;
};

/**
 * Parses the command line arguments and decides on how our application is configured.
 *
 * @param config is a structure describing our configuration
 * @param argc is the array size of argv
 * @param argv is an array of pointers to C-style strings
 */
bool configureOptions(struct Configuration& config, int argc, char** argv);

/**
 * The performStompTask is called by the stomp-main module when the application
 * connects to its configured remote broker and is ready to operate on the connection.
 * After this function returns, the Stomp connection is disconnected and the
 * connection is cleaned up.
 */
void performStompTask(const struct Configuration& config, mee::stomp::StompClient& client);

#endif /* STOMP_UTIL_H_ */

