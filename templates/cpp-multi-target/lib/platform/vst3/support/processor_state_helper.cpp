#include "./processor_state_helper.h"
#include "../../../common/logger.h"
#include <base/source/fstreamer.h>
#include <glaze/glaze.hpp>
#include <pluginterfaces/base/ibstream.h>

namespace sonic_vst {

using namespace sonic;

using namespace Steinberg;

static constexpr uint32_t kStateMagic = 0x534F4E43; // 'S''O''N''C'
static constexpr int32_t kMaxStateBytes = 1024 * 1024;

class ProcessorStateReader {
  static bool readStateToJsonString(IBStream *state, std::string &jsonStr) {
    IBStreamer streamer(state, kLittleEndian);

    int32_t firstWord = 0;
    if (!streamer.readInt32(firstWord))
      return false;
    if (firstWord != kStateMagic)
      return false;

    int32_t size = 0;
    if (!streamer.readInt32(size))
      return false;

    if (size < 0 || size > kMaxStateBytes) {
      logger.error("invalid state size: %d", size);
      return false;
    }

    if (size == 0) {
      logger.log("empty state (size=0)");
      return true;
    }

    jsonStr.resize(static_cast<size_t>(size));
    const auto bytesRead = streamer.readRaw(jsonStr.data(), size);
    if (bytesRead != size) {
      logger.error("failed reading state bytes: %d/%d",
                   static_cast<int32>(bytesRead), size);
      return false;
    }
    return true;
  }

  static bool readProcessorStateFromJson(ProcessorState &processorState,
                                         const std::string &jsonStr) {
    auto ec = glz::read_jsonc(processorState, jsonStr);
    if (ec) {
      logger.error("error reading json: %s",
                   glz::format_error(ec, jsonStr).c_str());
      return false;
    }
    return true;
  }

public:
  static bool readState(IBStream *state, ProcessorState &processorState) {
    std::string jsonStr;
    auto ok = readStateToJsonString(state, jsonStr);
    if (!ok) {
      return false;
    }
    logger.log("jsonStr: %s", jsonStr.c_str());
    //
    return readProcessorStateFromJson(processorState, jsonStr);
  }
};

class ProcessorStateWriter {

  static bool writeJsonStringToVstState(IBStream *state, std::string &jsonStr) {
    constexpr int32 kStateMagic = 0x534F4E43; // 'S''O''N''C'
    constexpr int32 kStateVersion = 1;
    IBStreamer streamer(state, kLittleEndian);
    streamer.writeInt32(kStateMagic);
    streamer.writeInt32(static_cast<int32>(jsonStr.size()));
    if (!jsonStr.empty()) {
      const auto bytesWritten =
          streamer.writeRaw(jsonStr.data(), static_cast<int32>(jsonStr.size()));
      if (bytesWritten != static_cast<int32>(jsonStr.size())) {
        logger.error("failed writing state bytes: %d/%d",
                     static_cast<int32>(bytesWritten),
                     static_cast<int32>(jsonStr.size()));
        return false;
      }
    }
    return true;
  }

  static bool writeProcessorStateToJson(const ProcessorState &processorState,
                                        std::string &jsonStr) {
    auto ec = glz::write_jsonc(processorState, jsonStr);
    if (ec) {
      logger.log("error writing json: %s",
                 glz::format_error(ec, jsonStr).c_str());
      return false;
    }
    return true;
  }

public:
  static bool writeState(IBStream *state,
                         const ProcessorState &processorState) {
    std::string jsonStr;
    auto ok = writeProcessorStateToJson(processorState, jsonStr);
    if (!ok)
      return false;
    logger.log("jsonStr: %s", jsonStr.c_str());
    return writeJsonStringToVstState(state, jsonStr);
  }
};

bool processorStateHelper_readState(Steinberg::IBStream *state,
                                    ProcessorState &processorState) {
  return ProcessorStateReader::readState(state, processorState);
}

bool processorStateHelper_writeState(Steinberg::IBStream *state,
                                     const ProcessorState &processorState) {
  return ProcessorStateWriter::writeState(state, processorState);
}

} // namespace sonic_vst