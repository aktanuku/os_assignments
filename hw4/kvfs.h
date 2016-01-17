#ifndef KVFS_H
#define KVFS_H

#define FUSE_USE_VERSION 26L
#define PATH_MAX 1024
#define NAME_SIZE 1024
#define DATA_SIZE 1024
#define SUPERBLOCK_SIZE 10*4096*2048 + 1
#define NUM_FILES 10*4096
#define FNODE_MAGIC   0xDEADBEEF

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/mman.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif


char* mountparent;

typedef struct fnode {
    char name[NAME_SIZE];
    char data[DATA_SIZE];
    uint32_t magic;
} fnode_t;

typedef struct kvfs {
    uint32_t size;
    fnode_t data[0];
} kvfs_t;

#endif
