#ifndef ASYNC_QUEUE_H
#define ASYNC_QUEUE_H

#include <deque>
#include <uv.h>

template <class T> 
class LockedQueue {
    uv_mutex_t lock;
    std::deque<T> queue;
    
public:    
    LockedQueue() {
        uv_mutex_init(&lock);
    }
    
    ~LockedQueue() {
        uv_mutex_destroy(&lock);
    }
    
    void Lock() {
        uv_mutex_lock(&lock);
    }
    
    void Unlock() {
        uv_mutex_unlock(&lock);
    }
    
    T Pop() {
        T output = queue.front();
        queue.pop_front();
        return output;
    }
    
    void Push(T item) {
        queue.push_back(item);
    }
    
    bool Empty() {
        return queue.empty();
    }
};

#endif