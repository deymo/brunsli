// Copyright (c) Google LLC 2019
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

/* Write bits into a byte array. */

#ifndef BRUNSLI_ENC_WRITE_BITS_H_
#define BRUNSLI_ENC_WRITE_BITS_H_

#include "../common/platform.h"
#include "../common/types.h"

namespace brunsli {

/* This function writes bits into bytes in increasing addresses, and within
   a byte least-significant-bit first.

   The function can write up to 56 bits in one go with WriteBits
   Example: let's assume that 3 bits (Rs below) have been written already:

   BYTE-0     BYTE+1       BYTE+2

   0000 0RRR    0000 0000    0000 0000

   Now, we could write 5 or less bits in MSB by just shifting by 3
   and OR'ing to BYTE-0.

   For n bits, we take the last 5 bits, OR that with high bits in BYTE-0,
   and locate the rest in BYTE+1, BYTE+2, etc. */
BRUNSLI_INLINE void WriteBits(size_t n_bits,
                              uint64_t bits,
                              size_t* BRUNSLI_RESTRICT pos,
                              uint8_t* BRUNSLI_RESTRICT array) {
  BRUNSLI_LOG_DEBUG() << "WriteBits " << std::setw(2) << n_bits << " "
                      << std::hex << std::setw(16) << bits << " " << std::dec
                      << std::setw(10) << *pos << BRUNSLI_ENDL();
  BRUNSLI_DCHECK((bits >> n_bits) == 0);
  BRUNSLI_DCHECK(n_bits <= 56);
#if defined(BRUNSLI_LITTLE_ENDIAN)
  /* This branch of the code can write up to 56 bits at a time,
     7 bits are lost by being perhaps already in *p and at least
     1 bit is needed to initialize the bit-stream ahead (i.e. if 7
     bits are in *p and we write 57 bits, then the next write will
     access a byte that was never initialized). */
  uint8_t* p = &array[*pos >> 3];
  uint64_t v = static_cast<uint64_t>(*p);  /* Zero-extend 8 to 64 bits. */
  v |= bits << (*pos & 7);
  BRUNSLI_UNALIGNED_STORE64LE(p, v);  /* Set some bits. */
  *pos += n_bits;
#else
  /* implicit & 0xFF is assumed for uint8_t arithmetics */
  uint8_t* array_pos = &array[*pos >> 3];
  const size_t bits_reserved_in_first_byte = (*pos & 7);
  bits <<= bits_reserved_in_first_byte;
  *(array_pos++) |= static_cast<uint8_t>(bits);
  for (size_t bits_left_to_write = n_bits + bits_reserved_in_first_byte;
       bits_left_to_write >= 9;
       bits_left_to_write -= 8) {
    bits >>= 8;
    *(array_pos++) = static_cast<uint8_t>(bits);
  }
  *array_pos = 0;
  *pos += n_bits;
#endif
}

BRUNSLI_INLINE void WriteBitsPrepareStorage(
    size_t pos, uint8_t* array) {
  BRUNSLI_LOG_DEBUG() << "WriteBitsPrepareStorage " << std::setw(10) << pos
                      << BRUNSLI_ENDL();
  BRUNSLI_DCHECK((pos & 7) == 0);
  array[pos >> 3] = 0;
}

}  // namespace brunsli

#endif  // BRUNSLI_ENC_WRITE_BITS_H_
