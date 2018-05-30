#include "RestClient.h"

#define BUFFERLENGTH 10000

// Boost
#include <boost/asio.hpp>

using namespace std;

boost::asio::ip::tcp::iostream restStream;

/**
*  RestClient
* BML Server Web Services REST Client<p>
* This client does the following:<p>
*      Open a connection with the server on specified port (Default is 8080)<p>
*      Build an HTTP POST transaction from parameters and BML XML document<p>
*      Submit the transaction<p>
*      Read the result<p>
*      Disconnect from the server<p>
*      Return the result received from the server to the caller<p>
*/
/****************************************/
/* RestClient() - Constructor    */
/****************************************/
RestClient::RestClient()
{
	//setting defaults
	host = "10.2.10.30";    
	port = "8080";   
	path = "BMLServer/bml";
	domain = "BML";
	submitter = "vrforces";
	firstForwarder = "";
}
RestClient::RestClient(std::string hostRef) 
{ 
	host = hostRef;
	port = "8080";
	path = "BMLServer/bml";
	domain = "BML";
	submitter = "vrforces";
	firstForwarder = "";
}

/****************************/
/*  getters and setters     */
/****************************/


/**
* Set the port number (as a string)
* @param port
*/
void RestClient::setPort(std::string p)
{
	port = p;
}


/**
* Set the host name or address
* @param host
*/
void RestClient::setHost(std::string h)
{
	host = h;
}

/**
* Set the port number (as a string)
* @param path
*/
void RestClient::setPath(std::string pt)
{
	path = pt;
}


/**
* Set the domain property.  Used to discriminate between BML dialects
* @param domain
*/
void RestClient::setDomain(std::string d)
{
	domain = d;
}

/**
* Set the Requestor property indicating the identity of the client
* @param requestor
*/
void RestClient::setSubmitter(std::string s)
{
	submitter = s;
}
/**
* Set the FirstForwarder property indicating first server to
* forward the XML
*/
void RestClient::setFirstForwarder(std::string f)
{
	firstForwarder = f;
}

/**
* Get the current setting of the port property
* @return
*/
std::string RestClient::getPort()
{
	return port;
}

/**
* Get the current setting of the path property
* @return
*/
std::string RestClient::getPath()
{
	return path;
}

/**
*
* @return the current setting of the domain property
*/
std::string RestClient::getDomain()
{
	return domain;
}

/**
* Get current setting of host property
* @return
*/
std::string RestClient::getHost()
{
	return host;
}

/**
* Get current setting of Requestor property
* @return
*/
std::string RestClient::getSubmitter()
{
	return submitter;


/**
* Get the FirstForwarder property indicating first server to
* forward the XML
*/
}std::string RestClient::getFirstForwarder()
{
	return firstForwarder;
}


/********************/
/*  bmlRequest()    */
/********************/
/**
* Submit a bml transaction to the host specified
* @param xmlTransaction - An XML string containing the bml
* @return - The response returned by the host BML server
* build of http without library by JMP 16May18
*/

std::string RestClient::bmlRequest(std::string xmlTransaction)
{
	// build parameter string for http header
	if (submitter.compare("") == 0) {
		std::cout << "error in RestClient.bmlRequest - Submitter not specified.\n";
		return "";
	}
	if (domain.compare("") == 0) {
		std::cout << "error in RestClient.bmlRequest - Domain not specified.\n";
		return "";
	}
	std::string parameters;
	parameters += "?submitterID=" + submitter;
	parameters += "&protocol=" + domain;
	if (firstForwarder.compare("") != 0)
		parameters += "&forwarders=" + firstForwarder;

	// exchange C2SIM HTTP transaction with server
	try
	{	
		// open a boost connection
		try {
			// Establish the connection and associate it with StompClient
			restStream.connect(host.c_str(), port.c_str());
			if (!restStream) {
				std::cerr << "error opening REST connection to " <<
					host << ":" << port << std::endl;
				return "";
			}
		}
		catch (const std::exception& e) {
			std::cerr << "exception " << e.what()
				<< " opening REST connection to " <<
				host << ":" << port << std::endl;
			return "";
		}

		// assemble and POST http request header
		std::string httpRequest;
		httpRequest += "POST /" + path;
		httpRequest += parameters;
		httpRequest += " HTTP/1.1\r\n";
		httpRequest += "Content-Type:application/xml\r\n";
		httpRequest += "Accept:application/xml\r\n";
		httpRequest += "Host:" + host + ":8080\r\n";
		httpRequest += "Connection:.keep-alive\r\n";
		httpRequest += "Content-Length:";
		std::stringstream transBuf;
		transBuf << xmlTransaction;
		std::stringstream lengthBuf;
		lengthBuf << transBuf.str().length();
		httpRequest += lengthBuf.str() + "\r\n";
		httpRequest += "\r\n";
		httpRequest += xmlTransaction + "\r\n";

		// send the header and XML document
		restStream << httpRequest;
		restStream.flush();

		// receive http response and check that response is OK.
		std::string httpVersion;
		restStream >> httpVersion;
		unsigned int statusCode;
		restStream >> statusCode;
		std::string statusMessage;
		std::getline(restStream, statusMessage);
		if (!restStream || httpVersion.substr(0, 5) != "HTTP/") {
			std::cerr << "invalid REST response " << httpVersion << "\n";
			return "";
		}
		if (statusCode != 200)
		{
			std::cerr << "REST response returned with status code " << statusCode << "\n";
			return "";
		}

		// read past the response headers, which are terminated by a blank line
		// in the process capture Content-Length
		string responseLine;
		int contentLength = BUFFERLENGTH;
		while (!restStream.eof() && !restStream.bad()) {
			std::getline(restStream, responseLine);
			if (responseLine.substr(0, 15) == "Content-Length:") {
				contentLength = std::stoi(responseLine.substr(15));
			}
			if (responseLine.length() <= 1) break;
		}
		if (restStream.eof() || restStream.bad()) {
			std::cerr << "error reading HTTP response header in REST\n";
			return "";
		}

		// read the response
		char* httpResponse = new char[BUFFERLENGTH];
		httpResponse[0] = '\0';
		restStream.read(httpResponse, contentLength);
		if (restStream.bad()) {
			std::cerr << "error reading HTTP response in REST\n";
			return "";
		}

		// NOTE: the following depends on correct value of Content-Length in server output
		// null-terminate part of response ending </result>
		//httpResponse[strstr(httpResponse, "</result>") - httpResponse + strlen("</result>")] = '\0';debugx
		httpResponse[contentLength] = '\0';
		std::string returnResponse(httpResponse, contentLength);
		free (httpResponse);
		return returnResponse;
	}
	catch (const std::exception& e) {
		std::cout << "exception in RestClient: " << e.what();
		return "exception in RestClient: " + std::string(e.what());
	}
}// end RestClient::bmlRequest


RestClient::~RestClient()
{
}
