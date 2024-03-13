#pragma once

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

class Transition {
private:
  int _from;
  int _to;

  std::string _probability_key;

public:
  Transition(int from, int to, std::string probability_key) : _from(from), _to(to), _probability_key(probability_key) {}

  int from() const { return _from; }
  int to() const { return _to; }
  std::string probability_key() const { return _probability_key; }
};

class ProbabilisticStateMachine {
public:
  ProbabilisticStateMachine(std::map<std::string, Transition>& transitions, int init) : _transitions(transitions), _currentState(init) {}

  void performTransition(std::map<std::string, double>& probabilities) {
    // Choose a transition based on probabilities
    double randomValue = ((double)rand() / (double)(RAND_MAX));
    double cumulativeProbability = 0.0;

    for (const auto& transition : _transitions) {
      if (transition.second.from() != _currentState) {
        continue;
      }
      cumulativeProbability += probabilities[transition.second.probability_key()];
      if (randomValue <= cumulativeProbability) {
        // Transition to the next state
        _currentState = transition.second.to();
        return;
      }
    }

    throw std::runtime_error("No transitions from current state");
  }

  int getCurrentState() const { return _currentState; }

private:
  std::map<std::string, Transition>& _transitions;
  int _currentState;
};
