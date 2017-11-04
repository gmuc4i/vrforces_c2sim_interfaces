/*
 * testStomClient.cpp
 *
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
 *  Created on: Aug 22, 2010
 *      Author: "Mark Eschbach" &lt;meschbach@gmail.c&gt;
 * $HeadURL: file:///home/gremlin/backups/rcs/svn/software/msg-util/trunk/stomp-util/testStompClient.cpp $
 * $Id: testStompClient.cpp 2497 2010-11-05 20:13:38Z meschbach $
 */
#include "StompClient.h"
#include <gtest/gtest.h>

TEST(StompClient,Connect_Send) {
	EXPECT_NO_THROW( {
				std::stringstream buffer;
				StompClient client(buffer);
				client.sendConnect();

				std::string token;
				std::getline(buffer,token);
				EXPECT_EQ(token,std::string("CONNECT"));
			});
}

TEST(StompClient,ReceiveProperty_Good) {
	try {
		std::stringstream buffer;
		buffer << "Test: property\n";

		struct StompFrame frame;
		StompClient client(buffer);
		std::pair<std::string,std::string> p = client.receiveProperty();

		EXPECT_EQ(p.first,"Test");
		EXPECT_EQ(p.second,"property");
	} catch(StompException &e) {
		FAIL() << "Caught exception: " << e.what();
	}
}

TEST(StompClient, RecieveBadProperty_WithNL) {
	try {
		std::stringstream buffer;
		buffer << "Test\n";

		struct StompFrame frame;
		StompClient client(buffer);
		std::pair<std::string,std::string> p = client.receiveProperty();
		FAIL() << "Property is invalid without colon";
	} catch(StompException &e) {
		SUCCEED();
	}
}

TEST(StompClient, ReceiveEmptyProperties) {
	try {
		std::stringstream buffer;
		buffer << "\n";

		StompClient client(buffer);
		std::map<std::string,std::string> props;
		client.receiveProperties(props);

		EXPECT_TRUE(props.empty());
	} catch(StompException &e) {
		FAIL() << "Exception caught: " << e.what();
	}
}

TEST(StompClient, ReceiveOneProperties) {
	try {
		std::stringstream buffer;
		buffer << "One: Property\n\n";

		StompClient client(buffer);
		std::map<std::string,std::string> props;
		client.receiveProperties(props);

		EXPECT_TRUE(props.size() == 1);
	} catch(StompException &e) {
		FAIL() << "Exception caught: " << e.what();
	}
}

TEST(StompClient, ReceiveTwoProperties) {
	try {
		std::stringstream buffer;
		buffer << "One: Property\n";
		buffer << "Two: 2\n";
		buffer << "\n";

		StompClient client(buffer);
		std::map<std::string,std::string> props;
		client.receiveProperties(props);

		EXPECT_TRUE(props.size() == 2);
	} catch(StompException &e) {
		FAIL() << "Exception caught: " << e.what();
	}
}

TEST(StompClient,Connect_Recieved_Good_Frame) {
	try {
		struct StompFrame frame;
		frame.operation = "CONNECTED";
		frame.properties.insert(std::pair<std::string,std::string>("session","1234"));

		std::stringstream buffer;
		StompClient client(buffer);
		client.processConnect(frame);
		EXPECT_EQ(client.getSessionID(),"1234");
	} catch(StompException &e) {
		FAIL() << "Caught exception: " << e.what();
	}
}

TEST(StompClient,FrameTerminatorConsumed) {
	try {
		struct StompFrame firstFrame, secondFrame;

		std::stringstream buffer;
		buffer << "ONE\n\n";
		buffer.put(0);
		buffer << "TWO\n\n";
		buffer.put(0);

		StompClient client(buffer);
		client.receiveFrame(firstFrame);
		client.receiveFrame(secondFrame);

		EXPECT_EQ(firstFrame.operation, "ONE");
		EXPECT_EQ(secondFrame.operation, "TWO");
	} catch(StompException &e) {
		FAIL() << "Caught exception: " << e.what();
	}
}

