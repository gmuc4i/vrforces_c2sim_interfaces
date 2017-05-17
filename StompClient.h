/*
 * StompClient.h
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
 *      Author: "Mark Eschbach" <meschbach@gmail.com>
 *
 * $HeadURL: file:///home/gremlin/backups/rcs/svn/software/msg-util/trunk/stomp-util/StompClient.h $
 * $Id: StompClient.h 2566 2010-12-12 09:39:15Z meschbach $
 *
 * C++ STOMP Client components.
 *
 * File Version History:
 * 	Version 0.0.1
 *		Initial revision
 *
 *	Version 1.0.0
 *		 Initial working copy used in production code and released 
 *	to the public under BSD.
 *
 *	Version 1.1.0 (2010-10-24)
 *		Migrated the code base to the C++ namespace mee::stomp.
 *	Split the I/O streams in the StompClient class to provide easier
 *	testing and diagnostics.
 *
 *	Version 1.1.1 (2010-10-25)
 *		Typedefined the structure StompFrame to be easier to operate against in a foriegn
 *	C++ namespace. 
 *
 *	Version 1.1.2 (2010-11-05)
 *		Updated documentation in prepartion of being released on my website, including
 *		attaching a license.
 *
 *	Version 1.2.0 (2010-12-07)
 *		Added selectors to the subscription method.
 */

#ifndef _MEE_STOMP_STOMPCLIENT_H_
#define _MEE_STOMP_STOMPCLIENT_H_

#include<map>
#include<iostream>
#include<sstream>

namespace mee { namespace stomp {

/**
 * Acknowledgement modes for queue/topic subscriptions.
 *
 * @since 1.0.0
 * @version 1.0.0
 * @author Mark Eschbach <meschbach@gmail.com>
 */
typedef enum Acknowledge_e {
	/**
	 * The <code>Client</code> value states that a client
	 * will acknowledge a message when at the client's
	 * descretion 
	 */
		Client, 
	/**
	 */
		Auto
} AcknowledgeMode;

/**
 * The <code>StompFrame</code> is a kind of <code>std::exception</code> encapsulating
 * STOMP related issues, including socket connectivity.  <code>StompException</code>
 * will provide a context specific exception, ie: Unexpected end of frame encountered.
 * 
 * @since 1.0.0
 * @version 1.0.0
 * @author Mark Eschbach <meschbach@gmail.com>
 */
class StompException : public std::exception {
	/**
	 * The <code>message</code> constains the message to be returned 
	 * when the object is sent the <code>what()</code> method.
	 */
		std::string message;
public:
	/** 
	 * Constructs a new <code>StompException</code> with the given
	 * message.
	 *
	 * @param msg is the message describing the problem encountered
	 */
		StompException(const std::string& msg);
	/**
	 * Nothing special about this destructor
	 */
		virtual ~StompException() throw ();
	/**
	 * Overrides <code>std::exception</code> to provide the
	 * <code>message</code> field in C string format.
	 */
		virtual const char* what() const throw ();
};

/**
 * A <code>StompFrame</code> is the basic unit of communication of the STOMP protocol.
 * Frames are passed to and from the server.  Frames resemble HTTP requests and replies,
 * each consiting of an operation line, properties, and a message body (entity).
 *
 * Please see http://stomp.codehaus.org/Protocol for more details about the frames
 *
 * @since 0.0.1
 * @version 1.0.0
 * @author Mark Eschbach <meschbach@gmail.com>
 */
typedef struct StompFrame_t {
	/**
	 * The <code>operation</code> is the protocol operation being sent.
	 */
	std::string operation;
	/**
	 * The <code>properties</code> is a set of key-value pairs contained within the
	 * frame.  On similar protocols, these are also called <pre>headers</pre>.
	 */
	std::map<std::string, std::string> properties;
	/**
	 * The <code>message</code> is the message body.
	 */
	std::stringstream message;
} StompFrame;

/**
 * The <code>StompClient</code> is a facade around a set of components to
 * communciate with a STOMP broker.  Currently this component contains
 * both the frame serialization components and high level message method.
 *
 * Revision History:
 *	Revision 0.0.1
 *		Initial draft of the working StompClient
 *	Revision 1.0.0
 *		Initial production and release version
 *	Revision 1.1.0
 *		Migrated to using a separate I/O streams,
 *	adding a new constructor and refactoring old code
 * 	to supprot the change.
 *
 * @since 0.0.1
 * @author Mark Eschbach
 * @version 1.1.0 
 */
class StompClient {
	/**
	 * Input stream to recieve data from the remote STOMP broker
	 *
	 * @since 1.1.0
	 */
	std::istream& inputSource;
	/**
	 * Output stream to send data to the remote STOMP broker
	 *
	 * @since 1.1.0
	 */
	std::ostream& outputSink;
	/**
	 * Session identifier
	 *
	 * @since 0.0.1
	 */
	std::string sessionID;

public:
	/**
	 * Constructs a new StompClient with the given I/O streams.
	 *
	 * @param anInput is the stream to recieve data from the remote broker
	 * @param anOutput is the stream to send data to the STOMP broker
	 * @since 1.1.0
	 */
		StompClient( std::istream& anInput, std::ostream& anOutput );
	/**
	 * Constructs a new StompClient with the given socket for communication 
	 *
	 * @param socket is the I/O stream fro communicating with the STOMP broker
 	 * @since 0.0.1
	 */
		StompClient(std::iostream &socket);
	/**
	 * This descrutor is empty and will not close either the input or output streams.
	 *
	 * @since 0.0.1
	 */
		virtual ~StompClient();

	/**
	 * @since 0.0.1
	 * @deprecated Use the getInput() implementation instead, will be removed in 2.0 release
	 * @return the input stream used by the STOMP client
	 */
		std::istream& input();

	/**
	 * @since 1.1.0
	 * @return the input stream used to recieve data from the STOMP client
	 */
		std::istream& getInput();
	/**
	 * @since 1.1.0
	 * @return the output stream used to send data to the remote STOMP client
	 */
		std::ostream& getOutput();

	/**
	 * @since 1.1.0
	 * @return the session ID given by the STOMP broker upon connection
	 */
		const std::string& getSessionID() const;
	/**
	 * Sends the given frame to the remote broker.
	 *
	 * @since 0.0.1 
	 * @throws StompException if there is an error reading in the entire frame
	 */
		void sendFrame(StompFrame& frame) throw (StompException);

	/**
	 * Parses an entire STOMP frame from the input stream, placing
	 * all values into the given frame.
	 *
	 * @param frame is the frame to place all values into
	 * @throws StompException if a frame couldn't be properly read
	 * @since 0.0.1
	 */
		void receiveFrame(StompFrame& frame);

	/**
	 * Parses just the properties of a STOMP frame
	 * from the input stream, in the <pre>key: value</pre> format.
	 *
	 * @param props is the map to place the values into
	 * @throws StompException if the properties couldn't be properly parsed
	 * @since 0.0.1
	 */
		void receiveProperties(std::map<std::string, std::string> &props);
	/**
	 * Parses a single property from the input stream into a <pre>key: value</pre>
	 * pair.
	 *
	 * @since 0.0.1
	 * @throws StompException if the pair couldn't be parsed
	 * @returns a single property parsed from the input stream
	 */
		std::pair<std::string, std::string> receiveProperty();

	/**
	 * A utility method to perform a full connection handshake
	 * for the STOMP protocol.  If <code>sendLogin</code> is false then
	 * the authentication information passed in <code>user</code> and <code>pass</code>
	 * wil not be sent to the remote broker.
	 *
	 * @since 0.0.1
	 * @throws StompException if there is a problem communicating using the STOMP protocol
	 */
		void connect(const bool sendLogin = false, const std::string& user = "", const std::string& pass = "");

	/**
	 * Sends a connect message to the STOMP broker. This method doesn't not
	 * listen for the response from the remote STOMP broker.  If <code>sendLogin</code>
	 * is false, then no authentication information is sent to the broker.
	 *
	 * @param sendLogin if true then the user alias and password is sent to the broker
	 * @param user is the user alias to send to the remote broker
	 * @param pass is the password to send to the remote broker
	 *
	 * @since 0.0.1
	 */
		void sendConnect(const bool sendLogin = false, const std::string& user = "", const std::string& pass = "");
	/**
	 * Processes the server's response to connection request.  This
	 * will extract the session identifier from the message.
	 *
	 * @param frame is the StompFrame containing the server's reply
	 * @throws StompException if the frame is not a valid connected frame
	 */
		void processConnect(StompFrame& frame);

	/**
	 * Sends a disconnect message to the remote server.  This method will not
	 * close either the input or output stream.
	 */
		void disconnect();

	/**
	 * Sends a message to a specific destination using this connection.
	 *
	 * @param destination is where the message is to be sent
	 * @param message is the message to be sent
	 * @throw StompExceptin if a problem occurs sending or recieving frames
	 */
	void
	sendMessage(const std::string &destination, const std::string &message) throw (StompException);

	/**
	 * Send a message to a specific destiantion with the specified properties
	 *
	 * @since 1.0.0
	 * @param dest is the destination to send the message to
	 * @param msg is the message body or entity to send
	 * @param properties is the properties to be set on the message
	 *
	 * @throws StompException if there was an issue sending or recieving responses
	 */
	void
	sendMessage(const std::string& dest, 
		const std::string& msg,
		const std::map< std::string, std::string >& properties ) throw (StompException);

	void
	sendMessage(
		const std::string& dest,
		const StompFrame& frame
		) throw ( StompException );
	/**
	 * Subscribe the current connection to the specified queue
	 * so that message may be received.
	 *
	 * @param destination is the destination to subscribe too
	 * @param mode is the acknowledgment mode to use with the protocol
	 */
	void subscribe(const std::string& destination, 
		const AcknowledgeMode mode = Auto,
		const std::string& selector = "");

	/**
	 * Unsubscribes the client from the specified destination
	 */
	void unsubscribe(const std::string& destination);

	/**
	 * Sends an acknowledgment to the broker for the specific message.
	 * The message identifier is set on incoming message frames
	 * in the property 'message-id'.
	 *
	 * @param messageID is the ID of the message to be acknowledged
	 */
	void acknowledge(const std::string& messageID);
};

} /* namespace stomp */ } /* namespace mee */

#endif /* _MEE_STOMP_STOMPCLIENT_H_ */

