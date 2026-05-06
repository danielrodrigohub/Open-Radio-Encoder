#include "ringbuffer.h"
namespace ore {
void rb_init(RingBuffer*, size_t) {}
void rb_free(RingBuffer*) {}
int rb_write(RingBuffer*, const float*, size_t) { return 0; }
int rb_read(RingBuffer*, float*, size_t) { return 0; }
size_t rb_filled(const RingBuffer*) { return 0; }
void rb_clear(RingBuffer*) {}
} // namespace ore
