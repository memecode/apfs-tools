/**
 * Structures and related items as defined in
 * §10 "Data Streams"
 */

#ifndef APFS_STRUCT_DSTREAM_H
#define APFS_STRUCT_DSTREAM_H

#include "j.h"      // for `j_key_t`

/** `j_phys_ext_key_t` **/

typedef struct {
    j_key_t     hdr;
} ATTR_PACK1   j_phys_ext_key_t;

/** `j_phys_ext_val_t` **/

typedef struct {
    uint64_t    len_and_kind;
    uint64_t    owning_obj_id;
    int32_t     refcnt;
} ATTR_PACK1   j_phys_ext_val_t;

#define PEXT_LEN_MASK       0x0fffffffffffffffULL
#define PEXT_KIND_MASK      0xf000000000000000ULL
#define PEXT_KIND_SHIFT     60

/** `j_file_extent_key_t` **/

typedef struct {
    j_key_t     hdr;
    uint64_t    logical_addr;
} ATTR_PACK1   j_file_extent_key_t;

/** `j_file_extent_val_t` **/

typedef struct {
    uint64_t    len_and_flags;
    uint64_t    phys_block_num;
    uint64_t    crypto_id;
} ATTR_PACK1   j_file_extent_val_t;

#define J_FILE_EXTENT_LEN_MASK      0x00ffffffffffffffULL
#define J_FILE_EXTENT_FLAG_MASK     0xff00000000000000ULL
#define J_FILE_EXTENT_FLAG_SHIFT    56

/** `j_dstream_id_key_t` **/

typedef struct {
    j_key_t     hdr;
} ATTR_PACK1   j_dstream_id_key_t;

/** `j_dstream_id_val_t` **/

typedef struct {
    uint32_t    refcnt;
} ATTR_PACK1   j_dstream_id_val_t;

/** `j_dstream_t` --- forward declared for `j_xattr_dstream` **/
#ifdef _WIN32
#pragma pack(push, 8)
#endif
typedef struct {
    uint64_t    size;
    uint64_t    alloced_size;
    uint64_t    default_crypto_id;
    uint64_t    total_bytes_written;
    uint64_t    total_bytes_read;
} ATTR_PACK8	j_dstream_t;
#ifdef _WIN32
#pragma pack(pop)
#endif

/** `j_xattr_dstream` **/

typedef struct {
    uint64_t        xattr_obj_id;
    j_dstream_t     dstream;
} j_xattr_dstream_t;

#endif // APFS_STRUCT_DSTREAM_H
