/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2014 Bottiger.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Version: $Id$
 */

#include <unordered_set>
#include <algorithm>

#include "extension.h"
#include "context.h"
#include "queue.h"
#include "handles.h"

CURLM* g_curl;
uv_loop_t* g_loop;
uv_thread_t g_loop_thread;
uv_timer_t g_timeout;

uv_async_t g_async_stop;
uv_async_t g_async_add;
uv_async_t g_async_curl_settings;

LockedQueue<CurlContext*> g_curl_queue;
LockedQueue<CurlContext*> g_curl_done_queue;
LockedQueue<CurlGlobalOption*> g_curl_option_queue;
std::unordered_set<IPluginContext*> g_plugin_contexts;

HandleManager g_handle_manager;

Sample g_Sample;
SMEXT_LINK(&g_Sample);

void RunCurlJobs(uv_timer_t *req);
void CurlSocketActivity(uv_poll_t *req, int status, int events);

void OnGameFrame(bool simulating) {
    if(!g_curl_queue.Empty()) {
        uv_async_send(&g_async_add);
    }
    if(!g_curl_done_queue.Empty()) {
        g_curl_done_queue.Lock();
        while(!g_curl_done_queue.Empty()) {
            g_curl_done_queue.Pop()->OnCurlCompletedGameThread();
        }
        g_curl_done_queue.Unlock();
    }
}

void CheckCompletedCurlJobs() {
    CURLMsg* message;
    int pending;
    
    while ((message = curl_multi_info_read(g_curl, &pending))) {
        switch (message->msg) {
            case CURLMSG_DONE:
                //rootconsole->ConsolePrint("[%s] done", SMEXT_CONF_LOGTAG);
                CurlContext* curl_context;
                curl_multi_remove_handle(g_curl, message->easy_handle);
                curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, (char*)&curl_context);
                curl_context->curlcode = message->data.result;
                curl_context->OnCurlCompleted();
                g_curl_done_queue.Lock();
                g_curl_done_queue.Push(curl_context);
                g_curl_done_queue.Unlock();
                break;
            default:
                rootconsole->ConsolePrint("[%s] Unknown curl message %i", SMEXT_CONF_LOGTAG, message->msg);
                break;
        }
    }
}

// callbacks made by CURL
int CurlTimeoutChange(CURLM *multi, long timeout_ms, void *userdata) { 
    if(timeout_ms == -1) {
        return 0;
    }
    
    uv_timer_start(&g_timeout, RunCurlJobs, timeout_ms, 0);
    return 0;
}

inline CurlSocketContext* CreateSocketContextIfNeeded(void* socketdata, curl_socket_t s) {
    CurlSocketContext* context;
    if(socketdata == NULL) {
        context = new CurlSocketContext(s);
        curl_multi_assign(g_curl, s, (void*)context);
    } else {
        context = (CurlSocketContext*)socketdata;
    }
    return context;
}

 // called by curl when sockets need to be registered or removed
int CurlSocketCallback(CURL* easy, curl_socket_t s, int action, void* data, void* socketdata) {
    CurlSocketContext* context;    
    switch (action) {
        case CURL_POLL_IN:
            context = CreateSocketContextIfNeeded(socketdata, s);
            uv_poll_start(&context->uv_poll_handle, UV_READABLE, CurlSocketActivity);
            break;
        case CURL_POLL_OUT:
            context = CreateSocketContextIfNeeded(socketdata, s);
            uv_poll_start(&context->uv_poll_handle, UV_WRITABLE, CurlSocketActivity);
            break;
        case CURL_POLL_REMOVE:
            if (socketdata) {
                context = (CurlSocketContext*)socketdata;
                context->BeginRemoval();
                curl_multi_assign(g_curl, s, NULL);
            }
            break;
        default:
            break;
    }
    return 0;
}

size_t CurlReceiveData(char* contents, size_t size, size_t nmemb, void* userdata) {
    size_t realsize = size * nmemb;
    CurlContext* c = (CurlContext*)userdata;
    c->buffer.insert(c->buffer.end(), contents, contents + realsize);
    return realsize;
}

std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}


size_t CurlReceiveHeaders(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t realsize = size * nitems;
    CurlContext* c = (CurlContext*)userdata;
    std::string headerLine(buffer, realsize);

    size_t delimiter = headerLine.find(":");
    if(delimiter == std::string::npos)
    {
        return realsize;
    }

    auto name = headerLine.substr(0, delimiter);
    auto value = headerLine.substr(delimiter+1, headerLine.size());
    value = trim(value);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    //rootconsole->ConsolePrint("header %s value %s", name.data(), value.data());
    c->response_headers[name] = value;
    return realsize;
}

// callbacks made by UV

void RunCurlJobs(uv_timer_t *req) {
    int running;
    curl_multi_socket_action(g_curl, CURL_SOCKET_TIMEOUT, 0, &running);
    CheckCompletedCurlJobs();
}

// activated when UV tells us a socket has activity
void CurlSocketActivity(uv_poll_t *req, int status, int events) {
    uv_timer_stop(&g_timeout);
    
    int running;
    int flags = 0;
    
    if (events & UV_READABLE)
        flags |= CURL_CSELECT_IN;
    if (events & UV_WRITABLE)
        flags |= CURL_CSELECT_OUT;
    
    CurlSocketContext* socket_context = (CurlSocketContext*)req->data;
    curl_multi_socket_action(g_curl, socket_context->curl_socket, flags, &running);
    CheckCompletedCurlJobs();
}

// main event loop thread
void EventLoop(void* data) {
    uv_run(g_loop, UV_RUN_DEFAULT);
}

// helper to add essential curl options and adding it to curl multi interface
void ProcessCurlJob(CurlContext* c) {
    curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, CurlReceiveData);
    curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, (void*)c);
    curl_easy_setopt(c->curl, CURLOPT_HEADERFUNCTION, CurlReceiveHeaders);
    curl_easy_setopt(c->curl, CURLOPT_HEADERDATA, (void*)c);    
    curl_easy_setopt(c->curl, CURLOPT_PRIVATE, (void*)c);
    curl_easy_setopt(c->curl, CURLOPT_NOSIGNAL, 1L);
    if(c->headers != NULL)
        curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, c->headers);
    curl_multi_add_handle(g_curl, c->curl);
}

// Async commands that can be called from other threads

void AddCurlJob(CurlContext* c) {
    c->OnCurlStarted();
    g_curl_queue.Lock();
    g_curl_queue.Push(c);
    g_curl_queue.Unlock();
}

void ChangeCurlSettings(CURLMoption k, long v) {
    CurlGlobalOption* o = new CurlGlobalOption();
    o->key = k;
    o->value = v;
    g_curl_option_queue.Lock();
    g_curl_option_queue.Push(o);
    g_curl_option_queue.Unlock();
    uv_async_send(&g_async_curl_settings);
}

// Async callbacks run in the event thread

void AsyncAddJobs(uv_async_t* handle) {
    g_curl_queue.Lock();
    while(!g_curl_queue.Empty()) {
        CurlContext* c = g_curl_queue.Pop();
        ProcessCurlJob(c);
    }
    g_curl_queue.Unlock();
}

void AsyncStopLoop(uv_async_t* handle) {
    uv_stop(g_loop);
}

void AsyncChangeCurlSettings(uv_async_t* handle) {
    g_curl_option_queue.Lock();
    while(!g_curl_option_queue.Empty()) {
        CurlGlobalOption* o = g_curl_option_queue.Pop();
        curl_multi_setopt(g_curl, o->key, o->value);
        delete o;
    }
    g_curl_option_queue.Unlock();
}

// Sourcepawn plugin detection

class PluginsListener : public IPluginsListener {
    public:
        virtual void OnPluginLoaded(IPlugin *plugin) {
            g_plugin_contexts.insert(plugin->GetBaseContext());
        }
        
        virtual void OnPluginUnloaded(IPlugin *plugin) {
            g_plugin_contexts.erase(plugin->GetBaseContext());
        }
};
PluginsListener g_plugins_listener;

IPluginFunction* GetSourcepawnFunction(IPluginContext* context, cell_t id) {
    if(g_plugin_contexts.find(context) == g_plugin_contexts.end())
        return NULL;
    
    return context->GetFunctionById(id);
}

// Sourcemod Plugin Events
bool Sample::SDK_OnLoad(char *error, size_t maxlength, bool late) {
    extern sp_nativeinfo_t MyNatives[];
    sharesys->AddNatives(myself, MyNatives);
    sharesys->RegisterLibrary(myself, "async");

    curl_global_init(CURL_GLOBAL_ALL);
    
    g_loop = uv_default_loop();
    g_curl = curl_multi_init();
    
    curl_multi_setopt(g_curl, CURLMOPT_SOCKETFUNCTION, CurlSocketCallback);
    curl_multi_setopt(g_curl, CURLMOPT_TIMERFUNCTION, CurlTimeoutChange);    
    
    uv_timer_init(g_loop, &g_timeout);
        
    uv_async_init(g_loop, &g_async_stop, AsyncStopLoop);
    uv_async_init(g_loop, &g_async_add, AsyncAddJobs);
    uv_async_init(g_loop, &g_async_curl_settings, AsyncChangeCurlSettings);
    
    uv_thread_create(&g_loop_thread, EventLoop, NULL);
    
    // detect which plugins are loaded so we don't crash calling a function in a plugin that was unloaded
    IPluginIterator* plugin_iterator = plsys->GetPluginIterator();
    while (plugin_iterator->MorePlugins()) {
        g_plugin_contexts.insert(plugin_iterator->GetPlugin()->GetBaseContext());
        plugin_iterator->NextPlugin();
    }
    plugin_iterator->Release();

    // add hooks
    plsys->AddPluginsListener((IPluginsListener*)&g_plugins_listener);
    smutils->AddGameFrameHook(OnGameFrame);
    
    return true;
}

void Sample::SDK_OnUnload() {    
    rootconsole->ConsolePrint("[%s] Waiting until event thread ends to prevent crash.", SMEXT_CONF_LOGTAG);
    uv_async_send(&g_async_stop);
    uv_thread_join(&g_loop_thread);
    rootconsole->ConsolePrint("[%s] Event thread ended.", SMEXT_CONF_LOGTAG);
    
    uv_loop_close(g_loop);
    curl_multi_cleanup(&g_curl);
    curl_global_cleanup();
    
    g_handle_manager.CleanHandles();
    
    plsys->RemovePluginsListener((IPluginsListener*)&g_plugins_listener);
    smutils->RemoveGameFrameHook(OnGameFrame);
}
