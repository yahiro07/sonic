#pragma once

#include <string>
#include <unordered_map>

typedef struct {
  int parametersVersion;
  std::unordered_map<std::string, double> parameters;
} ProcessorState;
