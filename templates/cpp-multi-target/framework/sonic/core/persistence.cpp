#include "./persistence.h"
#include <cstdio>
#include <glaze/glaze.hpp>

namespace sonic {

void serializePersistState(const PersistStateData &data,
                           std::vector<uint8_t> &buffer) {
  auto res = glz::write<glz::opts{.prettify = true}>(data);
  if (res) {
    const auto &str = res.value();
    // printf("Serializing persist state, json: %.*s\n", (int)str.size(),
    //        str.data());
    buffer.assign(reinterpret_cast<const uint8_t *>(str.data()),
                  reinterpret_cast<const uint8_t *>(str.data() + str.size()));
  } else {
    printf("Failed to serialize persist state\n");
  }
}
bool deserializePersistState(const std::vector<uint8_t> &buffer,
                             PersistStateData &data) {
  std::string_view sv(reinterpret_cast<const char *>(buffer.data()),
                      buffer.size());
  // printf("Deserializing persist state, json: %.*s\n", (int)sv.size(),
  //        sv.data());
  auto _data = glz::read_json<PersistStateData>(sv);
  if (_data) {
    data = _data.value();
    return true;
  } else {
    printf("Failed to deserialize persist state\n");
    return false;
  }
}

} // namespace sonic