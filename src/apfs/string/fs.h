/**
 * Functions that generate nicely formatted strings representing data found in
 * File-system-related objects, such as `apfs_superblock_t`.
 */

#ifndef APFS_STRING_FS_H
#define APFS_STRING_FS_H

#include <time.h>

#include "../struct/fs.h"
#include "object.h"

/**
 * Get a human-readable string that lists the optional feature flags that are
 * set on a given APFS volume superblock.
 * 
 * apsb:    A pointer to the APFS volume superblock in question.
 * 
 * RETURN VALUE:
 *      A pointer to the first character of the string. The caller must free
 *      this pointer when it is no longer needed.
 */
char* get_apfs_features_string(apfs_superblock_t* apsb) {
    // String to use if no flags are set
    char* default_string = "- No volume feature flags are set.\n";
    size_t default_string_len = strlen(default_string);
    
    const int NUM_FLAGS = 3;
    
    uint64_t flag_constants[] = {
        APFS_FEATURE_DEFRAG_PRERELEASE,
        APFS_FEATURE_HARDLINK_MAP_RECORDS,
        APFS_FEATURE_DEFRAG,
    };

    char* flag_strings[] = {
        "Reserved --- To avoid data corruption, this flag must not be set; this flag enabled a prerelease version of the defragmentation system in macOS 10.13 versions. Itʼs ignored by macOS 10.13.6 and later.",
        "This volume has hardlink map records.",
        "Defragmentation is supported.",
    };

    // Allocate sufficient memory for the result string
    size_t max_mem_required = 0;
    for (int i = 0; i < NUM_FLAGS; i++) {
        max_mem_required += strlen(flag_strings[i]) + 3;
        // `+ 3` accounts for prepending "- " and appending "\n" to each string
    }
    if (max_mem_required < default_string_len) {
        max_mem_required = default_string_len;
    }
    max_mem_required++; // Make room for terminating NULL byte

    char* result_string = malloc(max_mem_required);
    if (!result_string) {
        fprintf(stderr, "\nABORT: get_apfs_features_string: Could not allocate sufficient memory for `result_string`.\n");
        exit(-1);
    }

    char* cursor = result_string;

    // Go through possible flags, adding corresponding string to result if
    // that flag is set.
    for (int i = 0; i < NUM_FLAGS; i++) {
        if (apsb->apfs_features & flag_constants[i]) {
            *cursor++ = '-';
            *cursor++ = ' ';
            memcpy(cursor, flag_strings[i], strlen(flag_strings[i]));
            cursor += strlen(flag_strings[i]);
            *cursor++ = '\n';
        }
    }

    if (cursor == result_string) {
        // No strings were added, so it must be that no flags are set.
        memcpy(cursor, default_string, default_string_len);
        cursor += default_string_len;
    }

    *cursor = '\0';

    // Free up excess allocated memory.
    result_string = realloc(result_string, strlen(result_string) + 1);
    return result_string;
}

/**
 * Get a human-readable string that lists the optional feature flags that are
 * set on a given APFS volume superblock.
 * 
 * apsb:    A pointer to the APFS volume superblock in question.
 * 
 * RETURN VALUE:
 *      A pointer to the first character of the string. The caller must free
 *      this pointer when it is no longer needed.
 */
char* get_apfs_readonly_compatible_features_string(apfs_superblock_t* apsb) {
    // String to use if no flags are set
    char* default_string = "- No read-only compatible volume feature flags are set.\n";
    size_t default_string_len = strlen(default_string);
    
    const int NUM_FLAGS = 0;
    uint64_t flag_constants[1] = { 0
        // empty
    };
    char* flag_strings[1] = { 0
        // empty
    };

    // Allocate sufficient memory for the result string
    size_t max_mem_required = 0;
    for (int i = 0; i < NUM_FLAGS; i++) {
        max_mem_required += strlen(flag_strings[i]) + 3;
        // `+ 3` accounts for prepending "- " and appending "\n" to each string
    }
    if (max_mem_required < default_string_len) {
        max_mem_required = default_string_len;
    }
    max_mem_required++; // Make room for terminating NULL byte

    char* result_string = malloc(max_mem_required);
    if (!result_string) {
        fprintf(stderr, "\nABORT: get_apfs_readonly_compatible_features_string: Could not allocate sufficient memory for `result_string`.\n");
        exit(-1);
    }

    char* cursor = result_string;

    // Go through possible flags, adding corresponding string to result if
    // that flag is set.
    for (int i = 0; i < NUM_FLAGS; i++) {
        if (apsb->apfs_readonly_compatible_features & flag_constants[i]) {
            *cursor++ = '-';
            *cursor++ = ' ';
            memcpy(cursor, flag_strings[i], strlen(flag_strings[i]));
            cursor += strlen(flag_strings[i]);
            *cursor++ = '\n';
        }
    }

    if (cursor == result_string) {
        // No strings were added, so it must be that no flags are set.
        memcpy(cursor, default_string, default_string_len);
        cursor += default_string_len;
    }

    *cursor = '\0';

    // Free up excess allocated memory.
    result_string = realloc(result_string, strlen(result_string) + 1);
    return result_string;
}

/**
 * Get a human-readable string that lists the backward-incompatible feature
 * flags that are set on a given APFS volume superblock.
 * 
 * apsb:    A pointer to the APFS volume superblock in question.
 * 
 * RETURN VALUE:
 *      A pointer to the first character of the string. The caller must free
 *      this pointer when it is no longer needed.
 */
char* get_apfs_incompatible_features_string(apfs_superblock_t* apsb) {
    // String to use if no flags are set
    char* default_string = "- No backward-incompatible volume feature flags are set.\n";
    size_t default_string_len = strlen(default_string);
    
    const int NUM_FLAGS = 4;

    uint64_t flag_constants[] = {
        APFS_INCOMPAT_CASE_INSENSITIVE,
        APFS_INCOMPAT_DATALESS_SNAPS,
        APFS_INCOMPAT_ENC_ROLLED,
        APFS_INCOMPAT_NORMALIZATION_INSENSITIVE,
    };

    char* flag_strings[] = {
        "Filenames on this volume are case-insensitive.",
        "At least one snapshot with no data exists for this volume.",
        "This volume's encryption has changed keys at least once.",
        "Filenames on this volume are normalization insensitive.",
    };

    // Allocate sufficient memory for the result string
    size_t max_mem_required = 0;
    for (int i = 0; i < NUM_FLAGS; i++) {
        max_mem_required += strlen(flag_strings[i]) + 3;
        // `+ 3` accounts for prepending "- " and appending "\n" to each string
    }
    if (max_mem_required < default_string_len) {
        max_mem_required = default_string_len;
    }
    max_mem_required++; // Make room for terminating NULL byte

    char* result_string = malloc(max_mem_required);
    if (!result_string) {
        fprintf(stderr, "\nABORT: get_apfs_incompatible_features_string: Could not allocate sufficient memory for `result_string`.\n");
        exit(-1);
    }

    char* cursor = result_string;

    // Go through possible flags, adding corresponding string to result if
    // that flag is set.
    for (int i = 0; i < NUM_FLAGS; i++) {
        if (apsb->apfs_incompatible_features & flag_constants[i]) {
            *cursor++ = '-';
            *cursor++ = ' ';
            memcpy(cursor, flag_strings[i], strlen(flag_strings[i]));
            cursor += strlen(flag_strings[i]);
            *cursor++ = '\n';
        }
    }

    if (cursor == result_string) {
        // No strings were added, so it must be that no flags are set.
        memcpy(cursor, default_string, default_string_len);
        cursor += default_string_len;
    }

    *cursor = '\0';

    // Free up excess allocated memory.
    result_string = realloc(result_string, strlen(result_string) + 1);
    return result_string;
}

/**
 * Get a human-readable string that lists the volume flags that are
 * set on a given APFS volume superblock.
 * 
 * apsb:    A pointer to the APFS volume superblock in question.
 * 
 * RETURN VALUE:
 *      A pointer to the first character of the string. The caller must free
 *      this pointer when it is no longer needed.
 */
char* get_apfs_fs_flags_string(apfs_superblock_t* apsb) {
    // String to use if no flags are set
    char* default_string = "- No flags are set.\n";
    size_t default_string_len = strlen(default_string);
    
    const int NUM_FLAGS = 7;

    uint64_t flag_constants[] = {
        APFS_FS_UNENCRYPTED,
        APFS_FS_RESERVED_2,
        APFS_FS_RESERVED_4,
        APFS_FS_ONEKEY,
        APFS_FS_SPILLEDOVER,
        APFS_FS_RUN_SPILLOVER_CLEANER,
        APFS_FS_ALWAYS_CHECK_EXTENTREF,
    };
    
    char* flag_strings[] = {
        "Volume is unencrypted.",
        "Reserved flag (0x2).",
        "Reserved flag (0x4).",
        "Single VEK (volume encryption key) for all files in this volume.",
        "Volume has run out of allocated space on SSD, so has spilled over to other drives.",
        "Volume has spilled over and spillover cleaner must be run.",
        "When deciding whether to overwrite a file extent, always consult the extent reference tree.",
    };

    // Allocate sufficient memory for the result string
    size_t max_mem_required = 0;
    for (int i = 0; i < NUM_FLAGS; i++) {
        max_mem_required += strlen(flag_strings[i]) + 3;
        // `+ 3` accounts for prepending "- " and appending "\n" to each string
    }
    if (max_mem_required < default_string_len) {
        max_mem_required = default_string_len;
    }
    max_mem_required++; // Make room for terminating NULL byte

    char* result_string = malloc(max_mem_required);
    if (!result_string) {
        fprintf(stderr, "\nABORT: get_apfs_fs_flags_string: Could not allocate sufficient memory for `result_string`.\n");
        exit(-1);
    }

    char* cursor = result_string;

    // Go through possible flags, adding corresponding string to result if
    // that flag is set.
    for (int i = 0; i < NUM_FLAGS; i++) {
        if (apsb->apfs_fs_flags & flag_constants[i]) {
            *cursor++ = '-';
            *cursor++ = ' ';
            memcpy(cursor, flag_strings[i], strlen(flag_strings[i]));
            cursor += strlen(flag_strings[i]);
            *cursor++ = '\n';
        }
    }

    if (cursor == result_string) {
        // No strings were added, so it must be that no flags are set.
        memcpy(cursor, default_string, default_string_len);
        cursor += default_string_len;
    }

    *cursor = '\0';

    // Free up excess allocated memory.
    result_string = realloc(result_string, strlen(result_string) + 1);
    return result_string;
}

/**
 * Get a human-readable string that lists the role flags that are
 * set on a given APFS volume superblock.
 * 
 * apsb:    A pointer to the APFS volume superblock in question.
 * 
 * RETURN VALUE:
 *      A pointer to the first character of the string. The caller must free
 *      this pointer when it is no longer needed.
 */
char* get_apfs_role_string(apfs_superblock_t* apsb) {
    // String to use if no flags are set
    char* default_string = "- This volume has no defined roles\n";
    size_t default_string_len = strlen(default_string);
    
    const int NUM_FLAGS = 9;

    // `APFS_ROLE_NONE` (0x0) is intentionally ommitted from this array.
    uint64_t flag_constants[] = {
        APFS_VOL_ROLE_SYSTEM,
        APFS_VOL_ROLE_USER,
        APFS_VOL_ROLE_RECOVERY,
        APFS_VOL_ROLE_VM,
        APFS_VOL_ROLE_PREBOOT,
        APFS_VOL_ROLE_INSTALLER,
        APFS_VOL_ROLE_DATA,
        APFS_VOL_ROLE_BASEBAND,
        APFS_VOL_ROLE_RESERVED_200,
    };
    
    char* flag_strings[] = {
        "Root volume (contains a root directory for the system)",
        "Home volume (contains users' home directories)",
        "Recovery volume (contains a recovery system)",
        "Swap volume (used as swap space for virtual memory)",
        "Preboot volume (contains files needed to boot from an encrypted volumes)",
        "Installer volume (used by the OS installer)",
        "Data volume (contains mutable data)",
        "Baseband volume (sed by the radio firmware)",
        "Reserved flag (0x200)",
    };

    // Allocate sufficient memory for the result string
    size_t max_mem_required = 0;
    for (int i = 0; i < NUM_FLAGS; i++) {
        max_mem_required += strlen(flag_strings[i]) + 3;
        // `+ 3` accounts for prepending "- " and appending "\n" to each string
    }
    if (max_mem_required < default_string_len) {
        max_mem_required = default_string_len;
    }
    max_mem_required++; // Make room for terminating NULL byte

    char* result_string = malloc(max_mem_required);
    if (!result_string) {
        fprintf(stderr, "\nABORT: get_apfs_role_string: Could not allocate sufficient memory for `result_string`.\n");
        exit(-1);
    }

    char* cursor = result_string;

    // Go through possible flags, adding corresponding string to result if
    // that flag is set.
    for (int i = 0; i < NUM_FLAGS; i++) {
        if (apsb->apfs_role & flag_constants[i]) {
            *cursor++ = '-';
            *cursor++ = ' ';
            memcpy(cursor, flag_strings[i], strlen(flag_strings[i]));
            cursor += strlen(flag_strings[i]);
            *cursor++ = '\n';
        }
    }

    if (cursor == result_string) {
        // No strings were added, so it must be that no flags are set.
        memcpy(cursor, default_string, default_string_len);
        cursor += default_string_len;
    }

    *cursor = '\0';

    // Free up excess allocated memory.
    result_string = realloc(result_string, strlen(result_string) + 1);
    return result_string;
}

/**
 * Print a nicely formatted string describing the data contained in a given
 * APFS volume superblock.
 * 
 * apsb:    A pointer to the APFS volume superblock in question.
 */
void print_apfs_superblock(apfs_superblock_t* apsb) {
    print_obj_phys((obj_phys_t*)apsb);   // `apsb` equals `&(apsb->apfs_o)`
    printf("\n");

    char magic_string[] = {
        (char)apsb->apfs_magic,
        (char)(apsb->apfs_magic >> 8),
        (char)(apsb->apfs_magic >> 16),
        (char)(apsb->apfs_magic >> 24),
        '\0'
    };
    printf("Magic string:                           %s\n",  magic_string);
    printf("Index within container volume array:    %u\n",  apsb->apfs_fs_index);
    printf("\n");

    printf("Volume name:        ### %s ###\n",  apsb->apfs_volname);
    printf("\n");

    char* tmp_string = get_apfs_fs_flags_string(apsb);
    printf("Flags:\n%s", tmp_string);
    free(tmp_string);

    tmp_string = get_apfs_features_string(apsb);
    printf("Supported features:\n%s", tmp_string);
    free(tmp_string);

    tmp_string = get_apfs_readonly_compatible_features_string(apsb);
    printf("Supported read-only compatible features:\n%s", tmp_string);
    free(tmp_string);

    tmp_string = get_apfs_incompatible_features_string(apsb);
    printf("Backward-incompatible features:\n%s", tmp_string);
    free(tmp_string);

    tmp_string = get_apfs_role_string(apsb);
    printf("Roles:\n%s", tmp_string);
    free(tmp_string);
    
    printf("\n");

    // Dividing timestamps by 10^9 to convert APFS timestamps (Unix timestamps
    // in nanoseconds) to Unix timestamps in seconds.
    // Trailing '\n' for each line is provided by the result of `ctime()`.
    time_t timestamp = apsb->apfs_unmount_time  / 1000000000;
    printf("Last unmount time:                  %s",    ctime(&timestamp));
    timestamp = apsb->apfs_last_mod_time / 1000000000;
    printf("Last modification time:             %s",    ctime(&timestamp));
    printf("\n");

    printf("Reserved blocks:                    %llu blocks\n", apsb->apfs_fs_reserve_block_count);
    printf("Block quota:                        %llu blocks\n", apsb->apfs_fs_quota_block_count);
    printf("Allocated blocks:                   %llu blocks\n", apsb->apfs_fs_alloc_count);
    printf("\n");

    printf("Volume object map Physical OID:     0x%llx\n",    apsb->apfs_omap_oid);
    printf("\n");
    
    printf("Root tree info:\n");
    printf("- OID:              0x%llx\n",  apsb->apfs_root_tree_oid);
    printf("- Storage type:     %s\n",      o_storage_type_to_string(apsb->apfs_root_tree_type));
    
    tmp_string = get_o_type_flags_string(apsb->apfs_root_tree_type);
    printf("- Type flags:       %s\n",  tmp_string);
    free(tmp_string);

    tmp_string = get_o_type_string(apsb->apfs_root_tree_type);
    printf("- Object type:      %s\n",  tmp_string);
    free(tmp_string);

    printf("\n");

    printf("Extent-reference tree info:\n");
    printf("- OID:              0x%llx\n",  apsb->apfs_extentref_tree_oid);
    printf("- Storage type:     %s\n",      o_storage_type_to_string(apsb->apfs_extentref_tree_type));
    
    tmp_string = get_o_type_flags_string(apsb->apfs_extentref_tree_type);
    printf("- Type flags:       %s\n",  tmp_string);
    free(tmp_string);

    tmp_string = get_o_type_string(apsb->apfs_extentref_tree_type);
    printf("- Object type:      %s\n",  tmp_string);
    free(tmp_string);

    printf("\n");

    printf("Snapshot metadata tree info:\n");
    printf("- OID:              0x%llx\n",  apsb->apfs_snap_meta_tree_oid);
    printf("- Storage type:     %s\n",      o_storage_type_to_string(apsb->apfs_snap_meta_tree_type));
    
    tmp_string = get_o_type_flags_string(apsb->apfs_snap_meta_tree_type);
    printf("- Type flags:       %s\n",  tmp_string);
    free(tmp_string);

    tmp_string = get_o_type_string(apsb->apfs_snap_meta_tree_type);
    printf("- Object type:      %s\n",  tmp_string);
    free(tmp_string);

    printf("\n");

    printf("On next mount, revert to:\n");
    printf("- snapshot with this XID:                           0x%llx\n",  apsb->apfs_revert_to_xid);
    printf("- APFS volume superblock with this Physical OID:    0x%llx\n",  apsb->apfs_revert_to_sblock_oid);
    printf("\n");

    printf("Next file-system object ID that will be assigned:   0x%llx\n",  apsb->apfs_next_obj_id);
    printf("Next document ID that will be assigned:             0x%x\n",    apsb->apfs_next_doc_id);
    printf("\n");

    printf("Number of:\n");
    printf("\n");
    printf("- regular files:                %llu\n",    apsb->apfs_num_files);
    printf("- directories:                  %llu\n",    apsb->apfs_num_directories);
    printf("- symbolic links:               %llu\n",    apsb->apfs_num_symlinks);
    printf("- other file-system objects:    %llu\n",    apsb->apfs_num_other_fsobjects);
    printf("\n");
    printf("- snapshots:                    %llu\n",    apsb->apfs_num_snapshots);
    printf("- block allocations ever made:  %llu\n",    apsb->apfs_total_block_alloced);
    printf("- block liberations ever made:  %llu\n",    apsb->apfs_total_blocks_freed);
    printf("\n");

    printf("UUID:   0x%016llx%016llx\n",
        *((uint64_t*)(apsb->apfs_vol_uuid) + 1),
        * (uint64_t*)(apsb->apfs_vol_uuid)
    );

    /*
     * TODO: Print the following fields:
     * - apfs_formatted_by
     * - apfs_modified_by
     * - apfs_root_to_xid
     * - apfs_er_state_oid
     */
}

#endif // APFS_STRING_FS_H
