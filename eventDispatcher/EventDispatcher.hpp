#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <typeindex>
#include "ListenerHandle.hpp"

class EventDispatcher {
public:
    template<typename T>
    using CallBack = std::function<void(T &)>;

    template<typename T>
    ListenerHandle listenFor(CallBack<T> action, std::size_t priority = 0);

    template<typename T>
    ListenerHandle once(CallBack<T> action, std::size_t priority = 0);

    template<typename EventType>
    void publish(EventType &event);

    void unsubscribe(const ListenerHandle &handle);

private:
    struct ListenerWrapper {
        std::size_t id;
        std::function<void(void *)> fn;
        std::size_t priority{};
        bool pendingToUnsubscribe = false;
    };

    std::unordered_map<std::type_index, std::vector<ListenerWrapper> > registry;
    std::size_t nextId{0};
   int dispatchDepth{0};
};


template<typename T>
ListenerHandle EventDispatcher::listenFor(CallBack<T> action, std::size_t priority) {
    auto type = std::type_index(typeid(T));
    std::size_t id = ++nextId;
    registry[type].push_back(
        ListenerWrapper{
            id,
            [action = std::move(action)](void *e) {
                action(*static_cast<T *>(e));
            },
            priority
        }
    );
    auto wrapper = registry.find(type);
    std::sort(wrapper->second.begin(),
              wrapper->second.end(),
              [](const ListenerWrapper &a, const ListenerWrapper &b) {
                  return a.priority > b.priority;
              });
    return ListenerHandle{type, id,this};
}

template<typename T>
ListenerHandle EventDispatcher::once(CallBack<T> action, std::size_t priority) {
    auto sharedHandle = std::make_shared<ListenerHandle>(
        ListenerHandle{std::type_index(typeid(T)), 0,this}
    );
    *sharedHandle = listenFor<T>(
        [this, action = std::move(action), sharedHandle](T &e) mutable {
            action(e);
            unsubscribe(*sharedHandle);
        },
        priority
    );

    return std::move(*sharedHandle);
}template<typename EventType>
void EventDispatcher::publish(EventType& event) {
    auto key = std::type_index(typeid(EventType));
    auto it = registry.find(key);
    if (it == registry.end()) return;

    // always clean pending BEFORE taking snapshot, regardless of depth
    it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
        [](const ListenerWrapper& w) { return w.pendingToUnsubscribe; }
    ), it->second.end());

    auto listeners = it->second;
    dispatchDepth++;
    for (auto& wrapper : listeners) {
        if (!wrapper.pendingToUnsubscribe) // skip if unsubscribed mid-dispatch
            wrapper.fn(&event);
    }
    dispatchDepth--;

    if (dispatchDepth == 0) {
        auto& original = registry[key];
        original.erase(std::remove_if(original.begin(), original.end(),
            [](const ListenerWrapper& w) { return w.pendingToUnsubscribe; }
        ), original.end());
    }
}


#endif
