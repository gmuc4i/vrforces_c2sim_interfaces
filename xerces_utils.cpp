
#include "xerces_utils.h"
//#include "stdafx.h" debugx

/********************
*	findNode		*
********************/
// search for xpath and dump into result
// result will be created in this function - it is up to the caller to delete result when done using it!!!
// 
// NOTE: The documentation for ->evaluate indicates that one could re-use the DOMXPathResult-object multiple times if 
//		 passed as the last argument to evaluate(). However doing so results in memory leaks. Therefore we delete any created
//		 result-objekt passed to this function.
// Also note result may be NULL once returning from this function.

// Run xpath based on starting Node
// Dump result into result
// Return number of results
//
int findNode(
	DOMDocument *domDoc, 
	const DOMElement* baseNode, 
	const XMLCh* xpath, 
	DOMXPathResult*& result)
{
	XMLSize_t nLength;
	if( result != NULL )
	{ 
		delete result;
		result = NULL;
	} 
	try
	{
		wcout <<"Domdoc node:" << domDoc->getNodeName()<< "|\n\n"; //debugx
		wcout << "xpath:"<< xpath <<"|\n";
		wcout << "Base node:" << baseNode->getNodeName() << "|\n\n";

		//result = domDoc->evaluate(xpath.c_str(), baseNode, NULL, 
		//DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE, NULL);
		result = domDoc->evaluate(xpath, baseNode, NULL,
			DOMXPathResult::SNAPSHOT_RESULT_TYPE, NULL);
		wcout << "result:" << result->getStringValue();
		cout << "\n\nDocument is parsed!!\n\n"; //debugx
	}
	catch (const DOMException& toCatch)
	{
		// return code indicating exception
		char* message = XMLString::transcode(toCatch.getMessage());
		cout << "Exception in findNode :\n"
			<< message << "\n";
		XMLString::release(&message);
		return -1;
	}

	// return number of results
	nLength = result->getSnapshotLength();
	return (int)nLength;
}

/********************
 *  parseString
 ********************/
// Parses XML stored in a string.
//      Returns a pointer to the parser for that document.
//      Requires that Xerces has already been initialized.
//      Note that, unless you redefine the constant WSS_DO_NAMESPACES,
//      this function WILL tell Xerces to care about namespaces (which
//      is different from Xerces' default behavior.)

/* debugx
XercesDOMParser *parseString(string &str) {
    // We have to save to a file to get XPath to work
    wstring tempPath;
   
	if (!IOHelper::saveString(str.c_str(), tempPath)) {
        return NULL;
    }
    XercesDOMParser *parser = new XercesDOMParser();
    parser->parse(tempPath.c_str());
    return parser;
}
*/

/********************
 * serializeDocument
 ********************/
// Serializes an XML document to a string, using pretty print.
string serializeDocument(DOMDocument *doc) {
	// Get a DOM implementation with load and save ("LS") features
	DOMImplementation *impl = 
        DOMImplementationRegistry::getDOMImplementation(L"LS");

	// Create a serializer
	DOMLSSerializer *mySerializer = ((DOMImplementationLS*)impl)->createLSSerializer();

	// Turn on pretty print
	if (mySerializer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
		mySerializer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
    mySerializer->setNewLine(L"\r\n");

	// Set the target, a memory buffer
	MemBufFormatTarget *target = new MemBufFormatTarget();
	DOMLSOutput *output = ((DOMImplementationLS*)impl)->createLSOutput();
	output->setByteStream(target);

	// Serialize the XML to the memory buffer
	mySerializer->write(doc, output);
    target->flush();

    // Copy memory buffer to a string
    string myStr((char *)target->getRawBuffer());

	// Clean up
    delete target;
	output->release();
	mySerializer->release();

    return myStr;
}