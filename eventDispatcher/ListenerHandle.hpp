#ifndef LISTENERHANDLE_HPP
#define LISTENERHANDLE_HPP

#include <cstddef>
#include <typeindex>

class EventDispatcher;

struct ListenerHandle {
    std::type_index type;
    std::size_t id;

    ListenerHandle(std::type_index type, std::size_t id, EventDispatcher* dispatcher)
    : type(type), id(id), dispatcher(dispatcher) {}

    ListenerHandle() : type(typeid(void)), id(0), dispatcher(nullptr) {}
    EventDispatcher* dispatcher;

    ~ListenerHandle();

    ListenerHandle(const ListenerHandle& ) = delete;
    ListenerHandle& operator=(const ListenerHandle& ) = delete;

    ListenerHandle(ListenerHandle&&) noexcept;
    ListenerHandle& operator=(ListenerHandle&& other) noexcept;
};

#endif
