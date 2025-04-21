#pragma once

#include <filestorm/ioengines/factory.h>

#include <iostream>  // For debug logging

#define REGISTER_IO_ENGINE(EngineClass)                                                                                                          \
  namespace {                                                                                                                                    \
    struct EngineClass##Registrator {                                                                                                            \
      EngineClass##Registrator() {                                                                                                               \
        IOEngineFactory::instance().registerEngine(#EngineClass, []() -> std::unique_ptr<IOEngine> { return std::make_unique<EngineClass>(); }); \
      }                                                                                                                                          \
    };                                                                                                                                           \
    static EngineClass##Registrator global_##EngineClass##_registrator;                                                                          \
  }
