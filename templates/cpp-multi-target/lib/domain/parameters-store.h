#pragma once
#include "./interfaces.h"
#include <cstdint>

namespace sonic {

class VectorParametersStore : public IParametersStore {
private:
  std::vector<float> parameters;

public:
  VectorParametersStore(size_t parameterCount)
      : parameters(parameterCount, 0.0) {}

  double get(uint32_t id) override {
    if (id < parameters.size()) {
      return parameters[id];
    }
    return 0.0;
  }
  void set(uint32_t id, double value) override {
    if (id < parameters.size()) {
      parameters[id] = value;
    }
  }
};

class UnorderedMapParametersStore : public IParametersStore {
private:
  std::unordered_map<uint32_t, double> parameters;

public:
  double get(uint32_t id) override {
    auto it = parameters.find(id);
    if (it != parameters.end()) {
      return it->second;
    }
    return 0.0;
  }
  void set(uint32_t id, double value) override { parameters[id] = value; }
};

class ParametersStore : public IParametersStore {
private:
  std::unique_ptr<IParametersStore> impl;

public:
  void setup(uint32_t maxId) {
    if (maxId < 4096) {
      impl = std::make_unique<VectorParametersStore>(maxId + 1);
    } else {
      // this is not preferred but we use unordered map for sparse and large
      // parameter IDs
      // it is recommended to define dense and small parameter IDs for better
      // performance
      impl = std::make_unique<UnorderedMapParametersStore>();
    }
  }

  double get(uint32_t id) override { return impl->get(id); }

  void set(uint32_t id, double value) override { impl->set(id, value); }
};

} // namespace sonic