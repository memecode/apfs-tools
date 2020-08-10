/**
 * Structures and related items as defined in
 * §13 "Snapshot Metadata"
 */

#ifndef APFS_STRUCT_SNAP_H
#define APFS_STRUCT_SNAP_H

#include "j.h"          // for `j_key_t`
#include "object.h"     // for `oid_t`, `xid_t`

/** `j_snap_metadata_key_t` **/

typedef struct {
    j_key_t     hdr;
} ATTR_PACK1	j_snap_metadata_key_t;

/** `j_snap_metadata_val_t` **/

typedef struct {
    oid_t       extentref_tree_oid;
    oid_t       sblock_oid;
    uint64_t    create_time;
    uint64_t    change_time;
    uint64_t    inum;
    uint32_t    extentref_tree_type;
    uint32_t    flags;
    uint16_t    name_len;
    uint8_t     name[0];
} ATTR_PACK1	j_snap_metadata_val_t;

/** `j_snap_name_key_t` **/

typedef struct {
    j_key_t     hdr;
    uint16_t    name_len;
    uint8_t     name[0];
} ATTR_PACK1	j_snap_name_key_t;

/** `j_snap_name_val_t` **/

typedef struct {
    xid_t   snap_xid;
} ATTR_PACK1	j_snap_name_val_t;

/** `snap_meta_flags` **/

typedef enum {
    SNAP_META_PENDING_DATALESS  = 0x00000001,
} snap_meta_flags;

#endif // APFS_STRUCT_SNAP_H
