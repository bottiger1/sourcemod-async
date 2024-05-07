#include "context.h"
#include "globals.h"
#include <zlib.h>

CurlContext::CurlContext(CURL* c, IPluginContext* plugin) {
    curl = c;
    curlcode = (CURLcode)-1;
    sourcepawn_plugin_context = plugin;
    in_event_thread = false;
    handle_closed = false;
    headers = NULL;
    post = NULL;
}

CurlContext::~CurlContext() {
    curl_easy_cleanup(curl);
    if(headers != NULL)
        curl_slist_free_all(headers);   
}

void CurlContext::AddHeader(const char* string) {
    curl_slist* new_headers = curl_slist_append(headers, string);
    if(new_headers != NULL)
        headers = new_headers;
}

// called right before event thread adds job to the queue
void CurlContext::OnCurlStarted() {
    in_event_thread = true;
    // compress post if available
    if(post) {
        char* compressBuffer = (char*)malloc(compressBound(postLength));
        if(compressBuffer != NULL) {
            uLongf compressLength;
            int result = compress2((Bytef*)compressBuffer, &compressLength, (Bytef*)post, postLength, Z_BEST_SPEED);
            if(result == Z_OK) {
                free(post);
                post = compressBuffer;
                postLength = compressLength;
                AddHeader("Content-Encoding: gzip");
            } else {
                // compression failed
                free(compressBuffer);
            }
        }

        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postLength);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
    }
}

// called when the game thread loops through finished jobs
void CurlContext::OnCurlCompletedGameThread() {
    in_event_thread = false;

    IPluginFunction* func = GetSourcepawnFunction(sourcepawn_plugin_context, sourcepawn_callback);
    // the plugin that called us disappeared, delete ourselves
    if(func == NULL) {
        rootconsole->ConsolePrint("[%s] Closing %i because plugin was unloaded", SMEXT_CONF_LOGTAG, sourcepawn_handle);
        g_handle_manager.FreeHandle(sourcepawn_handle);
        return;
    }

    if(handle_closed) {
        g_handle_manager.FreeHandle(sourcepawn_handle);
        return;        
    }
    
    long httpresult = 0;
    long size = 0;
    
    if(curlcode == 0) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpresult);
        size = buffer.size();
    }
    
    func->PushCell(sourcepawn_handle);
    func->PushCell(curlcode);
    func->PushCell(httpresult);
    func->PushCell(size);
    func->PushCell(sourcepawn_userdata);
    func->Execute(NULL);
    
    //printf("Finished Code %i size %i\n%.*s\n", curlcode, buffer.size(), 256, &buffer[0]);    
}

// called when event thread finishes a curl job
void CurlContext::OnCurlCompleted() {
    if(post) {
        free(post);
        post = NULL;
    }
}

////////////////

CurlSocketContext::CurlSocketContext(curl_socket_t s) {
    curl_socket = s;
    uv_poll_init_socket(g_loop, &uv_poll_handle, s);
    uv_poll_handle.data = this;
}

void CurlSocketContext::BeginRemoval() {
    uv_poll_stop(&uv_poll_handle);
    uv_close((uv_handle_t*)&uv_poll_handle, CurlSocketContext::OnPollClosed);
}

void CurlSocketContext::OnPollClosed(uv_handle_t* handle) {
    // delete self
    // cannot do this in BeginRemoval or it will crash
    delete (CurlSocketContext*)handle->data;
}