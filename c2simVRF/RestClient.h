
#include <iostream>
#include <sstream>

#pragma once


class RestException : public std::exception {
	/**
	* The <code>message</code> constains the message to be returned
	* when the object is sent the <code>what()</code> method.
	*/
	std::string message;
public:
	/**
	* new <code>RestException</code> with the given message
	*
	* @param msg is the message describing the problem encountered
	*/
	RestException(const std::string& msg);

	virtual ~RestException() throw ();

	const char* RestException::what() const throw ();
};



class RestClient
{
std::string host;
std::string port;
std::string path;
std::string submitter;
std::string domain;
std::string firstForwarder;
public:
	RestClient();
	RestClient(std::string host);
	~RestClient();
	std::string bmlRequest(std::string xmlTransaction);
	void setPort(std::string port);
	void setHost(std::string host);
	void setPath(std::string path);
	void setDomain(std::string domain);
	void setSubmitter(std::string submitter);
	void setFirstForwarder(std::string ff);

	std::string getPort();
	std::string getPath();
	std::string getHost();
	std::string getDomain();
	std::string getSubmitter();
	std::string getFirstForwarder();



};

