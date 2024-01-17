#pragma once

#include <cstdlib>
#include <iostream>
#include <vector>

class State {
public:
  State(std::string name) : name(name) {}
  State(const char* name) : name(name) {}
  const std::string& getName() const { return name; }

private:
  std::string name;
};

class Transition {
private:
  State& _from;
  State& _to;

  std::function<double()> _probability_function;

public:
  Transition(State& from, State& to, std::function<double()> probability_callback) : _from(from), _to(to), _probability_function(probability_callback) {}

  const State& from() const { return _from; }
  const State& to() const { return _to; }
  double probability() const { return _probability_function(); }
};

class ProbabilisticStateMachine {
public:
  ProbabilisticStateMachine(std::map<std::string, State>& states, std::map<std::string, Transition>& transitions, State& init) : _states(states), _transitions(transitions), _currentState(init) {}

  void performTransition() {
    // Choose a transition based on probabilities
    double randomValue = static_cast<double>(rand()) / RAND_MAX;
    double cumulativeProbability = 0.0;

    for (const auto& transition : _transitions) {
      if (transition.second.from().getName() != _currentState.getName()) {
        continue;
      }

      cumulativeProbability += transition.second.probability();
      if (randomValue <= cumulativeProbability) {
        // Transition to the next state
        _currentState = transition.second.to();
        std::cout << "Performed transition from " << transition.second.from().getName() << " to " << transition.second.to().getName() << std::endl;
        break;
      }
    }
    throw std::runtime_error("No transitions from current state");
  }

  const State& getCurrentState() const { return _currentState; }

private:
  State& _currentState;
  std::map<std::string, State>& _states;
  std::map<std::string, Transition>& _transitions;
};
