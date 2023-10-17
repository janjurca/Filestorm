#include <filestorm/scenario.h>
#include <fmt/format.h>

Scenario::Scenario(std::string name) : _name(name) {}

Scenario::~Scenario() {}

std::string Scenario::name() const { return _name; }