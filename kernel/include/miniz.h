#ifndef MINIZ_H
#define MINIZ_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Types de base pour l'API
typedef unsigned char mz_uint8;
typedef signed short mz_int16;
typedef unsigned short mz_uint16;
typedef unsigned int mz_uint32;
typedef unsigned int mz_uint;
typedef uint64_t tinfl_bit_buf_t;

// Constantes
#define TINFL_FAST_LOOKUP_BITS 10
#define TINFL_FAST_LOOKUP_SIZE (1 << TINFL_FAST_LOOKUP_BITS)
#define TINFL_LZ_DICT_SIZE 32768
#define TINFL_DECOMPRESS_MEM_TO_MEM_FAILED ((size_t)-1)

typedef int (*tinfl_put_buf_func_ptr)(const void *pBuf, int len, void *pUser);

// Status codes
typedef enum {
    TINFL_STATUS_BAD_PARAM = -3,
    TINFL_STATUS_ADLER32_MISMATCH = -2,
    TINFL_STATUS_FAILED = -1,
    TINFL_STATUS_DONE = 0,
    TINFL_STATUS_NEEDS_MORE_INPUT = 1,
    TINFL_STATUS_HAS_MORE_OUTPUT = 2,
    TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS = -4
} tinfl_status;

// Flags
#define TINFL_FLAG_HAS_MORE_INPUT 0x01
#define TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF 0x02
#define TINFL_FLAG_PARSE_ZLIB_HEADER 0x04
#define TINFL_FLAG_COMPUTE_ADLER32 0x08

// Structure du décompresseur
typedef struct {
    mz_uint32 m_state, m_num_bits, m_zhdr0, m_zhdr1, m_z_adler32, m_final, m_type, m_check_adler32, m_dist, m_len, m_counter, m_num_extra;
    mz_uint8 m_raw_header[4]; 
    size_t m_dist_from_out_buf_start; 
    tinfl_bit_buf_t m_bit_buf;
    mz_uint8 m_len_codes[288 + 32 + 137];
    mz_uint8 m_code_size_0[288], m_code_size_1[32], m_code_size_2[19];
    mz_int16 m_look_up[3][TINFL_FAST_LOOKUP_SIZE];
    mz_int16 m_tree_0[288 * 2], m_tree_1[32 * 2], m_tree_2[19 * 2];
    mz_uint m_table_sizes[3];
} tinfl_decompressor;

// Prototypes
#define tinfl_init(r) do { (r)->m_state = 0; } while(0)
#define tinfl_get_adler32(r) (r)->m_z_adler32

tinfl_status tinfl_decompress(tinfl_decompressor *r, const mz_uint8 *pIn_buf_next, size_t *pIn_buf_size, mz_uint8 *pOut_buf_start, mz_uint8 *pOut_buf_next, size_t *pOut_buf_size, const mz_uint32 decomp_flags);

#endif
