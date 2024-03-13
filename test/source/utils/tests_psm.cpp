#include <doctest/doctest.h>
#include <filestorm/utils/psm.h>

#include <map>

TEST_CASE("Testing Transition class") {
  Transition t(1, 2, "key1");

  SUBCASE("Constructor initializes values correctly") {
    CHECK(t.from() == 1);
    CHECK(t.to() == 2);
    CHECK(t.probability_key() == "key1");
  }
}

TEST_CASE("Testing ProbabilisticStateMachine") {
  std::map<std::string, Transition> transitions = {{"t1", Transition(1, 2, "p1")}, {"t2", Transition(2, 3, "p2")}};
  std::map<std::string, double> probabilities = {
      {"p1", 1.0},  // 100% chance to transition from 1 to 2
      {"p2", 0.0}   // 0% chance to transition from 2 to 3 (for testing)
  };

  ProbabilisticStateMachine psm(transitions, 1);

  SUBCASE("performTransition correctly updates the current state based on probabilities") {
    // Seed rand to make the test deterministic
    srand(123);  // Use a fixed seed for reproducibility

    psm.performTransition(probabilities);
    CHECK(psm.getCurrentState() == 2);

    // Test for a transition that cannot happen due to 0% probability
    try {
      psm.performTransition(probabilities);
      CHECK(false);  // Should not reach here
    } catch (const std::runtime_error& e) {
      CHECK(std::string(e.what()) == "No transitions from current state");
    }
  }

  SUBCASE("getCurrentState returns the correct initial and subsequent states") {
    CHECK(psm.getCurrentState() == 1);  // Initial state

    // Perform a valid transition and check state again
    srand(123);  // Ensure deterministic behavior
    psm.performTransition(probabilities);
    CHECK(psm.getCurrentState() == 2);
  }
}
