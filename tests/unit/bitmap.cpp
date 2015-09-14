#include <stdint.h>
#include <stdlib.h>

#include "gtest.h"

#include "bitmap.hh"
#include "sentinelmap.hh"

// FIXME: use the ones defined in sentinalmap
enum { WORDBYTES = sizeof(intptr_t) };
enum { WORDBITS  = sizeof(intptr_t) * 8 };

TEST(BitmapTest, SetGet) {
  const int NTRIALS = 1000;

  for (int n=100; n < 10000; n *= 2) {
    bitmap b;

    // FIXME: is this calculation right?
    size_t size = sentinelmap::getBytes(n);
    void *buf = calloc(1, size);
    ASSERT_NE(buf, nullptr);

    b.initialize(buf, n, size);

    for (int k = 0; k < NTRIALS; k++) {
      // Generate a random stream of bits.
      int *rnd = (int *)calloc(n, sizeof(int));
      ASSERT_NE(rnd, nullptr);

      for (int i=0; i < n; i++) {
        rnd[i] = lrand48() % 2;
      }

      for (int i=0; i < n; i++) {
        if (rnd[i] == 0) {
          bool wasSet = b.checkSetBit(i);
          ASSERT_TRUE(wasSet);
        } else {
          b.clearBit(i);
        }
      }

      for (int i=0; i < n; i++) {
        if (rnd[i] == 0) {
          ASSERT_TRUE(b.isBitSet(i));
          b.clearBit(i);
        } else {
          ASSERT_FALSE(b.isBitSet(i));
        }
      }
      free(rnd);
    }
    free(buf);
  }
}
