**TL;DR — My APFS volume appears to be partially corrupted and missing my user directory, `/Users/jivan`. How can I recover it?**

# Update 2019-11-14

I didn't need to discover the IDs of my files in the manner I described in my previous update. Rather, I was able to see that `/Users/jivan` had an ID of `0xb54a8`, and thus search for nodes which contained dentries for items whose parent ID was also `0xb54a8`; these nodes were then the ones which listed the contents of `/Users/jivan`.

In order to more easily do an automated recovery, I reconstructed the missing internal node of the file-system B-tree, and then wrote a tool called `apfs-recover`, which — when given a device file, a volume ID, and a path to a file within that volume — writes the content of that file (if it can find all the file extents) to `stdout`. Then, to recover `/Users/jivan/Documents/my file.pdf`, for example, I can do:

```
apfs-recover /dev/disk2s2 0 "/Users/jivan/Documents/my file.pdf" > "~/Desktop/my file.pdf"
```

I wrote a Bash script, `pull.sh`, which, when given a target recovery directory and a file which lists paths to files to attempt to recover, runs `apfs-recover` for each such filepath and outputs the result to a corresponding path in the recovery directory. For example, if the contents of `filepaths.txt` are

```
/Users/jivan/Documents/my doc.pdf
/Users/jivan/Pictures/my pic.jpg
```

then running `pull.sh ~/Dekstop/RECOVERY filepaths.txt` recovers the files to the following paths:

```
~/Desktop/RECOVERY/Users/jivan/Documents/my doc.pdf
~/Desktop/RECOVERY/Users/jivan/Pictures/my pic.jpg
```

I added the desired entries in `filepaths.txt` with some programmatic assistance, and am now successfully recovering the vast majority of my files. For any particularly important files which this script fails to recover (due to bugs in the software I've written or additional malformed/missing APFS structures on the affected disk) I'll have to dig deeper, but this is effectively solved now.

For anyone that wants to use `apfs-recover`, I should be adding it to [the GitHub repo](https://github.com/jivanpal/apfs-tools) sometime soon.

# Update 2019-11-11

Using [the tools I've made](https://github.com/jivanpal/apfs-tools), I've indeed found that a few nodes in the file-system root B-tree for my main APFS volume have been zeroed out. Thanks to APFS's copy-on-write/transaction-based behaviour, I have been able to search the entire disk for older versions of these missing nodes, and I've successfully found recent instances of them — *except* for the particular leaf node that contains the file-system records for `/Users/jivan`, so its contents cannot be directly determined. Just my luck(!)

Provided that I can recall the name of at least one item in a given directory within `/Users/jivan`, I should be able to find the file-system records for that directory, and thus recover that entire directory. For example, if I know that the file `/Users/jivan/Desktop/my-file.txt` exists, I can search the disk for all leaf nodes which contain a dentry for a file named `my-file.txt`, and thus determine the Virtual object ID of `/Users/jivan/Desktop`. Once this ID is known, all of the file-system records for `/Users/jivan/Desktop` (in particular, the dentries for items within `/Users/jivan/Desktop`) should be discoverable.

Of course, the success of such a search relies on these file-system records existing in a leaf node that has also not been clobbered/zeroed-out.

# Update 2019-10-31

I'm now [developing some tools](https://github.com/jivanpal/apfs-tools) to inspect the APFS object map and see if I can regain access to my files the ridiculously hard way. Hopefully, the object map for an older APFS transaction is intact...

There is a particular `fsck` error that I encountered, which is in my original post:

```
** Checking the object map.
error: (oid 0xd31c1) om: btn: found zeroed-out block
   Object map is invalid.
```

Here, `om` refers to the object map of the `macOS` volume, and `btn` refers to a B-tree node in that object map. Evidently, part of the node has been zeroed-out, leading to some or all of the dentries for `/Users/jivan` being inaccessible.

# Original post 2019-10-28

The SATA cable in my MacBookPro9,2, which was running Mojave, failed recently. Whilst waiting for a replacement cable, I used a SATA-to-USB adapter to attempt to diagnose whether the drive (a Kingston A400 480GB) was at fault, using Ubuntu on another laptop. It seems that some of the data on the drive became corrupted due to the failing cable, as `gdisk` reported that the main GPT table was corrupted. I restored it from the backup GPT table, as that was apparently intact.

I then used [`apfs-fuse`](https://github.com/sgan81/apfs-fuse) to attempt to mount the APFS volumes on the drive (it just has an ESP and an unencrypted APFS container with the installation of Mojave that I was using). This showed that all the data on the main APFS volume was intact, aside from my user folder, `/Users/jivan`. Whilst `jivan` is shown by `ls /mnt/apfs-volume/Users`, trying `cd` or `ls` on `jivan` reports an I/O error.

***

I booted into macOS Catalina Internet Recovery on my MacBook to inspect the drive further there using the SATA-to-USB cable, but `diskutil apfs list` reported that the APFS container had no volumes along with some errors, as follows:

    APFS Containers (2 found)
    |
    +-- Container ERROR -69808
        ======================
        APFS Container Reference:     disk23
        Size (Capacity Ceiling):      ERROR -69620
        Capacity In Use By Volumes:   ERROR -69620
        Capacity Not Allocated:       ERROR -69620
        |
        +-< Physical Store disk22s2 60A9A81B-E7B9-4471-A76B-B98A419B5928
        |   -----------------------------------------------------------
        |   APFS Physical Store Disk:   disk22s2
        |   Size:                       479894224896 B (479.9 GB)
        |
        +-> No Volumes

Also, `fsck_apfs -n /dev/disk22` gives the following (and likewise for `disk22s2` and `disk23`):

    ** Checking the container superblock.
    ** Checking the EFI jumpstart record.
    ** Checking the space manager.
    ** Checking the space manager free queue trees.
    ** Checking the object map.
    ** Checking volume.
    ** Checking the APFS volume superblock.
    ** The volume macOS was formatted by diskmanagementd (945.241.4) and last modified by apfs_kext (1412.11.7).
    ** Checking the object map.
    error: (oid 0xd31c1) om: btn: found zeroed-out block
       Object map is invalid.
    ** The volume /dev/disk22 could not be verified completely.

***

After my replacement SATA cable arrived, I fitted the Kingston drive back into my MacBook using the new cable, and interestingly, `diskutil apfs list` in Internet Recovery revealed the APFS volumes. I decided to install Catalina onto another drive (a Samsung spinning HDD), and booted from that using my SATA-to-USB cable to inspect further. Catalina mounted the Kingston drive's main APFS volume automatically on login, and shows the same directory tree and contents as `apfs-fuse` did on my Ubuntu laptop, but the `/Users/jivan` directory is apparently missing altogether from the volume; `ls -al /Volumes/apfs-volume/Users` just shows the file `.localized` and the directory `Shared`.

In this environment, here is the output of some commands. Here, `disk0` is the affected Kingston drive, and `disk2` is the Samsung drive which I am booting from over USB:

**`diskutil apfs list` —**

    APFS Containers (2 found)
    |
    +-- Container disk1 37FD550D-60EE-4499-8C8F-DA1B831D5307
    |   ====================================================
    |   APFS Container Reference:     disk1
    |   Size (Capacity Ceiling):      479894224896 B (479.9 GB)
    |   Capacity In Use By Volumes:   393049374720 B (393.0 GB) (81.9% used)
    |   Capacity Not Allocated:       86844850176 B (86.8 GB) (18.1% free)
    |   |
    |   +-< Physical Store disk0s2 60A9A81B-E7B9-4471-A76B-B98A419B5928
    |   |   -----------------------------------------------------------
    |   |   APFS Physical Store Disk:   disk0s2
    |   |   Size:                       479894224896 B (479.9 GB)
    |   |
    |   +-> Volume disk1s1 6E2363BF-6CE3-4C87-9F02-BFFE741B8C6E
    |   |   ---------------------------------------------------
    |   |   APFS Volume Disk (Role):   disk1s1 (No specific role)
    |   |   Name:                      macOS (Case-insensitive)
    |   |   Mount Point:               Not Mounted
    |   |   Capacity Consumed:         389137477632 B (389.1 GB)
    |   |   FileVault:                 No
    |   |
    |   +-> Volume disk1s2 1190A062-F647-4365-A53F-2D0F18786F4C
    |   |   ---------------------------------------------------
    |   |   APFS Volume Disk (Role):   disk1s2 (Preboot)
    |   |   Name:                      Preboot (Case-insensitive)
    |   |   Mount Point:               Not Mounted
    |   |   Capacity Consumed:         23650304 B (23.7 MB)
    |   |   FileVault:                 No
    |   |
    |   +-> Volume disk1s3 FD9795DB-502A-4234-AD25-CAC5DC529D4C
    |   |   ---------------------------------------------------
    |   |   APFS Volume Disk (Role):   disk1s3 (Recovery)
    |   |   Name:                      Recovery (Case-insensitive)
    |   |   Mount Point:               Not Mounted
    |   |   Capacity Consumed:         507379712 B (507.4 MB)
    |   |   FileVault:                 No
    |   |
    |   +-> Volume disk1s4 24A3A499-3198-4C08-8566-4F1CBBCCC463
    |       ---------------------------------------------------
    |       APFS Volume Disk (Role):   disk1s4 (VM)
    |       Name:                      VM (Case-insensitive)
    |       Mount Point:               Not Mounted
    |       Capacity Consumed:         3221250048 B (3.2 GB)
    |       FileVault:                 No
    |
    +-- Container disk3 DE001FA1-3FF2-4F81-B9DF-14D7625570CF
        ====================================================
        APFS Container Reference:     disk3
        Size (Capacity Ceiling):      499898105856 B (499.9 GB)
        Capacity In Use By Volumes:   27490463744 B (27.5 GB) (5.5% used)
        Capacity Not Allocated:       472407642112 B (472.4 GB) (94.5% free)
        |
        +-< Physical Store disk2s2 C586BA38-5AC2-4FA5-B915-6A5AB52530FF
        |   -----------------------------------------------------------
        |   APFS Physical Store Disk:   disk2s2
        |   Size:                       499898105856 B (499.9 GB)
        |
        +-> Volume disk3s1 3278C2B2-F51A-42BB-91D5-D451BC6A9DF6
        |   ---------------------------------------------------
        |   APFS Volume Disk (Role):   disk3s1 (Data)
        |   Name:                      macOS - Data (Case-sensitive)
        |   Mount Point:               /System/Volumes/Data
        |   Capacity Consumed:         7580196864 B (7.6 GB)
        |   FileVault:                 No
        |
        +-> Volume disk3s2 B0CD759B-06AF-4928-90B8-E04E9777CF9F
        |   ---------------------------------------------------
        |   APFS Volume Disk (Role):   disk3s2 (Preboot)
        |   Name:                      Preboot (Case-insensitive)
        |   Mount Point:               Not Mounted
        |   Capacity Consumed:         25210880 B (25.2 MB)
        |   FileVault:                 No
        |
        +-> Volume disk3s3 DE577553-733C-462C-85DB-7FBEE04DAD1B
        |   ---------------------------------------------------
        |   APFS Volume Disk (Role):   disk3s3 (Recovery)
        |   Name:                      Recovery (Case-insensitive)
        |   Mount Point:               Not Mounted
        |   Capacity Consumed:         525926400 B (525.9 MB)
        |   FileVault:                 No
        |
        +-> Volume disk3s4 DAE3C853-521B-426F-A183-BBBD9395F305
        |   ---------------------------------------------------
        |   APFS Volume Disk (Role):   disk3s4 (VM)
        |   Name:                      VM (Case-insensitive)
        |   Mount Point:               /private/var/vm
        |   Capacity Consumed:         8591003648 B (8.6 GB)
        |   FileVault:                 No
        |
        +-> Volume disk3s5 B9CFDA60-3D07-4499-9DE5-9F0D1ADFC63F
            ---------------------------------------------------
            APFS Volume Disk (Role):   disk3s5 (System)
            Name:                      macOS (Case-sensitive)
            Mount Point:               /
            Capacity Consumed:         10606632960 B (10.6 GB)
            FileVault:                 No

**`diskutil info disk0` —**

    Device Identifier:         disk0
    Device Node:               /dev/disk0
    Whole:                     Yes
    Part of Whole:             disk0
    Device / Media Name:       KINGSTON SA400S37480G
    Volume Name:               Not applicable (no file system)
    Mounted:                   Not applicable (no file system)
    File System:               None
    Content (IOContent):       GUID_partition_scheme
    OS Can Be Installed:       No
    Media Type:                Generic
    Protocol:                  SATA
    SMART Status:              Verified
    Disk Size:                 480.1 GB (480103981056 Bytes) (exactly 937703088 512-Byte-Units)
    Device Block Size:         512 Bytes
    Read-Only Media:           No
    Read-Only Volume:          Not applicable (no file system)
    Device Location:           Internal
    Removable Media:           Fixed
    Solid State:               Yes
    Virtual:                   No
    Hardware AES Support:      No

**`sudo fsck_apfs -n /dev/disk0` (and likewise for `disk0s2`, `disk1` and `disk1s1`) —**

    ** Checking the container superblock.
    ** Checking the EFI jumpstart record.
    ** Checking the space manager.
    ** Checking the space manager free queue trees.
    ** Checking the object map.
    ** Checking volume.
    ** Checking the APFS volume superblock.
    ** The volume macOS was formatted by diskmanagementd (945.241.4) and last modified by apfs_kext (1412.11.7).
    ** Checking the object map.
    error: (oid 0xd31c1) om: btn: found zeroed-out block
       Object map is invalid.
    ** The volume /dev/disk0 could not be verified completely.

**So is there any chance that I can get my data back?**