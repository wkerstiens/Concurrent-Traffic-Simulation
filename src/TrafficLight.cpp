#include <iostream>
#include <random>
#include <future>

#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> lock(messageQueueMutex);
    _conditionVariable.wait(lock, [this] { return !_queue.empty(); });

    T nextMessage = std::move(_queue.back());  // _queue.end is an iterator thing
    _queue.pop_back();
    return nextMessage;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock(messageQueueMutex);
    _queue.push_front(std::move(msg));
    _conditionVariable.notify_one();
}


/* Implementation of class "TrafficLight" */
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true)
    {
        if(messageQueue.receive() == TrafficLightPhase::green) return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    std::random_device randomDevice;
    std::mt19937 engine(randomDevice());
    std::uniform_int_distribution<int> distribution(4, 6);

    int cycleDuration = distribution(engine);

    // need to track time the update actually occurred.  Set it to a time in the past so the update is
    // performed initially without pause.
    std::chrono::system_clock::time_point lastUpdatePerformed = std::chrono::system_clock::now();

    while (true)
    {
        // FP.2a requirement
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // get elapsed time since last update
        long elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastUpdatePerformed).count();
        if(elapsedTime >= cycleDuration)
        {
            // time to change the light color
            if(_currentPhase == red) _currentPhase = green;
            else _currentPhase = red;

            // TODO : sends an update method to the message queue using move semantics
            auto message = _currentPhase;
            messageQueue.send(std::move(message));

            lastUpdatePerformed = std::chrono::system_clock::now();
        }
    }
}
