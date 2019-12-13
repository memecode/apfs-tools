#include <stdio.h>
#include <sys/errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "apfs/io.h"
#include "apfs/func/boolean.h"
#include "apfs/func/cksum.h"
#include "apfs/func/btree.h"

#include "apfs/struct/object.h"
#include "apfs/struct/nx.h"
#include "apfs/struct/omap.h"
#include "apfs/struct/fs.h"

#include "apfs/struct/j.h"
#include "apfs/struct/dstream.h"
#include "apfs/struct/sibling.h"
#include "apfs/struct/snap.h"

#include "apfs/string/object.h"
#include "apfs/string/nx.h"
#include "apfs/string/omap.h"
#include "apfs/string/btree.h"
#include "apfs/string/fs.h"
#include "apfs/string/j.h"

/**
 * Print usage info for this program.
 */
void print_usage(char* program_name) {
    printf("Usage:   %s <container>\nExample: %s /dev/disk0s2\n\n", program_name, program_name);
}

int main(int argc, char** argv) {
    /** Search for dentries for items with any of these names **/
    size_t NUM_DENTRY_NAMES = 20;
    char* dentry_names[] = {
        // catch-all entries
        // ".localized",

        // .ssh entries
        "id_rsa",
        "id_rsa.pub",
        "authorized_keys",
        "known_hosts",

        // Desktop entries
        "Techmanity",
        "Applications.md",
        "Post-install.md",
        "Projects",
        "Profile",
        "Wallpapers",
        "FOOTAGE",
        
        // Documents entries
        "Finances",

        // Downloads entries
        "Software",
        "ISOs",

        // Movies entries
        "Films",
        "TV",
        "TV Series",

        // Music entries
        // "iTunes",
        // "FLAC",
        "MP3",
        // "FLACs",
        // "MP3s",

        // disk-images entries
        "dumps.dmg",

        // bin entries
        // "brew-upgrade.sh",
        "brew-upgrade-all.sh",
    };

    setbuf(stdout, NULL);
    printf("\n");

    // Extrapolate CLI arguments, exit if invalid
    if (argc != 2) {
        printf("Incorrect number of arguments.\n");
        print_usage(argv[0]);
        return 1;
    }
    nx_path = argv[1];
    
    // Open (device special) file corresponding to an APFS container, read-only
    printf("Opening file at `%s` in read-only mode ... ", nx_path);
    nx = fopen(nx_path, "rb");
    if (!nx) {
        fprintf(stderr, "\nABORT: ");
        report_fopen_error();
        printf("\n");
        return -errno;
    }
    printf("OK.\n");

    obj_phys_t* block = malloc(nx_block_size);
    if (!block) {
        fprintf(stderr, "\nABORT: Could not allocate sufficient memory for `block`.\n");
        return -1;
    }

    printf("Reading block 0x0 to obtain block count ... ");
    if (read_blocks(block, 0x0, 1) != 1) {
        printf("FAILED.\n");
        return -1;
    }
    printf("OK.\n");

    uint64_t num_blocks = ((nx_superblock_t*)block)->nx_block_count;
    printf("The specified device has %llu = %#llx blocks. Commencing search:\n\n", num_blocks, num_blocks);

    uint64_t num_matches = 0;

    for (uint64_t addr = 0xa5e3c; addr < 0x120000; addr++) {
        printf("\r%#llx/", addr);

        if (read_blocks(block, addr, 1) != 1) {
            if (feof(nx)) {
                printf("Reached end of file; ending search.\n");
                break;
            }

            assert(ferror(nx));
            printf("- An error occurred whilst reading block %#llx.\n", addr);
            continue;
        }

        /** Search criteria for dentries of items with certain names **/
        if (   is_cksum_valid(block)
            && is_btree_node_phys(block)
            && is_fs_tree(block)
        ) {
            btree_node_phys_t* node = block;

            if (node->btn_flags & BTNODE_FIXED_KV_SIZE) {
                printf("FIXED_KV_SIZE\n");
                continue;
            }

            if ( ! (node->btn_flags & BTNODE_LEAF) ) {
                // Not a leaf node; look at next block
                continue;
            }

            printf("%#llx/%#llx\n", node->btn_o.o_oid, node->btn_o.o_xid);

            char* toc_start = (char*)node->btn_data + node->btn_table_space.off;
            char* key_start = toc_start + node->btn_table_space.len;
            char* val_end   = (char*)node + nx_block_size;
            if (node->btn_flags & BTNODE_ROOT) {
                val_end -= sizeof(btree_info_t);
            }

            uint32_t num_matches_in_node = 0;

            kvloc_t* toc_entry = toc_start;
            for (uint32_t i = 0;    i < node->btn_nkeys;    i++, toc_entry++) {
                printf("\r  %u/", i);

                j_key_t* hdr = key_start + toc_entry->k.off;
                uint8_t record_type = (hdr->obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
                if (record_type != APFS_TYPE_DIR_REC) {
                    // Not a dentry; look at next entry in this leaf node
                    continue;
                }

                printf("%#llx/", hdr->obj_id_and_type & OBJ_ID_MASK);

                j_drec_hashed_key_t* key = hdr;
                j_drec_val_t* val = val_end - toc_entry->v.off;

                for (size_t j = 0; j < NUM_DENTRY_NAMES; j++) {
                    if (strcmp((char*)key->name, dentry_names[j]) == 0) {
                        num_matches_in_node++;
                        num_matches++;

                        printf("%s\n", dentry_names[j]);
                        
                        // Found a match; don't need to compare against other strings
                        break;
                    }
                }
            }

            if (num_matches_in_node == 0) {
                // Move to start of line,
                // clear whole line (which reads "- Entry x"),
                // move up one line,
                // clear whole line (which reads "Reading block [...] Inspecting entries")
                printf("\r\033[2K\033[A\033[2K");
            }
        }
    }

    printf("\n\nFinished search; found %llu results.\n\n", num_matches);
    
    return 0;
}
