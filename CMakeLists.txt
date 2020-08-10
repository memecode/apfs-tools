cmake_minimum_required (VERSION 3.5)

project(apfs)
SET (CMAKE_OSX_ARCHITECTURES "x86_64")
set (CMAKE_CXX_STANDARD 11)

if (UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

file(GLOB_RECURSE HDRS "*.h")

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
include_directories(src/apfs src/apfs/func src/apfs/string src/apfs/struct)

add_executable(apfs-read src/apfs-read.c ${HDRS})
add_executable(apfs-inspect src/apfs-inspect.c ${HDRS})
add_executable(apfs-explore-omap-tree src/apfs-explore-omap-tree.c ${HDRS})
add_executable(apfs-explore-fs-tree src/apfs-explore-fs-tree.c ${HDRS})
add_executable(apfs-search-last-btree-node src/apfs-search-last-btree-node.c ${HDRS})
add_executable(apfs-list src/apfs-list.c ${HDRS})
add_executable(apfs-recover src/apfs-recover.c ${HDRS})