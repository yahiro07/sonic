#include "./processor_state_helper.h"
#include "sonic/core/persistence.h"
#include <base/source/fstreamer.h>
#include <glaze/glaze.hpp>
#include <pluginterfaces/base/ibstream.h>
#include <sonic/common/logger.h>

namespace sonic_vst {

using namespace sonic;

using namespace Steinberg;

bool processorStateHelper_readState(IBStream *stream, PersistStateData &data) {
  IBStreamer streamer(stream, kLittleEndian);
  uint32_t byteCount = 0;
  streamer.readInt32u(byteCount);
  if (byteCount >= 1000000) {
    printf("State buffer size is too large, %d/%d\n", byteCount, 1000000);
    return false;
  }
  std::vector<uint8_t> buffer(byteCount);
  const auto bytesRead = streamer.readRaw(buffer.data(), byteCount);
  if (bytesRead != byteCount) {
    printf("Failed to read state from stream\n");
    return false;
  }
  return sonic::deserializePersistState(buffer, data);
}

bool processorStateHelper_writeState(IBStream *stream,
                                     const PersistStateData &data) {
  IBStreamer streamer(stream, kLittleEndian);

  std::vector<uint8_t> buffer;
  sonic::serializePersistState(data, buffer);
  uint32_t byteCount = buffer.size();
  if (byteCount >= 1000000) {
    printf("State buffer size is too large, %d/%d\n", byteCount, 1000000);
    return false;
  }
  streamer.writeInt32u(byteCount);

  const auto bytesWritten =
      streamer.writeRaw(buffer.data(), static_cast<int32>(buffer.size()));
  if (bytesWritten != static_cast<int32>(buffer.size())) {
    logger.error("failed writing state bytes: %d/%d",
                 static_cast<int32>(bytesWritten),
                 static_cast<int32>(buffer.size()));
    return false;
  }
  return true;
}

} // namespace sonic_vst