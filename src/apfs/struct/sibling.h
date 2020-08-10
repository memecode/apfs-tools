/**
 * Structures and related items as defined in
 * §12 "Siblings"
 */

#ifndef APFS_STRUCT_SIBLING_H
#define APFS_STRUCT_SIBLING_H

// #include <stdint.h>
#include "j.h"      // for `j_key_t`

/** `j_sibling_key_t` **/

typedef struct {
    j_key_t     hdr;
    uint64_t    sibling_id;
} ATTR_PACK1	j_sibling_key_t;

/** `j_sibling_val_t` **/

typedef struct {
    uint64_t    parent_id;
    uint16_t    name_len;
    uint8_t     name[0];
} ATTR_PACK1	j_sibling_val_t;

/** `j_sibling_map_key_t` **/

typedef struct {
    j_key_t     hdr;
} ATTR_PACK1	j_sibling_map_key_t;

/** `j_sibling_map_val_t` **/

typedef struct {
    uint64_t    file_id;
} ATTR_PACK1	j_sibling_map_val_t;

#endif // APFS_STRUCT_SIBLING_H
