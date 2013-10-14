/*
* Copyright (C) 2013 Lourival Vieira Neto <lourival.neto@gmail.com>.
* All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

// TODO: verify what is the POSIX header
#include <arpa/inet.h>

#include "bit_util.h"

#define _DEBUG
#ifdef _DEBUG
  // TODO: use this macro instead of printf!!
  #define DEBUG_MSG(...)  printf(__VA_ARGS__)
#else
  #define DEBUG_MSG(...)
#endif

#define RIGHT_OFFSET(offset, length, alignment, overflow)                               \
  (overflow ? 0 : (alignment - (offset % alignment) - length)) 

#define BITS_OFF(length, alignment, overflow)   (alignment - (length - overflow)) 

#define OFLOW_BITS_OFF(alignment, overflow)     (alignment - overflow) 

#define R_MASK(type, bits_off, right_offset)    ((type) -1 >> bits_off << right_offset)

#define OFLOW_R_MASK(buffer_, pos, alignment, overflow_bits_off, net, ntoh)             \
  ((net ? ntoh(buffer_[ pos + 1 ]) : buffer_[ pos + 1 ]) >> overflow_bits_off)

#define READ_BITS(...)  READ_BITS_(__VA_ARGS__)

#define READ_BITS_(buffer, net, type, ntoh, result,                                     \
  alignment, overflow, pos, right_offset, bits_off, oflow_bits_off)                     \
  do {                                                                                  \
    type * buffer_ = (type *) buffer;                                                   \
    type   r_mask  = R_MASK(type, bits_off, right_offset);                              \
    type   result_ = (net ? ntoh(buffer_[ pos ]) : buffer_[ pos ]) & r_mask;            \
    result_ = overflow ?                                                                \
      (result_ << overflow) | OFLOW_R_MASK(buffer_, pos, alignment, oflow_bits_off,     \
          net, ntoh) :                                                                  \
      result_ >> right_offset;                                                          \
    result = (int64_t) result_;                                                         \
    printf("[%s:%s:%d]: overflow: %d, right_offset: %d, bits_off = %d,"                 \
    " pos: %d, r_mask: %X\n", __FUNCTION__, __FILE__, __LINE__,                         \
      overflow, right_offset, bits_off, pos, (uint32_t)r_mask);                         \
  } while(0)

#define OFLOW_W_MASK(type, overflow_bits_off, value_)                                \
  (~((type) -1 << overflow_bits_off) | (value_ << overflow_bits_off))

#define CLR_MASK(type, bits_off, right_offset)  (~R_MASK(type, bits_off, right_offset))

#define W_MASK(value_, right_offset, overflow) (value_ << right_offset >> overflow)

#define WRITE_BITS(...) WRITE_BITS_(__VA_ARGS__)

#define WRITE_BITS_(buffer, net, type, ntoh, hton, values,                              \
  alignment, overflow, pos, right_offset, bits_off, overflow_bits_off)                  \
  do {                                                                                  \
    type * buffer_  = (type *) buffer;                                                  \
    type   value_   = (type) value;                                                     \
    type   clr_mask = CLR_MASK(type, bits_off, right_offset);                           \
    type   w_mask   = W_MASK(value_, right_offset, overflow);                           \
    type   result   = (net ? ntoh(buffer_[ pos ]) : buffer_[ pos ]) & clr_mask | w_mask;\
    buffer_[ pos ]  = net ? hton(result) : result;                                      \
    if (overflow) {                                                                     \
      result = (net ? ntoh(buffer_[ pos + 1 ]) : buffer_[ pos + 1 ]) &                  \
        OFLOW_W_MASK(type, overflow_bits_off, value_);                                  \
      buffer_[ pos + 1 ] = net ? hton(result) : result;                                 \
    }                                                                                   \
    printf("[%s:%s:%d]: overflow: %d, right_offset: %d, bits_off: %d, pos: %d, "        \
    "clr_mask: %X, buffer[ pos ]: %#X, buffer[ pos + 1 ]: %#X, result: %#X, "           \
    "value: %#X\n",__FUNCTION__, __FILE__, __LINE__, overflow, right_offset, bits_off,  \
    pos, (uint32_t)clr_mask, (int) buffer_[ pos ], (int) buffer_[ pos + 1 ],            \
    (int) result, (int) value);                                                         \
  } while(0)


typedef enum {
  ALIGN_8  =  8,
  ALIGN_16 = 16,
  ALIGN_32 = 32,
  ALIGN_64 = 64,
} align_t;

static align_t adjust_alignment(uint8_t length);

#define nop(x)  x

#define DECLARE_AUXILIARY(offset, length)                                               \
  align_t  alignment      = adjust_alignment(length);                                   \
  uint32_t pos            = offset / alignment;                                         \
  uint32_t align_limit    = alignment * (pos + 1);                                      \
  uint32_t last_bit       = offset + length - 1;                                        \
  uint8_t  overflow       = last_bit >= align_limit ? (last_bit % align_limit) + 1 : 0; \
  uint8_t  bits_off       = BITS_OFF(length, alignment, overflow);                      \
  uint8_t  right_offset   = RIGHT_OFFSET(offset, length, alignment, overflow);          \
  uint8_t  oflow_bits_off = OFLOW_BITS_OFF(alignment, overflow)

#define AUXILIARY   alignment, overflow, pos, right_offset, bits_off, oflow_bits_off

// TODO: check limit!!!
// if pos >= size then goto err ... if overflow then if pos + 1 >= size
int64_t get_bits(void * buffer, uint32_t offset, uint8_t length, bool net)
{
  DECLARE_AUXILIARY(offset, length);
  int64_t result = 0;

  switch (alignment) {
    case ALIGN_8:
      READ_BITS(buffer, net, uint8_t, nop, result, AUXILIARY);
      break;
    case ALIGN_16:
      READ_BITS(buffer, net, uint16_t, ntohs, result, AUXILIARY);
      break;
    case ALIGN_32:
      READ_BITS(buffer, net, uint32_t, ntohl, result, AUXILIARY);
      break;
    case ALIGN_64:
      // TODO: implement ntohll and htonll
      READ_BITS(buffer, net, uint64_t, nop, result, AUXILIARY);
      break;
  }
#ifdef _DEBUG
  printf("[%s:%s:%d]: result: %#llx\n",
      __FUNCTION__, __FILE__, __LINE__, (long long unsigned int) result);
#endif
  return result;
}

void set_bits(void * buffer, uint32_t offset, uint8_t length, bool net, int64_t value)
{
  DECLARE_AUXILIARY(offset, length);

  switch (alignment) {
    case 8:
      WRITE_BITS(buffer, net, uint8_t, nop, nop, value, AUXILIARY); 
      break;
    case 16:
      WRITE_BITS(buffer, net, uint16_t, ntohs, htons, value, AUXILIARY);
      break;
    case 32:
      WRITE_BITS(buffer, net, uint32_t, ntohl, htonl, value, AUXILIARY);
      break;
    case 64:
      // TODO: implement ntohll and htonll
      WRITE_BITS(buffer, net, uint64_t, nop, nop, value, AUXILIARY);
      break;
  }
}

/* static section */

static align_t adjust_alignment(uint8_t length)
{
  uint8_t alignment;
  if (length <= 8)
    alignment = ALIGN_8;
  else if (length <= 16)
    alignment = ALIGN_16;
  else if (length <= 32)
    alignment = ALIGN_32;
  else /* assertion: length > 32 */
    alignment = ALIGN_64;
  return alignment;
}

/* end of module */
