#ifndef TINFL_H
#define TINFL_H
#include <stdint.h>
#include <stddef.h>

#define TINFL_FLAG_PARSE_ZLIB_HEADER 1
#define TINFL_FLAG_HAS_MORE_INPUT 2
#define TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF 4
#define TINFL_FLAG_COMPUTE_ADLER32 8

size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);
#endif
