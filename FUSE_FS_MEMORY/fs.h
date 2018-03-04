/*
	gcc -Wall fs.c `pkg-config fuse3 --cflags --libs` -o fs
*/

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>


// File operations
static void *fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
static int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);
static int fs_mkdir(const char *path, mode_t mode);
static int fs_rmdir(const char *path);
static int fs_rename(const char *from, const char *to, unsigned int flags);
static int fs_truncate(const char *path, off_t size, struct fuse_file_info *fi);
static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int fs_open(const char *path, struct fuse_file_info *fi);
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fs_rm(const char *path);


static struct fuse_operations fs_oper = {
	.init       = fs_init,
	.getattr    = fs_getattr,
	.readdir	= fs_readdir,
	.mkdir		= fs_mkdir,
	.rmdir		= fs_rmdir,
	.rename		= fs_rename,
	//.truncate	= fs_truncate,
	.open       = fs_open,
	.create     = fs_create,
	.read       = fs_read,
	.write      = fs_write,
	.unlink			= fs_rm,
};

typedef struct {
	bool used;                  // valid inode or not
	int id;						// ID for the inode
	size_t size;				// Size of the file
	char *data;					// pointer of data block
	bool directory;				// true if its a directory else false
	int last_accessed;			// Last accessed time
	int last_modified;			// Last modified time
	int link_count; 			// 2 in case its a directory, 1 if its a file

} __attribute__((packed, aligned(1))) inode;

typedef struct {
	int magic;
	size_t blocks;
	size_t iblocks;
	size_t inodes;
} __attribute__((packed, aligned(1))) sblock;

typedef struct {
	int id;			// inode id
	inode *in;      // pointer to inode
}OPEN_FILE_TABLE_ENTRY;

typedef struct{
	char *filename;
	inode *file_inode;
}dirent;

// Size of dirent = 16 bytes
// Hence in a 4KB block we can have 256 directory entries

// Macros

#define SBLK_SIZE (1 << 4)
#define BLK_SIZE (1 << 12)

#define N_INODES 100
#define DBLKS_PER_INODE 1
#define DBLKS (DBLKS_PER_INODE * N_INODES)

#define ROUND_UP_DIV(x, y) (((x) + (y) - 1) / (y))

#define FREEMAP_BLKS ROUND_UP_DIV(DBLKS, BLK_SIZE)
#define INODE_BLKS ROUND_UP_DIV(N_INODES*sizeof(inode), BLK_SIZE)

#define FS_SIZE (INODE_BLKS + FREEMAP_BLKS + DBLKS) * BLK_SIZE


#define MAX_NO_OF_OPEN_FILES 10

#define DEBUG


// Helper Functions
int initialise_inodes(inode* i);
int initialise_freemap(int* map);
inode* return_first_unused_inode(inode *i);
int return_offset_of_first_free_datablock(int *freemap);
int isDir(char *path);
void path_to_inode(const char* path, inode **ino);
void allocate_inode(char *path, inode **ino, bool dir);
void print_inode(inode *i);


char *fs;												// The start of the FileSystem in the memory
inode *inodes;											// The start of the inode block
int *freemap;											// The start of the free-map block
char *datablks;											// The start of the data_blocks
OPEN_FILE_TABLE_ENTRY open_table[MAX_NO_OF_OPEN_FILES]; // The open file array

dirent *root_directory; // Address of the block representing the root directory
