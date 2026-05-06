#pragma once
#include <cstddef>
namespace ore {
struct RingBuffer { float* data; size_t capacity; size_t readPos; size_t writePos; };
void rb_init(RingBuffer* rb, size_t size);
void rb_free(RingBuffer* rb);
int rb_write(RingBuffer* rb, const float* data, size_t len);
int rb_read(RingBuffer* rb, float* data, size_t len);
size_t rb_filled(const RingBuffer* rb);
void rb_clear(RingBuffer* rb);
} // namespace ore
