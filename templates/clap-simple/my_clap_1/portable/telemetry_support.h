#pragma once

#include "my_clap_1/portable/interfaces.h"
#include "my_clap_1/wrapper/processor_adapter.h"
#include "sonic_common/advanced_synthesizer.h"
#include <atomic>
#include <map>

struct TelemetryDefinitionItem {
  int id;
  uint32_t count;
};

class TelemetryBuilderImpl : public TelemetryBuilder {
  std::vector<TelemetryDefinitionItem> &telemetryDefinitions;

public:
  TelemetryBuilderImpl(
      std::vector<TelemetryDefinitionItem> &telemetryDefinitions)
      : telemetryDefinitions(telemetryDefinitions) {}

  void defineFloatArray(int id, uint32_t count) override {
    telemetryDefinitions.push_back({.id = id, .count = count});
  }
};

// Lock-free (no mutex): triple-buffer per telemetry item.
// - audio thread writes only into a buffer that is neither "published" nor
// "reading"
// - UI thread reads only from "published" buffer
class TelementryItem {
public:
  int id{};
  uint32_t count{};

private:
  static constexpr uint8_t kNone = 255;

  std::array<std::vector<float>, 3> buffers{};

  std::atomic<uint8_t> published{0};
  std::atomic<uint8_t> reading{kNone};

  // audio-thread only
  uint8_t writeHint{1};

  uint8_t pickWritable(uint8_t pub, uint8_t rd) {
    for (uint8_t k = 0; k < 3; ++k) {
      const uint8_t idx = static_cast<uint8_t>((writeHint + k) % 3);
      if (idx != pub && idx != rd) {
        writeHint = static_cast<uint8_t>((idx + 1) % 3);
        return idx;
      }
    }
    return kNone;
  }

public:
  TelementryItem(int id, uint32_t count) : id(id), count(count) {
    for (auto &buf : buffers) {
      buf.assign(count, 0.0f);
    }
  }
  TelementryItem(const TelementryItem &) = delete;
  TelementryItem &operator=(const TelementryItem &) = delete;

  TelementryItem(TelementryItem &&other) noexcept
      : id(other.id), count(other.count), buffers(std::move(other.buffers)),
        published(other.published.load(std::memory_order_relaxed)),
        reading(other.reading.load(std::memory_order_relaxed)),
        writeHint(other.writeHint) {}

  TelementryItem &operator=(TelementryItem &&other) noexcept {
    if (this == &other)
      return *this;

    id = other.id;
    count = other.count;
    buffers = std::move(other.buffers);
    published.store(other.published.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
    reading.store(other.reading.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
    writeHint = other.writeHint;
    return *this;
  }

  template <class Producer> bool produce(Producer &&producer) {
    const uint8_t pub = published.load(std::memory_order_acquire);
    const uint8_t rd = reading.load(std::memory_order_acquire);

    const uint8_t w = pickWritable(pub, rd);
    if (w == kNone)
      return false;

    producer(buffers[w].data(), count);
    published.store(w, std::memory_order_release);
    return true;
  }

  template <class Consumer> void consume(Consumer &&consumer) {
    const uint8_t idx = published.load(std::memory_order_acquire);

    reading.store(idx, std::memory_order_release);
    consumer(buffers[idx]);
    reading.store(kNone, std::memory_order_release);
  }
};

class TelemetrySupport : public ITelemetrySupport {
private:
  AdvancedSynthesizer &synth;
  ProcessorAdapter &processorAdapter;

  std::vector<TelementryItem> telementryItems;

  std::map<int, std::function<void(int id, std::vector<float> &buffer)>>
      telemetryDataListeners;

  bool activeTelemetryBitFlags = 0;

  std::atomic<int> requestedTelemetryBitFlags{0};
  std::atomic<int> responseTelemetryBitFlags{0};

  void emitTelemetryData(int id, std::vector<float> &buffer) {
    for (auto listener : telemetryDataListeners) {
      listener.second(id, buffer);
    }
  }

  std::function<void(bool, int)> telemetryTimerRequestCallback = nullptr;

public:
  TelemetrySupport(AdvancedSynthesizer &synth,
                   ProcessorAdapter &processorAdapter)
      : synth(synth), processorAdapter(processorAdapter) {}

  int subscribeTelemetryData(
      std::function<void(int id, std::vector<float> &buffer)> callback)
      override {
    auto id = telemetryDataListeners.size() + 1;
    telemetryDataListeners[id] = callback;
    return id;
  }
  void unsubscribeTelemetryData(int subscriptionId) override {
    telemetryDataListeners.erase(subscriptionId);
  }

  void
  setTelemetryTimerRequestCallback(std::function<void(bool, int)> callback) {
    telemetryTimerRequestCallback = callback;
  }

  void setup() {
    std::vector<TelemetryDefinitionItem> telemetryDefinitions;
    auto builder = TelemetryBuilderImpl(telemetryDefinitions);
    synth.setupTelemetries(builder);
    telementryItems.clear();
    telementryItems.reserve(telemetryDefinitions.size());
    for (auto &def : telemetryDefinitions) {
      if (def.id >= 32) {
        printf("Telemetry id must be within 0~31. Invalid id: %d\n", def.id);
        continue;
      }
      TelementryItem item(def.id, def.count);
      telementryItems.push_back(std::move(item));
    }
  }

  // called in audio thread
  void process() {
    auto bitFlags = requestedTelemetryBitFlags.load(std::memory_order_acquire);
    if (bitFlags <= 0)
      return;
    for (auto &item : telementryItems) {
      if ((bitFlags & (1 << item.id)) > 0) {
        item.produce([&](float *buffer, uint32_t count) {
          synth.readTelemetry(item.id, buffer, count);
        });
      }
    }
    responseTelemetryBitFlags.store(bitFlags, std::memory_order_release);
    requestedTelemetryBitFlags.store(0, std::memory_order_release);
  }

  // called in main thread
  void setupPollingTelemetries(int targetBitFlags,
                               int timerIntervalMs) override {
    activeTelemetryBitFlags = targetBitFlags;
    if (telemetryTimerRequestCallback) {
      telemetryTimerRequestCallback(true, timerIntervalMs);
    }
  }

  void stopPollingTelemetries() override {
    activeTelemetryBitFlags = 0;
    if (telemetryTimerRequestCallback) {
      telemetryTimerRequestCallback(false, 0);
    }
  }

  // called in main thread, expected in the loop of 30~60fps
  void updateTelemetries() {
    // emit previous response data to UI
    auto bitFlags = responseTelemetryBitFlags.load(std::memory_order_acquire);
    for (auto &item : telementryItems) {
      if ((bitFlags & (1 << item.id)) > 0) {
        item.consume([&](std::vector<float> &buffer) {
          emitTelemetryData(item.id, buffer);
        });
      }
    }
    responseTelemetryBitFlags.store(0, std::memory_order_release);

    // trigger next fetch request
    if (activeTelemetryBitFlags > 0) {
      requestedTelemetryBitFlags.store(activeTelemetryBitFlags,
                                       std::memory_order_release);
    }
  }
};