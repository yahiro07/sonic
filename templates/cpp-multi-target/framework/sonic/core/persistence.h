#pragma once
#include <map>
#include <vector>

namespace sonic {

struct PersistStateData {
  // int parametersVersion;
  std::map<std::string, double> parameters;
  // std::map<std::string, std::string> stateKvs;
};

void serializePersistState(const PersistStateData &data,
                           std::vector<uint8_t> &buffer);

bool deserializePersistState(const std::vector<uint8_t> &buffer,
                             PersistStateData &data);

} // namespace sonic