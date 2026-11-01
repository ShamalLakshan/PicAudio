#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace adapters {

/**
 * @brief Lock-free Single-Producer Single-Consumer (SPSC) ring buffer.
 *
 * Cacheline-padded atomic indices prevent false sharing between producer
 * and consumer threads. Uses power-of-two sizing for bitwise wrapping.
 *
 * @tparam T Element type (typically uint8_t for raw PCM or float for samples).
 */
template <typename T> class LockFreeRingBuffer {
public:
  /**
   * @brief Constructs a ring buffer with at least the requested capacity.
   * @param requestedCapacity Minimum element capacity (rounded up to next power
   * of 2).
   */
  explicit LockFreeRingBuffer(size_t requestedCapacity = 65536) {
    m_capacity = nextPowerOfTwo(std::max<size_t>(64, requestedCapacity));
    m_mask = m_capacity - 1;
    m_buffer.resize(m_capacity);
    m_writeIndex.store(0, std::memory_order_relaxed);
    m_readIndex.store(0, std::memory_order_relaxed);
  }

  ~LockFreeRingBuffer() = default;

  // Non-copyable, movable
  LockFreeRingBuffer(const LockFreeRingBuffer &) = delete;
  LockFreeRingBuffer &operator=(const LockFreeRingBuffer &) = delete;
  LockFreeRingBuffer(LockFreeRingBuffer &&other) noexcept {
    m_capacity = other.m_capacity;
    m_mask = other.m_mask;
    m_buffer = std::move(other.m_buffer);
    m_writeIndex.store(other.m_writeIndex.load(std::memory_order_relaxed),
                       std::memory_order_relaxed);
    m_readIndex.store(other.m_readIndex.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
  }
  LockFreeRingBuffer &operator=(LockFreeRingBuffer &&other) noexcept {
    if (this != &other) {
      m_capacity = other.m_capacity;
      m_mask = other.m_mask;
      m_buffer = std::move(other.m_buffer);
      m_writeIndex.store(other.m_writeIndex.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
      m_readIndex.store(other.m_readIndex.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
    }
    return *this;
  }

  /**
   * @brief Writes data into the ring buffer.
   * @param source Pointer to data elements.
   * @param count Number of elements to write.
   * @return Actual number of elements written.
   */
  size_t write(const T *source, size_t count) {
    if (!source || count == 0) {
      return 0;
    }

    const size_t currentWrite = m_writeIndex.load(std::memory_order_relaxed);
    const size_t currentRead = m_readIndex.load(std::memory_order_acquire);
    const size_t freeSpace = (m_capacity - 1) - (currentWrite - currentRead);
    const size_t toWrite = std::min(count, freeSpace);

    if (toWrite == 0) {
      return 0;
    }

    const size_t writeOffset = currentWrite & m_mask;
    const size_t firstChunk = std::min(toWrite, m_capacity - writeOffset);
    const size_t secondChunk = toWrite - firstChunk;

    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(&m_buffer[writeOffset], source, firstChunk * sizeof(T));
      if (secondChunk > 0) {
        std::memcpy(&m_buffer[0], source + firstChunk, secondChunk * sizeof(T));
      }
    } else {
      for (size_t i = 0; i < firstChunk; ++i) {
        m_buffer[writeOffset + i] = source[i];
      }
      for (size_t i = 0; i < secondChunk; ++i) {
        m_buffer[i] = source[firstChunk + i];
      }
    }

    m_writeIndex.store(currentWrite + toWrite, std::memory_order_release);
    return toWrite;
  }

  /**
   * @brief Reads data from the ring buffer.
   * @param destination Pointer to buffer receiving data.
   * @param count Maximum number of elements to read.
   * @return Actual number of elements read.
   */
  size_t read(T *destination, size_t count) {
    if (!destination || count == 0) {
      return 0;
    }

    const size_t currentRead = m_readIndex.load(std::memory_order_relaxed);
    const size_t currentWrite = m_writeIndex.load(std::memory_order_acquire);
    const size_t available = currentWrite - currentRead;
    const size_t toRead = std::min(count, available);

    if (toRead == 0) {
      return 0;
    }

    const size_t readOffset = currentRead & m_mask;
    const size_t firstChunk = std::min(toRead, m_capacity - readOffset);
    const size_t secondChunk = toRead - firstChunk;

    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(destination, &m_buffer[readOffset], firstChunk * sizeof(T));
      if (secondChunk > 0) {
        std::memcpy(destination + firstChunk, &m_buffer[0],
                    secondChunk * sizeof(T));
      }
    } else {
      for (size_t i = 0; i < firstChunk; ++i) {
        destination[i] = m_buffer[readOffset + i];
      }
      for (size_t i = 0; i < secondChunk; ++i) {
        destination[firstChunk + i] = m_buffer[i];
      }
    }

    m_readIndex.store(currentRead + toRead, std::memory_order_release);
    return toRead;
  }

  /**
   * @brief Number of elements available for reading.
   */
  [[nodiscard]] size_t availableRead() const {
    const size_t currentRead = m_readIndex.load(std::memory_order_relaxed);
    const size_t currentWrite = m_writeIndex.load(std::memory_order_acquire);
    return currentWrite - currentRead;
  }

  /**
   * @brief Space available for writing before buffer fills.
   */
  [[nodiscard]] size_t availableWrite() const {
    const size_t currentWrite = m_writeIndex.load(std::memory_order_relaxed);
    const size_t currentRead = m_readIndex.load(std::memory_order_acquire);
    return (m_capacity - 1) - (currentWrite - currentRead);
  }

  /**
   * @brief Total capacity of the buffer.
   */
  [[nodiscard]] size_t capacity() const { return m_capacity - 1; }

  /**
   * @brief Buffer fill percentage in range [0.0f, 100.0f].
   */
  [[nodiscard]] float fillPercentage() const {
    const size_t readable = availableRead();
    const size_t cap = capacity();
    if (cap == 0) {
      return 0.0f;
    }
    return (static_cast<float>(readable) / static_cast<float>(cap)) * 100.0f;
  }

  /**
   * @brief Resets read and write pointers to empty state.
   */
  void reset() {
    m_writeIndex.store(0, std::memory_order_relaxed);
    m_readIndex.store(0, std::memory_order_relaxed);
  }

private:
  static size_t nextPowerOfTwo(size_t val) {
    size_t p = 1;
    while (p < val) {
      p <<= 1;
    }
    return p;
  }

  size_t m_capacity{0};
  size_t m_mask{0};
  std::vector<T> m_buffer;

  // Align to separate 64-byte cache lines to eliminate false sharing
  alignas(64) std::atomic<size_t> m_writeIndex{0};
  alignas(64) std::atomic<size_t> m_readIndex{0};
};

} // namespace adapters
