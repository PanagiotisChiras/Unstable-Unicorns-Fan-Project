#include "ListenerHandle.hpp"
#include "EventDispatcher.hpp"

ListenerHandle::~ListenerHandle() {
    if (dispatcher) {

        dispatcher->unsubscribe(*this);
    }
}

ListenerHandle::ListenerHandle(ListenerHandle&& other) noexcept
    : type(other.type), id(other.id), dispatcher(other.dispatcher)
{
    other.dispatcher = nullptr;
}

ListenerHandle & ListenerHandle::operator=(ListenerHandle && other) noexcept {
    if (&other == this) return *this;

    if (dispatcher) dispatcher->unsubscribe(*this);

    type = other.type;
    id = other.id;
    dispatcher = other.dispatcher;

    other.dispatcher = nullptr;

    return *this;
}
