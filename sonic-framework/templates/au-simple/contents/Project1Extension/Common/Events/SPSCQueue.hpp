#pragma once
#include <atomic>
#include <stdint.h>

template <typename T, size_t Capacity>

class SPSCQueue {
  static_assert(Capacity >= 2, "Capacity must be >= 2");
  static_assert((Capacity & (Capacity - 1)) == 0,
                "Capacity must be power of two");

private:
  // Swiftからインポートされるヘッダでstd::atomic<uint32_t>を使うとビルドエラーになる
  // Clangのimporterが型を解釈できず落ちている(?)std::atomic_ref<uint32_t>は問題ない
  // std::atomic<uint32_t> readIndex{0};
  // std::atomic<uint32_t> writeIndex{0};
  uint32_t _readIndex = 0;
  uint32_t _writeIndex = 0;
  T buffer[Capacity];

public:
  SPSCQueue() = default;
  ~SPSCQueue() = default;

  bool push(const T &item) noexcept {
    std::atomic_ref<uint32_t> readIndex(_readIndex);
    std::atomic_ref<uint32_t> writeIndex(_writeIndex);

    uint32_t currentWrite = writeIndex.load(std::memory_order_relaxed);
    uint32_t nextWrite = (currentWrite + 1) & (Capacity - 1);

    if (nextWrite == readIndex.load(std::memory_order_acquire)) {
      // Queue is full
      return false;
    }

    buffer[currentWrite] = item;
    writeIndex.store(nextWrite, std::memory_order_release);
    return true;
  }

  bool pop(T &item) noexcept {
    std::atomic_ref<uint32_t> readIndex(_readIndex);
    std::atomic_ref<uint32_t> writeIndex(_writeIndex);

    uint32_t currentRead = readIndex.load(std::memory_order_relaxed);

    if (currentRead == writeIndex.load(std::memory_order_acquire)) {
      // Queue is empty
      return false;
    }

    item = buffer[currentRead];
    readIndex.store((currentRead + 1) & (Capacity - 1),
                    std::memory_order_release);
    return true;
  }
};
