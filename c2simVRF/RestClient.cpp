#include "RestClient.h"
#include "cpprest/http_client.h"
#include "cpprest/filestream.h"
#include <sstream>
#include <codecvt>

using namespace web;
using namespace web::http;
using namespace web::http::client;

#include <iostream>
using namespace std;

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
	domain = "ibml09";
	submitter = "jmp";
	firstForwarder = "";
}
RestClient::RestClient(std::string hostRef) 
{ 
	host = hostRef;
	port = "8080";
	path = "BMLServer/bml";
	domain = "ibml09";
	submitter = "jmp";
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

/*
// Upload a file to an HTTP server.
pplx::task<void> UploadFileToHttpServerAsync(uri url, std::wstring xmlPath)
{
	using concurrency::streams::file_stream;
	using concurrency::streams::basic_istream;
	using concurrency::streams::istream;
	using concurrency::streams::container_buffer;
	using concurrency::streams::wstringstream;

	// Open stream to file. 
	return file_stream<unsigned char>::open_istream(xmlPath).then(
		[&](pplx::task<basic_istream<unsigned char>> previousTask)
	{
		try
		{
			auto fileStream = previousTask.get();

			// Make HTTP request with the file stream as the body.
			http_client client(url);
			http_request request(methods::POST);
			request.headers().add(L"Content-Type", L"application/xml");
			request.headers().add(L"Accept", L"application/xml");

			request.set_body(L"dfghjkjhgfdsdfgyuj");

			return client.request(request).then([fileStream](pplx::task<http_response> rep)
			{
				fileStream.close();
				std::wostringstream ss;
				try
				{
					auto response = rep.get();
					ss << L"Server returned returned status code " << response.status_code() << L"." << std::endl;
					std::string responseBody = utility::conversions::to_utf8string(response.to_string());

					if (responseBody.find("<status>Error</status>") == 0) {
						//throw RestException::exception("Error recieved from server");
						std::cout << "Error recieved from server\n\n";
						std::cout << responseBody;
					}

					else {
						std::cout << responseBody;
						//return responseBody;
					}
				}
				catch (const http_exception& e)
				{
					ss << e.what() << std::endl;
				}
				std::wcout << ss.str();
			});
		}
		catch (const std::system_error& e)
		{
			std::wcout << "file not found";
			std::wostringstream ss;
			ss << e.what() << std::endl;
			std::wcout << ss.str();
			// Return an empty task. 
			return pplx::task_from_result();
		}
	});


}

// Upload a file to an HTTP server.
pplx::task<void> UploadFileToHttpServer(std::string url, std::string xmlPath)
{
	using concurrency::streams::file_stream;
	using concurrency::streams::basic_istream;
	using concurrency::streams::istream;
	using concurrency::streams::container_buffer;
	using concurrency::streams::wstringstream;

	// Open stream to file. 
	return file_stream<unsigned char>::open_istream(xmlPath).then([&](pplx::task<basic_istream<unsigned char>> previousTask)
	{
		try
		{
			auto fileStream = previousTask.get();


			// Make HTTP request with the file stream as the body.
			http_client client(url);

			http_request request(methods::POST);
			request.headers().add(L"Content-Type", L"application/xml");
			request.headers().add(L"Accept", L"application/xml");

			request.set_body(L"dfghjkjhgfdsdfgyuj");

			return client.request(request).then([fileStream](pplx::task<http_response> rep)
			{
				fileStream.close();
				std::wostringstream ss;
				try
				{
					auto response = rep.get();
					ss << L"Server returned returned status code " << response.status_code() << L"." << std::endl;
					std::string responseBody = utility::conversions::to_utf8string(response.to_string());

					if (responseBody.find("<status>Error</status>") == 0) {
						//throw RestException::exception("Error recieved from server");
						std::cout << "Error recieved from server\n\n";
						std::cout << responseBody;
					}

					else {
						std::cout << responseBody;
						//return responseBody;
					}
				}
				catch (const http_exception& e)
				{
					ss << e.what() << std::endl;
				}
				std::wcout << ss.str();
			});
		}
		catch (const std::system_error& e)
		{
			std::wcout << "file not found";
			std::wostringstream ss;
			ss << e.what() << std::endl;
			std::wcout << ss.str();
			// Return an empty task. 
			return pplx::task_from_result();
		}
	});


}
*/


/********************/
/*  bmlRequest()    */
/********************/
/**
* Submit a bml transaction to the host specified
* @param xmlTransaction - An XML string containing the bml
* @return - The response returned by the host BML server
*/

std::string RestClient::bmlRequest(std::string xmlTransaction)
{
	std::string url;
	if (submitter.compare("") == 0)
		//throw RestException::exception();
		return "Error - Submitter not specified.\n";
	if (domain.compare("") == 0)
		//setDomain("NO_DOMAIN");
		return "Error - Domain not specified.\n";

	url = "http://" + host
		+ ":" + port
		+ "/" + path
		+ "?submitterID=" + submitter
		+ "&domain=" + domain
		;
	if (firstForwarder.compare("") != 0)
		url += "&forwarders=" + firstForwarder;
	utility::string_t u = utility::conversions::to_string_t(url);

	try
	{
		// build and submit the HTTP transaction
		http_client client(u);
		http_request request(methods::POST);
		request.headers().add(L"Content-Type", L"application/xml");
		request.headers().add(L"Accept", L"application/xml");

		request.set_body(xmlTransaction);

		std::string responseBody = "";
		client.request(request).then([&](pplx::task<http_response> res)
		{
			auto response = res.get();
			std::cout << "Server returned status code " << response.status_code() << ".";

			responseBody = utility::conversions::to_utf8string(response.to_string());

			if (responseBody.find("<status>Error</status>") == 0) {
				//throw RestException::exception("Error received from server");
				std::cout << "Error received from server\n\n";
				//std::cout << responseBody;
			}

			else {
				//std::cout << responseBody;
			}

		}).wait();

		return responseBody;
	}
	catch (const web::http::http_exception& e) {
		std::cout << "HTTP Exception: " << e.what();
		return "HTTP Exception " + std::string(e.what());
	}
	catch (const std::exception& e) {
		std::cout << "I/O Exception: " << e.what();
		return "IO Exception" + std::string(e.what());
	}

}


RestClient::~RestClient()
{
}
