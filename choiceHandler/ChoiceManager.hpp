#ifndef CHOICEMANAGER_HPP
#define CHOICEMANAGER_HPP
#include <stack>

#include "ChoiceRequest.hpp"
#include "eventDispatcher/EventDispatcher.hpp"

class ChoiceManager {

public:
    explicit ChoiceManager(EventDispatcher* disp) : dispatcher(disp){}
    void add(ChoiceRequest request);
    void pop();
    [[nodiscard]] bool hasPending() const;
    const ChoiceRequest& currentTop();

    std::stack<ChoiceRequest>& getPending();
    void resolve(ChoiceResult result);



private:
    std::stack<ChoiceRequest> _pending;
    EventDispatcher* dispatcher;
};

#endif
