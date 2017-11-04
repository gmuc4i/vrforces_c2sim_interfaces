/// Functions for handling HTTP requests

#include "stdafx.h"
#include "WSS_HTTP.h"
#include "WSS_Driver.h"
#include "WSS_Constants.h"

#define GET             0
#define POST            1

const char *okMessage = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" WSS_NL "<result>OK</result>";

const char *generalErrorMessage = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" WSS_NL "<result>ERROR</result>";

const char *messageStart = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" WSS_NL "<result>";

const char *messageEnd = "</result>";

const char * badURLMessage = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" WSS_NL "<result>Invalid URL</result>";

const char * msgSelectorsMessage = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" WSS_NL
"<collection>" WSS_NL
"<msg_selector><search>(/msdl:MilitaryScenario) |  (/msdl:MSDL_AddUnit) | (/msdl:MSDL_AddEquipment)</search><name>msdl</name></msg_selector>"
"<msg_selector><search>(/bml:OrderPushIBML) | (/OrderPushIBML)</search><name>IBML_Order</name></msg_selector>"
"<msg_selector><search>(/bml:OrderPush)| (/OrderPush)</search><name>CBML_Order</name></msg_selector>"
"<msg_selector><search>(/bml:BMLReport)|(/BMLReport)</search><name>IBML_Report</name></msg_selector>"
"<msg_selector><search>(/bml:BMLREPORTS)|(BMLREPORTS)</search><name>CBML_Report</name></msg_selector>"
"<msg_selector><search>//*</search><name>debug</ame></msg_selector></collection>";
const char * submitterID = "DSC";
const char * domain = "WISE";

IWISEDriverSink*	mysink;
CWISEDriver*		thisObject;
long activeHTTP = 0;
long activeProcess = 0;
long startIdle;


WISE_HANDLE hDB;

bool validate;

int answer_to_connection(void *cls,
	struct MHD_Connection *connection,
	const char *url,
	const char *method,
	const char *version,
	const char *upload_data,
	size_t *upload_data_size,
	void **con_cls);

struct connection_info
{
	int connectiontype;
	char* uploadedTransaction;
	unsigned int uploadedTransactionLength;
	std::string submitterID;
	std::string domain;
	std::string forwarders;
	connection_info()
		: connectiontype(0),
		uploadedTransaction(NULL),
		uploadedTransactionLength(0)
	{
	}
	~connection_info()
	{
		if (uploadedTransaction != NULL)
		{
			delete uploadedTransaction;
			uploadedTransaction = NULL;
		}
	}
};

struct MHD_Daemon *m_daemon;

/********************
* start_http 	    *
*********************/

int start_http(long port, bool doValidation, IWISEDriverSink * s, WISE_HANDLE h, CWISEDriver* to)
{
	// Save Sink pointer, database handle, and pointer to this 
	// CWISEDriver object to pass on to message processing routines
	mysink = s;
	hDB = h;
	thisObject = to;
	validate = doValidation;
	m_daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, (uint16_t)port, NULL, NULL,
		&answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL, MHD_OPTION_END);

	if (NULL == m_daemon)
		return 1;
	return 0;

}	// start_http

void stop_http()
{
	if (m_daemon != NULL)
	{
		MHD_stop_daemon(m_daemon);
		m_daemon = NULL;
	}
}

/*************************
* answer_to_connection	 *
**************************/
// An http connection has been made
//  On the first call we can get headers and parameters
//	On the second and subsequent calls the data in the http payload can be accessed
//  After the last data has been received we will get one more call with upload_data_size = 0
//	We keep connection specific data in an instance of struct connection_info pointed to by *con_cls.

int answer_to_connection(void *cls,
	struct MHD_Connection *connection,
	const char *url,
	const char *method,
	const char *version,
	const char *upload_data,
	size_t *upload_data_size,
	void **con_cls)
{
	struct connection_info *con_info;
	con_info = (struct connection_info*)*con_cls;

	// attempt to preserve operation in case we have errors
	try
	{
		// First call for this connection?
		if (*con_cls == NULL)
		{

			// Monitor idle time between transactions
			if (startIdle != 0) {
				thisObject->NotifyDebugMessage(L"\tIDLE\t" + StringHelper::ConvertLongToWString(GetTickCount() - startIdle), 1);
				startIdle = 0;
			}

			++activeHTTP;
			thisObject->NotifyDebugMessage(L"\tHTTP\t" + StringHelper::ConvertLongToWString(activeHTTP), 1);

			// Only headers are available on this call

			// Allocate storage for connection_info 
			con_info = new connection_info(); // (connection_info*) malloc(sizeof(struct connection_info));

											  // Save address for additional calls
			*con_cls = con_info;
			con_info->connectiontype = POST;

			// Extract the submitterID, forwarders and domain from the URL.
			con_info->submitterID = parse_url(url, "submitterID");
			if (con_info->submitterID == "")con_info->submitterID = "unknown";
			con_info->forwarders = parse_url(url, "forwarders");
			con_info->domain = parse_url(url, "domain");
			MHD_get_connection_values(connection, MHD_HEADER_KIND, &print_out_key, NULL);
			return MHD_YES;
		}

		// Not the first call - Data should now be available
		// If upload_data_size is zero then we have gotten all the data in the transaction
		if (*upload_data_size == 0)
		{
			--activeHTTP;

			// self-protection
			if (con_info->uploadedTransaction == NULL)
			{
				thisObject->NotifyInfoMessage(L"submitter:" + toWString(con_info->submitterID) + L" error - empty transaction");
			}

			// Is this a request for the Message Selectors?
			if (!strcmp(url, "/SBMLServerREST-war/SBML/msg_selectors")) {
				// Return the list of Message Selectors
				send_page(connection, msgSelectorsMessage);
				return MHD_YES;
			}

			// Verify that a valid URL has been used
			string u(url);
			size_t i = u.find("/SBMLServerREST-war/SBML/bml");
			if (i == string::npos) {
				send_page(connection, badURLMessage);
				return MHD_YES;
			}

			// if any of forwarders is ourself, stop now
			// forwarders has format ID1:ID2:...IDn;
			string compareForwarders = string(con_info->forwarders);
			int semicolonPosition = compareForwarders.find(";", 0);
			if (semicolonPosition < 0 || (unsigned)semicolonPosition > compareForwarders.length())
			{
				thisObject->NotifyInfoMessage(L"bad forwarders parameter received:" + toWString(compareForwarders));
				return MHD_YES;
			}

			// scan through forwarders
			char* res = NULL;
			int startPosition = 0;
			int colonPosition = compareForwarders.find(":", startPosition);
			string compareLocalAddress = string(localHostAddress);
			string testAddress;
			char* loopErrorMessage = "this server's ID found in forwarders - input dropped";
			while (colonPosition > 0)
			{
				// extract next address and moves indices forward
				testAddress = compareForwarders.substr(startPosition, colonPosition - startPosition);
				startPosition = colonPosition + 1;
				colonPosition = compareForwarders.find(":", startPosition);
				if (testAddress == compareLocalAddress)
				{
					thisObject->NotifyInfoMessage(toWString(loopErrorMessage));
					res = loopErrorMessage;
				}
			}

			// check last address before semicolon
			testAddress = compareForwarders.substr(startPosition, semicolonPosition - startPosition);
			if (testAddress == compareLocalAddress)
			{
				thisObject->NotifyInfoMessage(toWString(loopErrorMessage));
				res = loopErrorMessage;
			}

			// add our address to forwarders
			if (semicolonPosition == 0)
				con_info->forwarders = compareLocalAddress + ';';
			else
				con_info->forwarders = compareForwarders.substr(0, semicolonPosition) + ':' + localHostAddress + ';';

			// if no loop found, process the bml we received
			if (res == NULL)
			{
				++activeProcess;
				thisObject->NotifyDebugMessage(L"\tPROCESS\t" + StringHelper::ConvertLongToWString(activeProcess), 1);
				res = processBML(con_info->uploadedTransaction,
					con_info->uploadedTransactionLength,
					con_info->submitterID,
					con_info->domain,
					con_info->forwarders,
					validate,
					mysink,
					hDB,
					thisObject,
					connection);
				--activeProcess;
			}

			// Was there an error?
			if ((activeHTTP == 0) && (activeProcess == 0))
				startIdle = GetTickCount();
			if (res == NULL) {
				// No, send an "OK" response
				send_page(connection, okMessage);
			}
			else
			{
				// Yes, send "ERROR" response
				int errorSize = strlen(messageStart) + strlen(res) + strlen(messageEnd);
				char* sendError = (char*)malloc(errorSize + 1);
				strncpy_s(sendError, errorSize, messageStart, errorSize);
				strncat(sendError, res, errorSize);
				strncat(sendError, messageEnd, errorSize);
				send_page(connection, sendError);
				free(sendError);
				free(res);
			}

			return MHD_YES;
		}

		// Get the saved connection_info passed to us on each call
		con_info = (connection_info*)(*con_cls);

		// Allocate memory and copy the uploaded data being passed on this call.

		if (con_info->uploadedTransaction == NULL)
			// First data segment
		{
			// Allocate space and copy segment just uploaded and make it a c string
			con_info->uploadedTransaction = new char[*upload_data_size + 1]; //(char *)malloc(*upload_data_size + 1);
			strncpy(con_info->uploadedTransaction, upload_data, *upload_data_size);
			con_info->uploadedTransaction[*upload_data_size] = '\0'; // *(con_info->uploadedTransaction + *upload_data_size) = '\0';
			con_info->uploadedTransactionLength = *upload_data_size;
		}
		else
			// Not first data segment
		{
			// Allocate space for existing string + segment just uploaded
			char* newString = new char[con_info->uploadedTransactionLength + *upload_data_size + 1]; //(char*) malloc(uploadedSize + *upload_data_size + 2); //MXGR: why +2 instead of +1?

																									 // Copy existing string followed by uploaded string to newString
			strcpy(newString, con_info->uploadedTransaction);
			strncat(newString, upload_data, *upload_data_size);

			// Free space used by existing string
			delete con_info->uploadedTransaction;

			con_info->uploadedTransaction = newString;
			con_info->uploadedTransactionLength += *upload_data_size;
		}

		// Tell HTTP that we consumed all the data and return YES indicated that we want more.
		*upload_data_size = 0;
		return MHD_YES;
	}
	catch (exception& e)
	{
		char errorMessage[200];
		strncpy_s(errorMessage, 200, e.what(), 199);
		errorMessage[199] = '\0';
		send_page(connection, errorMessage);
		return MHD_YES;
	}
}// end answer_to_connection()


 /********************
 * print_out_key		*
 *********************/
 // Used when a call to MHD_Get_Connection_Values is made

int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
// Display key value pair
{
	printf("%d  %s: %s\n", kind, key, value);
	return MHD_YES;
}	// print_out_key

	/***********************************************
	* send_page (MHD_Connection*, const char*)     *
	************************************************/
	// Return a constant C-style string (usually, XML) to the requestor

int send_page(struct MHD_Connection* connection, const char* page)
{
	// Send page on connection
	struct MHD_Response *response;
	int ret;

	// Get the number of bytes to send (note that we DON'T
	// want to add one for the trailing null here, as
	// MHD will be sending raw bytes)
	size_t nbytes = strlen(page) * sizeof(const char);

	// Even though 'page' is const, we still have to tell
	// MHD to make a copy, since we're going to be queueing the response.
	MHD_ResponseMemoryMode mmode = MHD_RESPMEM_MUST_COPY;

	response = MHD_create_response_from_buffer(nbytes, (void *)page, mmode);
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;
}	// send_page (MHD_Connection *, const char*)

	/********************
	* parse_url			*
	*********************/
	// Parse the URL and extract the parameters
std::string parse_url(const char* url, const char* target)
{
	//	char* result;

	if (!(strcmp(target, "submitterID")))
	{
		string s(url);
		string submitter;
		int i = s.find("ID=");
		int j = s.find(";", i);
		if (i < 0)
			submitter = "";
		else
			submitter = s.substr(i + 3, j - (i + 3));
		return submitter;
		//char* res = (char*)submitter.c_str();
		//int l = strlen(res);
		//result = (char *)malloc(submitter.length()+1);
		//strncpy(result, submitter.c_str(), (submitter.length()+1));
	}
	else if (!(strcmp(target, "domain")))
	{
		string s(url);
		string domain;
		int i = s.find("domain=");
		int j = s.find(";", i);
		if (i < 0)
			domain = "";
		else
			domain = s.substr(i + 7, j - (i + 7));
		return domain;
		//char* res = (char*)domain.c_str();
		//int l = strlen(res);
		//result = (char *)malloc(domain.length()+1);
		//strncpy(result, domain.c_str(), (domain.length()+1));
	}
	else if (!(strcmp(target, "forwarders")))
	{
		string s(url);
		string forwarders;
		int i = s.find("forwarders=");
		int j = s.find(";", 0);
		if (i < 0)
			forwarders = ';';
		else
			forwarders = s.substr(i + 11, j - i - 11) + ';';
		return forwarders;
		//char* res = (char*)firstforwarder.c_str();
		//int l = strlen(res);
		//result = (char *)malloc(firstforwarder.length()+1);
		//strncpy(result, firstforwarder.c_str(), (firstforwarder.length()+1));
	}
	/*else
	{
	result = (char *)malloc(strlen("WISE")+1);
	strncpy(result, "WISE", strlen("WISE")+1);
	}
	*/
	return string("WISE");

}	// parse_url()

	/****************************
	* request_completed			*
	****************************/
	// All data has been processed and the connection has been broken
	//   Clean up and return dynamic memory

int request_completed(void *cls, struct MHD_Connection *connection,
	void **con_cls, enum MHD_RequestTerminationCode toe)
{
	struct connection_info *con_info = (struct connection_info*)*con_cls;
	// Free the saved fields in the connection_info structure for this connection
	try
	{
		if (con_info != NULL)
		{
			delete con_info;
			con_info = NULL;
		}
	}
	catch (exception&)
	{
		thisObject->NotifyInfoMessage(L"error in delete of con_info");
	}

	*con_cls = NULL;
	return MHD_YES;
}	// request_completed

