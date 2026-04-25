#include "ChoiceManager.hpp"

#include <iostream>

#include "events/ChoiceAddedEvent.hpp"

void ChoiceManager::add(ChoiceRequest request) {
    _pending.push(std::move(request));
    std::cout << "CHOICE ADDED, publishing event\n";
    ChoiceAddedEvent e;
   dispatcher->publish(e);
}

void ChoiceManager::pop() {
    _pending.pop();
}

bool ChoiceManager::hasPending() const {
    return  !_pending.empty();
}

const ChoiceRequest & ChoiceManager::currentTop() {
    return _pending.top();
}

std::stack<ChoiceRequest> & ChoiceManager::getPending() {
    return _pending;
}

void ChoiceManager::resolve(ChoiceResult result) {
    if (_pending.empty()) {
        std::cout << "Pending Tasks are empty";
        return;
    }
    std::cout << "RESOLVING\n";
    ChoiceRequest req = std::move(_pending.top());
    _pending.pop();
    req.callback(std::move(result));

}
