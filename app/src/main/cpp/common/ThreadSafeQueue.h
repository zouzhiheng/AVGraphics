//
// Created by zzh on 2017/12/28 0028.
//

#ifndef FFMEDIA_ZZH_THREADSAFEQUEUE_H
#define FFMEDIA_ZZH_THREADSAFEQUEUE_H

#include <queue>
#include <mutex>

template<typename T>
class ThreadSafeQueue {
 private:
    std::queue<T> dataQueue;
    mutable std::mutex m;
 public:
    ThreadSafeQueue() {
    }

    void push(T element) {
        std::lock_guard<std::mutex> lock(m);
        dataQueue.push(element);
    }

    T pop() {
        std::lock_guard<std::mutex> lock(m);
        T result = dataQueue.front();
        dataQueue.pop();
        return result;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m);
        return dataQueue.empty();
    }

    int size() {
        std::lock_guard<std::mutex> lock(m);
        return dataQueue.size();
    }
 };

#endif //FFMEDIA_ZZH_THREADSAFEQUEUE_H
