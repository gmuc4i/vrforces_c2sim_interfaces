/*
 * stomp-recv.cpp
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
 *      $HeadURL: file:///home/gremlin/backups/rcs/svn/software/msg-util/trunk/stomp-util/stomp-recv.cpp $
 *      $Id: stomp-recv.cpp 2501 2010-11-08 05:29:49Z meschbach $
 */
#include "stomp-util.h"

using namespace mee::stomp;

void performStompTask(const struct Configuration& config, StompClient& client) {
	/*
	 * Subscribe to our destination
	 */
	client.subscribe(config.destination, Client);
	client.subscribe(config.control, Client);
	/*
	 * While we haven't received an exit message
	 */
	bool continueProcessing = true;
	do {
		StompFrame frame;
		client.receiveFrame(frame);

		if (frame.properties["destination"] == config.control) {
			std::cout << "Exiting" << std::endl;
			continueProcessing = false;
		} else {
			std::cout << "Destination: " << frame.properties[std::string(
			        "destination")] << std::endl;
			std::cout << frame.message.str() << std::endl;
		}
		client.acknowledge(frame.properties["message-id"]);
	} while (continueProcessing);
	/*
	 * Unsubscribe
	 */
	client.unsubscribe(config.control);
	client.unsubscribe(config.destination);
}