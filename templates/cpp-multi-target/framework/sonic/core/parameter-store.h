#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace sonic {

class IParameterStore {
public:
  virtual ~IParameterStore() = default;
  virtual double get(uint32_t id) = 0;
  virtual void set(uint32_t id, double value) = 0;
};

class VectorParameterStore : public IParameterStore {
private:
  std::vector<float> parameters;

public:
  VectorParameterStore(size_t parameterCount)
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

// todo: avoid using std::unordered_map since its lookup costs is
// non-deterministic, instead use a custom container using std::vector<Entry>
// with sorted by id and binary search for lookup

class UnorderedMapParameterStore : public IParameterStore {
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

class ParameterStore : public IParameterStore {
private:
  std::unique_ptr<IParameterStore> impl;

public:
  void setup(uint32_t maxId) {
    if (maxId < 4096) {
      impl = std::make_unique<VectorParameterStore>(maxId + 1);
    } else {
      // this is not preferred but we use unordered map for sparse and large
      // parameter IDs
      // it is recommended to define dense and small parameter IDs for better
      // performance
      impl = std::make_unique<UnorderedMapParameterStore>();
    }
  }

  double get(uint32_t id) override { return impl->get(id); }

  void set(uint32_t id, double value) override { impl->set(id, value); }
};

} // namespace sonic