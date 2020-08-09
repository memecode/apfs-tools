/**
 * A set of functions used to interact with APFS B-trees.
 */

#ifndef APFS_FUNC_BTREE_H
#define APFS_FUNC_BTREE_H

#include <stdbool.h>

#include "../struct/general.h"
#include "../struct/btree.h"
#include "../struct/j.h"
#include "../io.h"

#include "../string/omap.h"
#include "../string/j.h"

/**
 * Get the latest version of an object, up to a given XID, from an object map
 * B-tree that uses Physical OIDs to refer to its child nodes.
 * 
 * root_node:   A pointer to the root node of an object map B-tree that uses
 *      Physical OIDs to refer to its child nodes.
 *      It is the caller's responsibility to ensure that `root_node` satisfies
 *      these criteria; if it does not, behaviour is undefined.
 * 
 * oid:         The OID of the object search for in the object map B-tree
 *      `root_node`. That is, the result returned will pertain to an object
 *      found in the object map that has the given OID.
 * 
 * max_xid:     The highest XID to consider for the given OID. That is, the
 *      result returned will pertain to the single object found in the object
 *      map that has the given OID, and also whose XID is the highest among all
 *      objects in the object map that have the same OID but whose XIDs do not
 *      exceed `max_xid`.
 * 
 * RETURN VALUE:
 *      A pointer to an object map value corresponding to the unique object
 *      whose OID and XID satisfy the criteria described above for the
 *      parameters `oid` and `max_xid`. If no object exists with the given OID,
 *      or an error occurs, a NULL pointer is returned.
 *      This pointer must be freed when it is no longer needed.
 */
omap_val_t* get_btree_phys_omap_val(btree_node_phys_t* root_node, oid_t oid, xid_t max_xid) {
    // Create a copy of the root node to use as the current node we're working with
    bool debug = oid == 295949;
    btree_info_t* bt_info = NULL;
    btree_node_phys_t* node = malloc(nx_block_size);

    if (!node) {
        fprintf(stderr, "\nERROR: get_btree_phys_omap_val: Could not allocate sufficient memory for `node`.\n");
        return NULL;
    }
    memcpy(node, root_node, nx_block_size);

    // Pointers to areas of the node
    char* toc_start = (char*)(node->btn_data) + node->btn_table_space.off;
    char* key_start = toc_start + node->btn_table_space.len;
    char* val_end   = (char*)node + nx_block_size - sizeof(btree_info_t);

    // We'll need access to the B-tree info after discarding our copy of the root node
    bt_info = malloc(sizeof(btree_info_t));
    if (!bt_info) {
        fprintf(stderr, "\nABORT: get_btree_phys_omap_val: Could not allocate sufficient memory for `bt_info`.\n");
        goto onError;
    }
    memcpy(bt_info, val_end, sizeof(btree_info_t));

    // Descend the B-tree to find the target key–value pair
    while (true) {
        if (!(node->btn_flags & BTNODE_FIXED_KV_SIZE)) {
            fprintf(stderr, "\nget_btree_phys_omap_val: Object map B-trees don't have variable size keys and values ... do they?\n");
            goto onError;
        }

        // TOC entries are instances of `kvoff_t`
        kvoff_t* toc_entry = toc_start;
        if (debug) printf("\t%s:%i - toc_entry->v=%i %lld\n", __func__, __LINE__, toc_entry->v, oid);

        // Find the correct TOC entry, i.e. the last TOC entry whose:
        // - OID doesn't exceed the given OID; or
        // - OID matches the given OID, and XID doesn't exceed the given XID
        uint32_t i;
        for (i = 0;    i < node->btn_nkeys;    i++, toc_entry++) {
            omap_key_t* key = key_start + toc_entry->k;
            if (debug) printf("\t%s:%i - key->ok_oid=%lli oid=%lld\n", __func__, __LINE__, key->ok_oid, oid);
            if (key->ok_oid > oid) {
                toc_entry--;
                break;
            }
            if (key->ok_oid == oid) {
                if (key->ok_xid > max_xid) {
                    toc_entry--;
                    break;
                }
                if (key->ok_xid == max_xid) {
                    break;
                }
            }
        }

        // `toc_entry` now points to the correct TOC entry to use; or
        // it points before `toc_start` if the desired (OID, XID) pair
        // does not exist in this B-tree.
        if ((char*)toc_entry < toc_start)
            goto onError;

        // If this is a leaf node, return the object map value
        if (node->btn_flags & BTNODE_LEAF) {
            // If the object doesn't have the specified OID, then no sufficient
            // object with that OID exists in the B-tree.
            omap_key_t* key = key_start + toc_entry->k;
            if (key->ok_oid != oid)
                goto onError;

            omap_val_t* val = val_end - toc_entry->v;

            omap_val_t* return_val = malloc(sizeof(omap_val_t));
            if (!return_val) {
                fprintf(stderr, "\nABORT: get_btree_phys_omap_val: Could not allocate sufficient memory for `return_val`.\n");
                goto onError;
            }
            memcpy(return_val, val, sizeof(omap_val_t));
            
            free(bt_info);
            free(node);
            return return_val;
        }

        if (i >= node->btn_nkeys)
            toc_entry--;

        // Else, read the corresponding child node into memory and loop
        paddr_t* child_node_addr = val_end - toc_entry->v;
        size_t result;

        if (debug)
            printf("    toc_entry->v=%i Node<=%llu,0x%llx\n", toc_entry->v, *child_node_addr, *child_node_addr);
        if ((result = read_blocks(node, *child_node_addr, 1)) != 1) {
            fprintf(stderr, "ABORT: get_btree_phys_omap_val: Failed to read block 0x%llx (%i).\n", *child_node_addr, (int)result);
            goto onError;
        }

        if (!is_cksum_valid(node)) {
            fprintf(stderr, "ABORT: get_btree_phys_omap_val: Checksum of node at block 0x%llx did not validate.\n", *child_node_addr);
            goto onError;
        }

        toc_start = (char*)(node->btn_data) + node->btn_table_space.off;
        key_start = toc_start + node->btn_table_space.len;
        val_end   = (char*)node + nx_block_size;    // Always dealing with non-root node here
    }

onError:
    free(bt_info);
    free(node);
    return NULL;
}

/**
 * Custom data structure used to store a full file-system record (i.e. a single
 * key–value pair from a file-system root tree) alongside each other for easier
 * data access and manipulation.
 * 
 * One can make use of an instance of this datatype by determining the strctures
 * contained within its `data` field by appealing to the `obj_id_and_type` field
 * of the `j_key_t` structure for the record, which is guaranteed to exist and
 * start at `data[0]`. That is, a pointer to this instance of `j_key_t` can be
 * obtained with `j_key_t* record_header = record->data`, where `record` is an
 * instance of this type, `j_rec_t`.
 * 
 * key_len: Length of the file-system record's key-part, in bytes.
 * 
 * val_len: Length of the file-system record's value-part, in bytes.
 * 
 * data:    Array of `key_len + val_len` bytes of data, of which,
 *          index 0 (inclusive) through `key_len` (exclusive) contain the
 *          key-part data, and index `key_len` (inclusive) through
 *          `key_len + val_len` (exclusive) contain the value-part data.
 */
typedef struct {
    uint16_t    key_len;
    uint16_t    val_len;
    char        data[];
} j_rec_t;

/**
 * Free memory allocated for a file-system records array that
 * was created by a call to `get_btree_virt_fs_records()`.
 * 
 * records_array:   A pointer to an array of pointers to instances of `j_rec_t`,
 *                  as returned by a call to `get_btree_virt_fs_records()`.
 */
void free_j_rec_array(j_rec_t** records_array) {
    if (!records_array) {
        return;
    }

    for (j_rec_t** cursor = records_array; *cursor; cursor++) {
        free(*cursor);
    }
    free(records_array);
}

/**
 * Get an array of all the file-system records with a given Virtual OID from a
 * given file-system root tree.
 * 
 * vol_omap_root_node:
 *      A pointer to the root node of the object map tree of the APFS volume
 *      which the given file-system root tree belongs to. This is needed in
 *      order to resolve the Virtual OIDs of objects listed in the file-system
 *      root tree to their respective block addresses within the APFS container,
 *      so that we can actually find the structures on disk.
 * 
 * vol_fs_root_node:
 *      A pointer to the root node of the file-system root tree.
 * 
 * oid:
 *      The Virtual OID of the desired records to fetch.
 * 
 * max_xid:
 *      The maximum XID to consider for a file-system object/entry. That is,
 *      no object whose XID exceeds this value will be present in the array
 *      of file-system rescords returned by this function.
 * 
 * RETURN VALUE:
 *      A pointer to the head of an array of pointers to instances of `j_rec_t`.
 *      Each such instance of `j_rec_t` describes a file-system record relating
 *      to the file-system object whose Virtual OID is `oid`. The length of this
 *      array is not explicitly returned to the caller, but the last pointer in
 *      the array returned will be a NULL pointer.
 * 
 *      When the data in the array is no longer needed, the pointer that was
 *      returned by this function should be passed to `free_j_rec_array()`
 *      in order to free the memory allocated by this function via internal
 *      calls to `malloc()` and `realloc()`.
 */
j_rec_t** get_fs_records(btree_node_phys_t* vol_omap_root_node, btree_node_phys_t* vol_fs_root_node, oid_t oid, xid_t max_xid) {
    /**
     * `desc_path` describes the path we have taken to descend down the file-
     * system root tree. The value of `desc_path[i]` is the index of the key
     * chosen `i` levels beneath the root level, out of the keys within the
     * node that key was chosen from.
     *
     * Since these B+ trees do not container pointers to their siblings, this
     * info is needed in order to easily walk the tree after we find the first
     * record with the given OID.
     */
    uint32_t desc_path[vol_fs_root_node->btn_level + 1];
    uint16_t i = 0;     // `i` keeps tracks of how many descents we've made from the root node.
    btree_node_phys_t* node = NULL;
    j_rec_t** records = NULL;
    btree_info_t* bt_info = NULL;
    bool debug = oid == 3307851;

    // Initialise the array of records which will be returned to the caller
    size_t num_records = 0;
    records = malloc(sizeof(j_rec_t*));
    if (!records) {
        fprintf(stderr, "\nABORT: get_fs_records: Could not allocate sufficient memory for `records`.\n");
        goto onFatal;
    }
    records[0] = NULL;

    // Create a copy of the root node to use as the current node we're working with
    node = malloc(nx_block_size);
    if (!node) {
        fprintf(stderr, "\nABORT: get_fs_records: Could not allocate sufficient memory for `node`.\n");
        goto onFatal;
    }
    memcpy(node, vol_fs_root_node, nx_block_size);

    // Pointers to areas of the node
    char* toc_start = (char*)(node->btn_data) + node->btn_table_space.off;
    char* key_start = toc_start + node->btn_table_space.len;
    char* val_end   = (char*)node + nx_block_size - sizeof(btree_info_t);

    // We may need access to the B-tree info after discarding our copy of the root node
    bt_info = malloc(sizeof(btree_info_t));
    if (!bt_info) {
        fprintf(stderr, "\nABORT: get_fs_records: Could not allocate sufficient memory for `bt_info`.\n");
        goto onFatal;
    }
    memcpy(bt_info, val_end, sizeof(btree_info_t));

    // Find the first (leftmost/least) record in the tree with the given OID
    while (true) {
        if (node->btn_flags & BTNODE_FIXED_KV_SIZE) {
            fprintf(stderr, "\nget_fs_records: File-system root B-trees don't have fixed size keys and values ... do they?\n");
            goto onFatal;
        }

        // TOC entries are instances of `kvloc_t`
        kvloc_t* toc_entry = toc_start;

        // Determine which entry in this node we should descend
        for (desc_path[i] = 0;    desc_path[i] < node->btn_nkeys;    desc_path[i]++, toc_entry++) {
            j_key_t* key = key_start + toc_entry->k.off;
            oid_t record_oid = key->obj_id_and_type & OBJ_ID_MASK;

            if (debug) printf("\t%s:%i - [%i][%i] record_oid=%lli oid=%lld toc_entry->v.off=%i\n",
                __func__, __LINE__, i, desc_path[i], record_oid, oid, toc_entry->v.off);
            if (record_oid == oid) {
                if (node->btn_flags & BTNODE_LEAF) {
                    // This is the first entry in this leaf node with the given
                    // OID, and thus the first record in the whole tree with
                    // the given OID
                    if (debug) printf("\t%s:%i - break, got leaf\n", __func__, __LINE__);
                    break;
                }

                // A record with the given OID may exist as a descendant of the
                // previous entry; backtrack so we descend that entry ...
                if (desc_path[i] != 0) {
                    desc_path[i]--;
                    toc_entry--;
                }
                // ... unless this is the first entry in the node, in which
                // case, backtracking is impossible, and this is the entry we
                // should descend
                if (debug) printf("\t%s:%i - break\n", __func__, __LINE__);
                break;
            }

            if (record_oid > oid) {
                if (node->btn_flags & BTNODE_LEAF) {
                    // If a record with the given OID exists in this leaf node,
                    // we would've encountered it by now. Hence, this leaf node
                    // contains no entries with the given OID, and so no record
                    // with the given OID exists in the whole tree
                    goto onFatal;
                }

                // We just passed the entry we want to descend; backtrack
                desc_path[i]--;
                toc_entry--;
                break;
            }
        }

        // `toc_entry` now points to the correct entry to descend; or
        // it points before `toc_start` if no records with the
        // desired OID exist in this B-tree.
        if ((char*)toc_entry < toc_start) {
            goto onFatal;
        }

        // If this is a leaf node, then it contains the first record with the
        // given OID, and `desc_path` desribes the path taken to reach this
        // node. Break from the while-loop so that we can walk along the tree
        // to get the rest of the records with the given OID.
        if (node->btn_flags & BTNODE_LEAF) {
            break;
        }

        if (desc_path[i] >= node->btn_nkeys) {
            toc_entry--;
        }

        // Else, read the corresponding child node into memory and loop
        oid_t* child_node_virt_oid = val_end - toc_entry->v.off;
        if (debug) printf("\t%s:%i - *child_node_virt_oid=%lli toc_entry->v.off=%i\n", 
            __func__, __LINE__, *child_node_virt_oid, toc_entry->v.off);
        omap_val_t* child_node_omap_val = get_btree_phys_omap_val(vol_omap_root_node, *child_node_virt_oid, max_xid);
        if (!child_node_omap_val) {
            fprintf(stderr, "get_fs_records: Need to descend to node with Virtual OID 0x%llx, but the file-system object map lists no objects with this Virtual OID.\n", *child_node_virt_oid);
            goto onFatal;
        }
        
        if (read_blocks(node, child_node_omap_val->ov_paddr, 1) != 1) {
            fprintf(stderr, "ERROR: get_fs_records: Failed to read block 0x%llx.\n", child_node_omap_val->ov_paddr);
            goto onFatal;
        }

        if (!is_cksum_valid(node)) {
            fprintf(stderr, "\nABORT: get_fs_records: Checksum of node at block 0x%llx did not validate.\n", child_node_omap_val->ov_paddr);
            goto onFatal;
        }

        toc_start = (char*)(node->btn_data) + node->btn_table_space.off;
        key_start = toc_start + node->btn_table_space.len;
        val_end   = (char*)node + nx_block_size;    // Always dealing with non-root node here

        i++;
    }

    /*
     * Now that we've found the first record with the given OID, walk along the
     * tree to get the rest of the records with that OID.
     * 
     * We do so by following `desc_path`, which describes the path to the the
     * next leaf-node entry in order, and then adjusting the values in
     * `desc_path` so that our next descent from the root takes us to the next
     * unvisited leaf-node entry in order.
     * 
     * TODO: This procedure could be optimised by walking along leaf nodes
     * directly rather than making a new descent from the root just to find the
     * sibling of an entry in a leaf node --- HINT: make `desc_path` one entry
     * shorter
     */
    while (true) {
        // Reset current node and pointers to the root node
        memcpy(node, vol_fs_root_node, nx_block_size);
        toc_start = (char*)(node->btn_data) + node->btn_table_space.off;
        key_start = toc_start + node->btn_table_space.len;
        val_end   = (char*)node + nx_block_size - sizeof(btree_info_t);

        if (debug) printf("\t%s:%i vol_fs_root_node->btn_level=%i\n", __func__, __LINE__, vol_fs_root_node->btn_level);
        for (i = 0; i <= vol_fs_root_node->btn_level; i++) {

            if (debug) printf("\t%s:%i i=%i\n", __func__, __LINE__, i);


            if (node->btn_flags & BTNODE_FIXED_KV_SIZE) {
                fprintf(stderr, "\nget_fs_records: File-system root B-trees don't have fixed size keys and values ... do they?\n");
                goto onFatal;
            }

            if (debug) printf("\t%s:%i desc_path[i]=%i node->btn_nkeys=%i\n", __func__, __LINE__, desc_path[i], node->btn_nkeys);
            if (desc_path[i] >= node->btn_nkeys) {
                // We've already gone through the last entry in this node.

                if (debug) printf("\t%s:%i node->btn_flags=%x\n", __func__, __LINE__, node->btn_flags);
                if (node->btn_flags & BTNODE_ROOT) {
                    // We've gone through the whole tree; return the results
                    free(node);
                    free(bt_info);
                    return records;
                }
                
                // Prep `desc_path` to take us to the leftmost descendant of
                // this node's sibling; then break from this for-loop so that
                // we loop inside the while-loop, i.e. make a new descent from
                // the root.
                desc_path[i - 1]++;
                for (uint16_t j = i; j <= vol_fs_root_node->btn_level; j++) {
                    desc_path[j] = 0;
                }
                break;
            }

            // TOC entries are instances of `kvloc_t`; look at the entry
            // described by `desc_path`
            kvloc_t* toc_entry = (kvloc_t*)toc_start + desc_path[i];

            // If this is a leaf node, we have the next
            // record; add it to the records array
            if (node->btn_flags & BTNODE_LEAF) {
                j_key_t* key = key_start + toc_entry->k.off;
                oid_t record_oid = key->obj_id_and_type & OBJ_ID_MASK;

                if (record_oid != oid) {
                    // This record doesn't have the right OID, so we must have
                    // found all of the relevant records; return the results
                    free(bt_info);
                    free(node);
                    return records;
                }

                char* val = val_end - toc_entry->v.off;

                records[num_records] = malloc(sizeof(j_rec_t) + toc_entry->k.len + toc_entry->v.len);
                if (!records[num_records]) {
                    fprintf(stderr, "\nABORT: get_fs_records: Could not allocate sufficient memory for `records[%lu]`.\n", num_records);
                    goto onFatal;
                }
                
                records[num_records]->key_len = toc_entry->k.len;
                records[num_records]->val_len = toc_entry->v.len;
                memcpy(
                    records[num_records]->data,
                    key,
                    records[num_records]->key_len
                );
                memcpy(
                    records[num_records]->data + records[num_records]->key_len,
                    val,
                    records[num_records]->val_len
                );

                num_records++;
                records = realloc(records, (num_records + 1) * sizeof(j_rec_t*));
                if (!records) {
                    fprintf(stderr, "\nABORT: get_fs_records: Could not allocate sufficient memory for `records`.\n");
                    goto onFatal;
                }
                records[num_records] = NULL;

                // Prep `desc_path` for the next descent from the root node,
                // then actually start the next descent from the root node.
                desc_path[i]++;
                break;
            }

            // Else, read the corresponding child node into memory and loop
            oid_t* child_node_virt_oid = val_end - toc_entry->v.off;
            omap_val_t* child_node_omap_val = get_btree_phys_omap_val(vol_omap_root_node, *child_node_virt_oid, max_xid);
            if (!child_node_omap_val) {
                fprintf(stderr, "get_fs_records: Need to descend to node with Virtual OID 0x%llx, but the file-system object map lists no objects with this Virtual OID.\n", *child_node_virt_oid);
                goto onFatal;
            }
            
            if (read_blocks(node, child_node_omap_val->ov_paddr, 1) != 1) {
                fprintf(stderr, "\nABORT: get_fs_records: Failed to read block 0x%llx.\n", child_node_omap_val->ov_paddr);
                goto onFatal;
            }

            if (!is_cksum_valid(node)) {
                fprintf(stderr, "\nABORT: get_fs_records: Checksum of node at block 0x%llx did not validate.\n", child_node_omap_val->ov_paddr);
                goto onFatal;
            }

            toc_start = (char*)(node->btn_data) + node->btn_table_space.off;
            key_start = toc_start + node->btn_table_space.len;
            val_end   = (char*)node + nx_block_size;    // Always dealing with non-root node here
        }
    }

onFatal:
    free(bt_info);
    free(node);
    free_j_rec_array(records);
    return NULL;
}

#endif // APFS_FUNC_BTREE_H
