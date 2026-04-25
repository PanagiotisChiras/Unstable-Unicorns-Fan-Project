#include "eventDispatcher/EventDispatcher.hpp"

void EventDispatcher::unsubscribe(const ListenerHandle &handle) {

    auto it = registry.find(handle.type);
    if (it == registry.end()) return;
    auto& listeners = it->second;

    if (dispatchDepth > 0) {
        for (auto& wrapper : listeners) {
            if (wrapper.id == handle.id)
                wrapper.pendingToUnsubscribe = true;
        }
    } else {
        std::erase_if(listeners,
            [&](const ListenerWrapper& w) { return w.id == handle.id; });
    }
    std::cout << "UNSUBSCRIBING DONE\n";
}

