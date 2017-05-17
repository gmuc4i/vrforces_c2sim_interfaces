#pragma once

//#include "microhttpd.h"

//Moved here by stmxgr from stdafx.h
#define PORT 8080
//#include <stdio.h>
//#include <tchar.h>
//#include <iostream>
#ifndef MHD_PLATFORM_H
#define MHD_PLATFORM_H
#include <winsock2.h>
#include <WS2tcpip.h>	
typedef SSIZE_T ssize_T;
typedef UINT64	uint64_t;
typedef UINT16	uint16_t;
extern "C"
{
#include "microhttpd.h"
}
#endif

using namespace std;


int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
std::string parse_url(const char* URL, const char* targetString);
int request_completed(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe);
int send_page(struct MHD_Connection *connection, const char *page);
int start_http(long port, bool doValidation, IWISEDriverSink * s, WISE_HANDLE h, CWISEDriver* t);
void stop_http(); #pragma once
