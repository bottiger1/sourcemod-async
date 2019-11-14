#include "handles.h"

HandleManager::HandleManager() {
    next_handle = 1;
}

void HandleManager::CleanHandles() {
    for(HandleMapType::iterator it = used_handles.begin();it != used_handles.end();it++) {
        DeleteHandlePointer(it->second);
    }
}

CurlContext* HandleManager::GetCurlPointer(int handle) {
    HandleMapType::const_iterator it = used_handles.find(handle);
    if(it == used_handles.end() || it->second.type != HANDLE_CURL)
        return 0;
    return static_cast<CurlContext*>(it->second.pointer);
}

int HandleManager::NewHandle(void* pointer, HandleType type) {
    int handle_number;
    Handle h;
    h.type = type;
    h.pointer = pointer;
    
    if(!freed_handles.empty()) {
        handle_number = freed_handles.top();
        freed_handles.pop();
    } else if(next_handle == std::numeric_limits<int>::max()) {
        return 0;
    } else {
        handle_number = next_handle++;
    }
    
    used_handles[handle_number] = h;
    return handle_number;
}

void HandleManager::FreeHandle(int handle) {
    HandleMapType::const_iterator it = used_handles.find(handle);
    if(it != used_handles.end()) {
        freed_handles.push(it->first);
        DeleteHandlePointer(it->second);
        used_handles.erase(it);
    }
}

void HandleManager::DeleteHandlePointer(Handle h) {
    switch(h.type) {
        case HANDLE_CURL:
            delete static_cast<CurlContext*>(h.pointer);
            break;
        default:
            break;
    }
}