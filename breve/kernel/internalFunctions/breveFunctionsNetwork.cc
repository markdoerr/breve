/*****************************************************************************
 *                                                                           *
 * The breve Simulation Environment                                          *
 * Copyright (C) 2000, 2001, 2002, 2003 Jonathan Klein                       *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 *****************************************************************************/

#include "kernel.h"
#include "breveFunctionsNetwork.h"

#ifndef MINGW
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <wininet.h>
#endif /* MINGW */

/*!
	\brief Data about a network client, passed off to the function which
	handles the connection.
*/

struct brNetworkClientData {
	brEngine *engine;
	brNetworkServerData *server;
	int socket;
	struct sockaddr_in addr;
};

/*! \addtogroup InternalFunctions */
/*@{*/

int brINetworkSetRecipient(brEval args[], brEval *target, brInstance *i) {
	brNetworkServerData *data = BRPOINTER(&args[0]);
	data->recipient = BRINSTANCE(&args[1]);

	return EC_OK;
}

/*!
	\brief Shuts down a network server.

	void closeServer(brNetworkServerData pointer).
*/

int brICloseServer(brEval args[], brEval *target, brInstance *i) {
	brNetworkServerData *data = BRPOINTER(&args[0]);

	if(!data) {
		slMessage(DEBUG_ALL, "closeServer called with invalid data\n");
		return EC_OK;
	}

	data->terminate = 1;

	shutdown(data->socket, 2);
	close(data->socket);

	if(data->index) slFree(data->index);

	pthread_join(data->thread, NULL);

	return EC_OK;
}

/*!
	\brief Sets the HTML index page of a server.

	void setIndexPage(brNetworkServerData pointer, string).
*/

int brISetIndexPage(brEval args[], brEval *target, brInstance *i) {
	brNetworkServerData *data = BRPOINTER(&args[0]);

	if(data->index) slFree(data->index);

	data->index = slStrdup(BRSTRING(&args[1]));

	return EC_OK;
}

/*!
	\brief Gets the URL required to connect to the given server.

	string getServerURL(brNetworkServerData pointer).
*/

int brIGetServerURL(brEval args[], brEval *target, brInstance *i) {
	brNetworkServerData *data = BRPOINTER(&args[0]);
	char hostname[1024], url[1024];
	int l = 1024;

	gethostname(hostname, l);

	sprintf(url, "http://%s:%d", hostname, data->port);

	BRSTRING(target) = slStrdup(url);

	return EC_OK;
}

/*!
	\brief Starts a network server listening on the given port.

	brNetworkServerData pointer listenOnPort(int).
*/

int brIListenOnPort(brEval args[], brEval *target, brInstance *i) {
	BRPOINTER(target) = brListenOnPort(BRINT(&args[0]), i->engine);

	return EC_OK;
}

/*@}*/

/*!
	\brief Initialize the breve engine internal networking functions.
*/

void breveInitNetworkFunctions(brNamespace *n) {
	brNewBreveCall(n, "listenOnPort", brIListenOnPort, AT_POINTER, AT_INT, 0);
	brNewBreveCall(n, "networkSetRecipient", brINetworkSetRecipient, AT_NULL, AT_POINTER, AT_INSTANCE, 0);
	brNewBreveCall(n, "getServerURL", brIGetServerURL, AT_STRING, AT_POINTER, 0);
	brNewBreveCall(n, "closeServer", brICloseServer, AT_NULL, AT_POINTER, 0);
	brNewBreveCall(n, "setIndexPage", brISetIndexPage, AT_NULL, AT_POINTER, AT_STRING, 0);
}

/*!
	\brief Get a hostname from an address.
*/

char *brHostnameFromAddr(struct in_addr *addr) {
	struct hostent *h;
	static char numeric[256];

	h = gethostbyaddr((const void*)addr, 4, AF_INET);

	if(!h) {
		unsigned char *address = (void*)addr;

		sprintf(numeric, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);

		return numeric;
	}

	return h->h_name;
}

/*!
	\brief Starts a network server on a given port.
*/

brNetworkServerData *brListenOnPort(int port, brEngine *engine) {
	int ssock;
	brNetworkServerData *serverData;
	char hostname[1024];
	int l = 1024;
	struct sockaddr_in saddr;

	if((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket");
		return NULL;
	}

	serverData = malloc(sizeof(brNetworkServerData));

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = INADDR_ANY;

	if(bind(ssock, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
		perror("Cannot bind socket");
		return NULL;
	}

	if(listen(ssock, 5) < 0) {
		slMessage(DEBUG_ALL, "Network server unable to listen for connections on port %d\n", port);
		return NULL;
	}

	serverData->index = NULL;
	serverData->engine = engine;
	serverData->recipient = engine->controller;
	serverData->port = port;
	serverData->socket = ssock;
	serverData->terminate = 0;
	pthread_create(&serverData->thread, NULL, (void*)brListenOnSocket, serverData);

	gethostname(hostname, l);

	return serverData;
}

/*!
	\brief Listens for connection on a socket.
*/

void *brListenOnSocket(brNetworkServerData *serverData) {
	brNetworkClientData *clientData;
	int caddr_size = sizeof(struct sockaddr_in);

	while(!serverData->terminate) {
		clientData = malloc(sizeof(brNetworkClientData));

		clientData->engine = serverData->engine;
		clientData->server = serverData;
		clientData->socket = accept(serverData->socket, (struct sockaddr*)&clientData->addr, &caddr_size);

		if(clientData->socket != -1) {
			// fcntl(clientData->socket, F_SETFL, O_NONBLOCK);
			brHandleConnection(clientData);
			close(clientData->socket);
		}
	}

	return NULL;
}

/*!
	\brief Handles a network connection.
*/

void *brHandleConnection(void *p) {
	brNetworkClientData *data = p;	
	brNetworkRequest request;
	char *hostname, *buffer;
	brStringHeader header;
	int length, count;
	brMethod *method;
	brEval eval[2], result, *args[2];
  
	hostname = brHostnameFromAddr(&data->addr.sin_addr);

	// slMessage(DEBUG_ALL, "network connection from %s\n", hostname);

	count = slUtilRead(data->socket, &request, sizeof(brNetworkRequest));

	if(!strncasecmp((char*)&request, "GET ", 4)) {
		char *http;

		// if the data we read is as large as the brNetworkRequest, then
		// there is more data left to go.  if it's shorter, then we've 
		// already hit the end of the stream.

		if(count == sizeof(brNetworkRequest)) {
			http = brFinishNetworkRead(data, &request);
		} else {
			http = slStrdup((char*)&request);
		}

		brHandleHTTPConnection(data, http);

		slFree(http);

		return NULL;
	}

	if(request.magic != NETWORK_MAGIC) {
		slMessage(DEBUG_ALL, "network connection from invalid client\n");
		return NULL;
	}

	switch(request.type) {
		case NR_XML:
			slUtilRead(data->socket, &header, sizeof(brStringHeader));
			length = header.length;
			// slMessage(DEBUG_ALL, "received XML message of length %d from host %s\n", length, hostname); 
			buffer = slMalloc(length+1);
			slUtilRead(data->socket, buffer, length);
			buffer[length] = 0;
			pthread_mutex_lock(&data->engine->lock);

			args[0] = &eval[0];
			args[1] = &eval[1];

			BRSTRING(&eval[0]) = buffer;
			eval[0].type = AT_STRING;

			brMethodCallByNameWithArgs(data->engine->controller, "parse-xml-network-request", args, 1, &result);

			args[0] = &result;

			BRSTRING(&eval[1]) = hostname;
			eval[1].type = AT_STRING;

			method = brMethodFindWithArgRange(data->server->recipient->object, "accept-upload", NULL, 0, 2);

			if(method) {
				brMethodCall(data->server->recipient, method, args, &result);
				brMethodFree(method);	
			}

			slFree(buffer);

			pthread_mutex_unlock(&data->engine->lock);
			break;
	}

	return NULL;
}

/*!
	\brief Finishes a network read.

	The stream has already been partially read in order to determine
	what kind of request we're dealing with, so here we ready the rest.
*/

char *brFinishNetworkRead(brNetworkClientData *data, brNetworkRequest *request) {
	char *d = slMalloc(sizeof(brNetworkRequest));
	char buffer[1024];
	int count;
	int size = sizeof(brNetworkRequest);

	bcopy(request, d, sizeof(brNetworkRequest));

	while((count = brHTTPReadLine(data->socket, buffer, 1024)) == 1024) {
		d = slRealloc(d, size + count + 1);

		printf("read %d bytes\n", count);

		bcopy(buffer, &d[size], count);

		size += count;
	}

	d[size] = 0;

	return d;
}

/*!
	\brief A baby little HTTP server, takes care of callbacks. 
*/

int brHandleHTTPConnection(brNetworkClientData *data, char *request) {
		char *end, *method, *string;
		brEval target;
		int count, n;
		int result;
		brEval *evals, **evalPtrs;

		// remove the "GET ", skip forward to the request

		request += 4;
		while(*request && isspace(*request)) request++;
		end = request;
		while(*end && !isspace(*end)) end++;
		*end = 0;

		if(*request == '/') request++;

		// if the request was ONLY "/", then they want the index 

		if(*request == 0 && data->server->index) {
			request = data->server->index;
		}

		if(strstr(request, ".html") || request == data->server->index) {
			brSendPage(data, request);
			return 0;
		}

		method = slSplit(request, "_", 0);

		n = 0;
		count = 0;

		while(request[n]) {
			if(request[n] == '_') count++;
			n++;
		}

		evalPtrs = alloca(sizeof(brEval*) * count);
		evals = alloca(sizeof(brEval) * count);

		for(n=0;n<count;n++) {
			evals[n].type = AT_STRING;
			BRSTRING(&evals[n]) = slSplit(request, "_", n + 1);
			evalPtrs[n] = &evals[n];
		}

		result = brMethodCallByNameWithArgs(data->engine->controller, method, evalPtrs, count, &target);

		slFree(method);

		if(result != EC_OK) {
			slUtilWrite(data->socket, SL_NET_FAILURE, strlen(SL_NET_FAILURE));
			return 0;
		}

		if(target.type == AT_NULL) {
			if(data->server->index) brSendPage(data, data->server->index);
			else slUtilWrite(data->socket, SL_NET_SUCCESS, strlen(SL_NET_SUCCESS));

			return 0;
		}	

		string = brFormatEvaluation(&target, data->engine->controller);

		if(strstr(string, ".html")) {
			brSendPage(data, string);
			return 0;
		}

		slUtilWrite(data->socket, string, strlen(string));

		return 0;
}

/*!
	\brief Sends an html page over a socket.
*/	

void brSendPage(brNetworkClientData *data, char *page) {
	char *file, *text;

	file = brFindFile(data->engine, page, NULL);

	if(!file) {
		slUtilWrite(data->socket, SL_NET_404, strlen(SL_NET_404));
		slMessage(DEBUG_ALL, "network request for unknown file: %s\n", page);
		return;
	}

	text = slUtilReadFile(file);

	// slMessage(DEBUG_ALL, "network request for file: %s\n", page);
	slUtilWrite(data->socket, text, strlen(text));

	slFree(file);
	slFree(text);

	return;
}

/*!
	\brief Reads an HTTP request until no more data is found.
*/

int brHTTPReadLine(int socket, void *buffer, size_t size ) {
    int n, readcount = 0;

    while(readcount < size) {
        n = read(socket, buffer + readcount, size - readcount);

		if(strchr(buffer, '\n')) return readcount;
        if(n < 1) return readcount;

        readcount += n;
    }

    return readcount;
}
