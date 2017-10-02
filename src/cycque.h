#ifndef CYCQUE_H_
#define CYCQUE_H_

#include <atomic>
#include <iostream>
#include <cassert>
#include <exception>

template <typename T, unsigned MaxSize = 16>
class cycque {
public:
    ~cycque() {
        delete[] _c;
    }
    cycque& operator=(const cycque&) = delete;
    cycque(): _c(new T[MaxSize]), _head(0), _tail(0) {
        if (!_c)
            throw std::runtime_error("No enough memory when construct cycque.");
    }

public:
    unsigned capasity() const {
        return MaxSize;
    }
    unsigned size() const {
        return (_head.load() + MaxSize - _tail.load()) % MaxSize;
    }
    bool is_empty() const {
        return _tail.load() == _head.load();
    }
    bool is_full() const {
        return successor(_head.load()) == _tail.load();
    }
    bool pop(T& ele) {
        unsigned tail = _tail.load(std::memory_order_relaxed);
        unsigned next_tail = successor(tail);
        unsigned head = _head.load(std::memory_order_acquire);
        if (head == tail)
            return false;
        ele = _c[tail];
        _tail.store(next_tail, std::memory_order_release);
        return true;
    }
    bool push(const T& ele) {
        unsigned head = _head.load(std::memory_order_relaxed);
        unsigned next_head = successor(_head);
        unsigned tail = _tail.load(std::memory_order_acquire);
        if (next_head == tail)
            return false;
        _c[head] = ele;
        _head.store(next_head, std::memory_order_release);
        return true;
    }

private:
    unsigned successor(unsigned a) const {
        return (a + 1) % MaxSize;
    }

private:
    T* const _c;
    volatile std::atomic_uint _head;
    volatile std::atomic_uint _tail;
}; //class cycque

#endif //CYCQUE_H_
