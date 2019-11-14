#ifndef ASYNC_HANDLES_H
#define ASYNC_HANDLES_H

#include <unordered_map>
#include <stack>
#include <limits>

#include "context.h"

enum HandleType {
    HANDLE_CURL,
    HANDLE_SOCKET,
    HANDLE_NONE,
};

struct Handle {
    HandleType type;
    void* pointer;
};

typedef std::unordered_map<int, Handle> HandleMapType;

class HandleManager {
    int next_handle;
    std::stack<int> freed_handles;
    HandleMapType used_handles;
    
public:
    HandleManager();
    void CleanHandles();
    CurlContext* GetCurlPointer(int handle);
    int NewHandle(void* pointer, HandleType type);
    void FreeHandle(int handle);
private:
    void DeleteHandlePointer(Handle h);
};
#endif