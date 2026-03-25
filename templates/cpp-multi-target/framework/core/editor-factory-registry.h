#pragma once

#include "./editor-interfaces.h"
#include <map>

namespace sonic {

class EditorFactoryRegistry {
private:
  EditorFactoryRegistry() = default;
  ~EditorFactoryRegistry() = default;
  std::map<std::string, EditorFactoryFn> factories;

public:
  static EditorFactoryRegistry *getInstance() {
    static EditorFactoryRegistry instance;
    return &instance;
  }

  void registerEditorVariant(std::string variantName,
                             EditorFactoryFn factoryFn) {
    factories[variantName] = factoryFn;
  }

  EditorFactoryFn getEditorFactory(std::string variantName) {
    auto kv = factories.find(variantName);
    if (kv == factories.end()) {
      return nullptr;
    }
    return kv->second;
  }
};
} // namespace sonic