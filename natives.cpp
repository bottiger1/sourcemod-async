#include "globals.h"
#include <string.h>
#include <algorithm>

int g_default_timeout = 10000;

#define GET_CURL_CONTEXT_MACRO \
    CurlContext* context = g_handle_manager.GetCurlPointer(params[1]); \
    if(context == NULL || context->in_event_thread) \
        return 2;

cell_t CurlNew(IPluginContext *pContext, const cell_t *params) {
    CURL* c = curl_easy_init();
    if(c == NULL)
        return 0;
    CurlContext* context = new CurlContext(c, pContext);
    int handle = g_handle_manager.NewHandle((void*)context, HANDLE_CURL);
    if(handle == 0) {
        delete context;
        return 0;
    }
    context->sourcepawn_handle = handle;
    context->sourcepawn_userdata = params[1];
    
    curl_easy_setopt(context->curl, CURLOPT_TIMEOUT_MS, g_default_timeout);
    curl_easy_setopt(context->curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(context->curl, CURLOPT_MAXREDIRS, 10);
    curl_easy_setopt(context->curl, CURLOPT_USERAGENT, "Sourcemod " SMEXT_CONF_NAME " " SMEXT_CONF_VERSION);
    
    return handle;
}

cell_t Close(IPluginContext *pContext, const cell_t *params) {
    CurlContext* context = g_handle_manager.GetCurlPointer(params[1]);
    if(context == NULL) {
        return 2;
    } else if(context->in_event_thread) {
        context->handle_closed = true;
    } else {
        g_handle_manager.FreeHandle(params[1]);
    }
    return 0;
}

cell_t CurlGet(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO
    
    char* url;
    pContext->LocalToString(params[2], &url);
    curl_easy_setopt(context->curl, CURLOPT_URL, url);
    context->sourcepawn_callback = params[3];
    AddCurlJob(context);
    return 0;
}

cell_t CurlPost(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO

    char* url;
    char* post;
    
    pContext->LocalToString(params[2], &url);
    pContext->LocalToString(params[3], &post);
    
    curl_easy_setopt(context->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->curl, CURLOPT_POST, 1);
    curl_easy_setopt(context->curl, CURLOPT_COPYPOSTFIELDS, post);
    context->sourcepawn_callback = params[4];
    AddCurlJob(context);    
    return 0;
}

cell_t CurlPostRaw(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO

    char* url;
    char* post;
    
    pContext->LocalToString(params[2], &url);
    pContext->LocalToString(params[3], &post);
    
    curl_easy_setopt(context->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->curl, CURLOPT_TIMEOUT_MS, g_default_timeout);
    curl_easy_setopt(context->curl, CURLOPT_POST, 1);
    curl_easy_setopt(context->curl, CURLOPT_POSTFIELDSIZE, params[4]);
    curl_easy_setopt(context->curl, CURLOPT_COPYPOSTFIELDS, post);
    context->sourcepawn_callback = params[5];
    AddCurlJob(context);    
    return 0;
}

cell_t CurlPostRawCompress(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO

    char* url;
    char* post;
    
    pContext->LocalToString(params[2], &url);
    pContext->LocalToString(params[3], &post);
    int postLength = params[4];

    context->post = (char*)malloc(postLength);
    context->postLength = postLength;
    memcpy(context->post, post, postLength);
    
    curl_easy_setopt(context->curl, CURLOPT_URL, url);
    curl_easy_setopt(context->curl, CURLOPT_TIMEOUT_MS, g_default_timeout);
    curl_easy_setopt(context->curl, CURLOPT_POST, 1);
    context->sourcepawn_callback = params[5];
    AddCurlJob(context);    
    return 0;
}

cell_t CurlStart(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO
    
    context->sourcepawn_callback = params[2];
    AddCurlJob(context);
    return 0;
}

cell_t CurlGetData(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO
    
    char* buffer;
    pContext->LocalToString(params[2], &buffer);
    int sbuffer_size = params[3];
    int cbuffer_size = context->buffer.size();
    int min = sbuffer_size < cbuffer_size ? sbuffer_size : cbuffer_size;
    memcpy(buffer, &context->buffer[0], min);
    if(sbuffer_size > cbuffer_size)
        buffer[cbuffer_size+1] = 0;
    return 0;
}

cell_t CurlSetInt(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO
    
    return curl_easy_setopt(context->curl, (CURLoption)params[2], (long)params[3]);
}

cell_t CurlSetString(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO
    
    char* value;
    pContext->LocalToString(params[3], &value);
    return curl_easy_setopt(context->curl, (CURLoption)params[2], value);
}

cell_t CurlGetInt(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO
    
    long output;
    cell_t* sp_output;
    
    CURLcode curlcode = curl_easy_getinfo(context->curl, (CURLINFO)params[2], &output);
    if(curlcode == 0) {
        pContext->LocalToPhysAddr(params[3], &sp_output);
        *sp_output = (cell_t)output;
    }
    return curlcode;
}

cell_t CurlGetString(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO
    
    char* result;
    CURLcode curlcode = curl_easy_getinfo(context->curl, (CURLINFO)params[2], &result);
    pContext->StringToLocal(params[3], params[4], result);
    return curlcode;
}

cell_t CurlAddHeader(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO
    
    char* header;
    pContext->LocalToString(params[2], &header);
    context->AddHeader(header);
    return 0;
}

cell_t CurlSetGlobalInt(IPluginContext *pContext, const cell_t *params) {
    ChangeCurlSettings((CURLMoption)params[1], params[2]);
    return 0;
}

cell_t CurlErrorString(IPluginContext *pContext, const cell_t *params) {
    const char* error = curl_easy_strerror((CURLcode)params[1]);
    pContext->StringToLocal(params[2], params[3], error);
    return 0;
}

cell_t CurlVersion(IPluginContext *pContext, const cell_t *params) {
    char* version = curl_version();
    pContext->StringToLocal(params[1], params[2], version);
    return 0;
}

cell_t CurlGetResponseHeaderSize(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO

    char* headerNameRaw;
    pContext->LocalToString(params[2], &headerNameRaw);

    std::string headerName(headerNameRaw);
    std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);

    if(context->response_headers.find(headerName) == context->response_headers.end())
    {
        return -1;
    }
    else
    {
        return context->response_headers[headerName].size();
    }
}

cell_t CurlGetResponseHeader(IPluginContext *pContext, const cell_t *params) {
    GET_CURL_CONTEXT_MACRO

    char* headerNameRaw;
    pContext->LocalToString(params[2], &headerNameRaw);

    std::string headerName(headerNameRaw);
    std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);

    if(context->response_headers.find(headerName) == context->response_headers.end())
    {
        return -1;
    }
    else
    {
        auto result = context->response_headers[headerName];
        pContext->StringToLocal(params[3], params[4], result.data());
        return result.size();
    }
}

sp_nativeinfo_t MyNatives[] = {
    {"Async_Close",                         Close},
    {"Async_CurlNew",                       CurlNew},
    {"Async_CurlGet",                       CurlGet},
    {"Async_CurlPost",                      CurlPost},
    {"Async_CurlPostRaw",                   CurlPostRaw},
    {"Async_CurlPostRawCompress",           CurlPostRawCompress},
    {"Async_CurlGetData",                   CurlGetData},
    {"Async_CurlAddHeader",                 CurlAddHeader},
    {"Async_CurlStart",                     CurlStart},
    {"Async_CurlSetInt",                    CurlSetInt},
    {"Async_CurlSetString",                 CurlSetString},
    {"Async_CurlGetInt",                    CurlGetInt},
    {"Async_CurlGetString",                 CurlGetString},
    {"Async_CurlSetGlobalInt",              CurlSetGlobalInt},
    {"Async_CurlErrorString",               CurlErrorString},
    {"Async_CurlVersion",                   CurlVersion},
    {"Async_CurlGetResponseHeaderSize",     CurlGetResponseHeaderSize},
    {"Async_CurlGetResponseHeader",         CurlGetResponseHeader},
    {NULL,                      NULL},
};