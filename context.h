/*
* Definitions for custom contexts to use with CURL and UV
*/

#ifndef ASYNC_CONTEXT_H
#define ASYNC_CONTEXT_H

#include <vector>
#include <uv.h>
#include <curl/curl.h>
#include <map>

#include "smsdk_ext.h"

class CurlContext {
public:
    CURL* curl;
    CURLcode curlcode;
    curl_slist* headers;
    std::map<std::string, std::string> response_headers;
    std::vector<char> buffer; // download data
    char* post; // uncompressed post data that will be compressed in the event thread before sending
    int postLength;
    IPluginContext* sourcepawn_plugin_context;
    funcid_t sourcepawn_callback;
    int sourcepawn_handle;
    int sourcepawn_userdata;
    bool in_event_thread;
    bool handle_closed; // if someone try to close the handle when it was processing, close it when it times out or completes

    CurlContext(CURL* c, IPluginContext* plugin);
    ~CurlContext();
    void AddHeader(const char* string);
    void OnCurlStarted();
    void OnCurlCompleted();
    void OnCurlCompletedGameThread();
};

class CurlSocketContext {
public:
    curl_socket_t curl_socket;
    uv_poll_t uv_poll_handle;
    
    CurlSocketContext(curl_socket_t s);
    void BeginRemoval();
    static void OnPollClosed(uv_handle_t* handle);
};

#endif