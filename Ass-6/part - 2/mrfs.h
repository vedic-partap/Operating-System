#ifndef MRFS_H
#define MRFS_H

#include <time.h>
#include <iostream>

int get_db_size() {
	int x;
	std::cout<<"Enter block size : ";
	std::cin>>x;
	return x;
}

const int DB_SZ = get_db_size(); // diskblock size in bytes
#define INODE_MAX (1<<8) // max number of inodes
/*
Assuming maximum memory allocation of 256MB
max no of diskblocks = 2^8 * 2^20 / DB_SZ = 2^20
*/
#define DB_MAX (1<<20)
/*
size of inode bitmap in bytes = INODE_MAX / 2^3 = 2^5
*/
#define INODE_BM_SZ (1<<5)
/*
size of diskblock bitmap in bytes = DB_MAX / 2^3 = 2^17
*/
#define DB_BM_SZ (1<<17)

struct SuperBlock_t{
	int total_sz;
	int max_inodes;
	int cur_inodes;
	int max_db;
	int cur_db;
	unsigned char inode_bm[INODE_BM_SZ];
	unsigned char db_bm[DB_BM_SZ];
};

// size of superblock
const int SB_SZ = sizeof(SuperBlock_t);
// number of diskblocks for superblock
const int SB_DB = SB_SZ / DB_SZ + 1;
// size of superblock block
const int SB_SZ_B = SB_DB * DB_SZ;

/*
i => Individual
g => group
o => others
value: 00000rwx;
*/
struct AccPer_t{
	unsigned char i;
	unsigned char g;
	unsigned char o;
};

const int PTR_COUNT = 10;
const int FILENAME_MAX1 = 30;

const int PTR_MAX = (1<<3);
const int IPTR_MAX = (1<<6);
const int DIPTR_MAX = (1<<12);

struct INode_t{
	bool ftype;
	int sz;
	time_t last_modified;
	time_t last_read;
	AccPer_t permissions;
	int ptr[PTR_COUNT];
};

struct INodeList_t{
	INode_t node[INODE_MAX];
};

struct Block_t{
	unsigned char * val = new unsigned char[DB_SZ];
};

struct DirectoryEntry_t{
	char filename[FILENAME_MAX1];
	short int ptr;
};

#define FILES_PER_DIR 8

struct Directory_t{
	DirectoryEntry_t entry[FILES_PER_DIR];
};

#define MAX_FD_SZ 32

struct FDEntry_t{
	int loc;
	INode_t* node;
	char mode;
};

struct FDTable_t{
	FDEntry_t entry[MAX_FD_SZ];
};

// size of InodeList
const int INODEL_SZ = sizeof(INodeList_t);
// number of diskblocks for INodeList
const int INODEL_DB = INODEL_SZ / DB_SZ + 1;
// size of InodeList block
const int INODEL_SZ_B = INODEL_DB * DB_SZ;

// maximum number of diskblocks for data
const int MAX_DATA_DB = DB_MAX - SB_DB - INODEL_DB;

// memory file system pointer
extern char* mrfs_mem;
extern int cur_dir;
extern FDTable_t fd_table;
/*
size denotes the total size of the file system in Mbytes.
Returns -1 on error
*/
int my_create(int size);

int my_copy(char *source, char* dest);

int copy_myfs2pc(char *source, char *dest);

int my_rm(char *filename);

int showfile_myfs(char *filename);

int my_ls();

int my_mkdir(char *dirname);

int my_chdir(char* dirname);

int my_rmdir(char *dirname);

int my_open(char *filename, char mode);

int my_close(int fd);

int my_read(int fd, int nbytes, char *buff);

int my_write(int fd, int nbytes, char *buff);

int eof_myfs(int fd);

int dump_myfs(char *dumpfile);

int restore_myfs(char *dumpfile);

int status_myfs();

int chmod_myfs(char *name, int mode);

#endif