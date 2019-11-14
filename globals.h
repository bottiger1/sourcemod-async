#ifndef ASYNC_GLOBALS_H
#define ASYNC_GLOBALS_H

#include <uv.h>
#include <curl/curl.h>

#include "smsdk_ext.h"
#include "context.h"
#include "handles.h"

extern CURLM* g_curl;
extern uv_loop_t* g_loop;
extern HandleManager g_handle_manager;

void AddCurlJob(CurlContext* c);
void ChangeCurlSettings(CURLMoption k, long v);
IPluginFunction* GetSourcepawnFunction(IPluginContext* context, cell_t id);

#endif