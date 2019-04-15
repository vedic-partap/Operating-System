// Part 1 --------------------------------------------------
#ifndef _myfs
#define _myfs

#include <bits/stdc++.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <bitset>
#include <semaphore.h>
using namespace std;

const int MAX_NO_BLOCKS = 1024;
const int MAX_DIRECTORY = 10;
const int MAX_FILE = 20;
const int SUPER_BLOCK_MAX_SIZE = 1000;

int cwd;
char *myfs_mem;

struct my_file{
	string name;
	int file_size;
	int mode;
	int start_block;
	int seek;
};

struct super_block_st{
	int block_size;
	int fs_size;
	int no_blocks;
	bitset <MAX_NO_BLOCKS> bit_vector;
	int no_files;
	my_file files[MAX_FILE];
	my_file *fd_t[MAX_FILE];
	int fd_count;
};

struct fat_entry{
	int is_free;
	int next;
};

struct block_1{
	fat_entry FAT[MAX_NO_BLOCKS];
};

int create_myfs(int fs_size, int block_size) {

	if(fs_size<SUPER_BLOCK_MAX_SIZE+sizeof(block_1)) {
		cout<<"INPUT SIZE TOO SMALL! \n";
		return -1;
	}

	myfs_mem = (char *)malloc(fs_size);
	if(myfs_mem==NULL) {
		cout<<"COULDN'T ALLOCATE MEMORY \n";
		return -1;
	}

	super_block_st *sb = (super_block_st *)myfs_mem;

	sb->fs_size = fs_size;
	sb->block_size = block_size;
	sb->no_blocks = (fs_size- SUPER_BLOCK_MAX_SIZE + sizeof(block_1))/block_size+2;

	for(int i=0;i<MAX_NO_BLOCKS;i++)
		sb->bit_vector[i] = 0;

	for(int i=0;i<MAX_FILE;i++) {
		sb->files[i].mode = 'n';
		sb->files[i].seek = 0;
		sb->files[i].file_size = 0;
	}

	sb->no_files = 0;
	sb->fd_count = 0;
	cwd = -1;
	block_1 * block1_ptr = (block_1 *)(myfs_mem+SUPER_BLOCK_MAX_SIZE);
	for(int i=0;i<MAX_NO_BLOCKS;i++)
		block1_ptr->FAT[i].next	 = -1;
	return 1;
}

int my_open(string file_name){    // returns fd
	// cout<<"[MY_OPEN] file_name = "<<file_name<<endl;
	super_block_st *sb = (super_block_st *)myfs_mem;

	my_file *files;
	int *no_files;

	files = (sb->files);
	no_files = &(sb->no_files);

	for(int i=0;i<*no_files;i++) {
		if(strcmp(file_name.c_str(), (files[i].name).c_str())==0) { //there exists a file
			sb->fd_count++;
			sb->fd_t[sb->fd_count] = &files[i];
			return sb->fd_count;
		}
	}

	// cout<<"[MY_OPEN] no_files = "<<*no_files<<endl;
	files[*no_files].name =file_name;
	files[*no_files].file_size = 0;
	files[*no_files].start_block = -1;
	files[*no_files].mode = 'o';

	(*no_files)++;

	(sb->fd_count)++;
	// cout<<"[MY_OPEN] after assigning file, fd_count = "<<sb->fd_count<<endl;
	// cout<<"[MY_OPEN] files[*no_files-1].name = "<< files[*no_files-1].name<<endl;
	// (sb->fd_t)[sb->fd_count] = files[(*no_files)-1];
	sb->fd_t[sb->fd_count] = &files[(*no_files)-1];
	// cout<<"[MY_OPEN] returning value "<<endl;
	return sb->fd_count;
}

int my_close(int fd) {
	super_block_st *sb = (super_block_st *)myfs_mem;
	
	if(sb->fd_t[fd]->mode=='n'){
		cout<<"INVALID FILE DESCRIPTOR! \n";
		return -1;
	}
	sb->fd_t[fd]->mode = 'n';
	return 1;
}

int my_read(int fd, int nbytes, char *buf){
	super_block_st *sb = (super_block_st *)myfs_mem;
	block_1 * block1_ptr = (block_1 *)(myfs_mem+SUPER_BLOCK_MAX_SIZE);

	if(sb->fd_t[fd]->mode=='n'){
		cout<<"INVALID FILE DESCRIPTOR! \n";
		return -1;
	}

	int curr_block = sb->fd_t[fd]->start_block;
	int file_size = sb->fd_t[fd]->file_size;

	int seek = sb->fd_t[fd]->seek;
	while(seek>=sb->block_size) {
		seek -= sb->block_size;
		curr_block = block1_ptr->FAT[curr_block].next;
	}
	if(curr_block==-1) {
		sb->fd_t[fd]->seek = -1;
		return 0;
	}

	// cout<<"[MY_READ] fd = "<<fd<<", curr_block = "<<curr_block<<endl;
	char *temp;
	int bytes_read = 0, to_be_read = min(file_size - sb->fd_t[fd]->seek, nbytes);
	// cout<<"[MY_READ] to_be_read = "<<to_be_read<<endl;

	while(bytes_read<to_be_read) {
		temp = (char *)(myfs_mem+SUPER_BLOCK_MAX_SIZE+sizeof(block_1)+(sb->block_size)*(curr_block-2));
		int bytes_here = seek;
		while(bytes_read<to_be_read) {
			if(bytes_here>=sb->block_size) {
				curr_block = block1_ptr->FAT[curr_block].next;
			}

			buf[bytes_read] = temp[bytes_here];
			bytes_read++;
			bytes_here++;
		}
	}

	sb->fd_t[fd]->seek += bytes_read;
	// cout<<"seek = "<<sb->fd_t[fd]->seek<<endl;
	if(file_size == sb->fd_t[fd]->seek) {
		sb->fd_t[fd]->seek = -1;
	}
	// cout<<"in my_read, buf = "<<buf<<", temp = "<<temp<<"bytes_read = "<<bytes_read<<endl;
	return bytes_read;
}

int get_free_block(int curr_block){
	super_block_st *sb = (super_block_st *)myfs_mem;
	block_1 * block1_ptr = (block_1 *)(myfs_mem+SUPER_BLOCK_MAX_SIZE);

	int alloted_block = -1;
	for(int i=2;i<sb->no_blocks;i++)
		if(sb->bit_vector[i]==0) {
			alloted_block = i;
			sb->bit_vector[i] = 1;
			break;
		}

	if(curr_block!=-1)
		block1_ptr->FAT[curr_block].next = alloted_block;

	return alloted_block;
}

int my_write(int fd, int nbytes, char *buf) {
	// cout<<"[MY_WRITE] fd = "<<fd<<" \n";
	// cout<<"[MY_WRITE] nbytes = "<<nbytes<<endl;
	super_block_st *sb = (super_block_st *)myfs_mem;
	block_1 * block1_ptr = (block_1 *)(myfs_mem+SUPER_BLOCK_MAX_SIZE);

	if(sb->fd_t[fd]->mode=='n'){
		cout<<"INVALID FILE DESCRIPTOR! \n";
		return -1;
	}

	int curr_block = sb->fd_t[fd]->start_block;
	int file_size = sb->fd_t[fd]->file_size;
	int rem_bytes_to_fill = 0;

	// cout<<"[MY_WRITE] got start_block = "<<curr_block<<" file_size = "<<file_size<<endl;

	if(curr_block == -1) {
		curr_block = get_free_block(-1);
		file_size = 0;
		rem_bytes_to_fill = sb->block_size;
		sb->fd_t[fd]->start_block = curr_block;
	}
	else {
		while(block1_ptr->FAT[curr_block].next!=-1) {
			curr_block = block1_ptr->FAT[curr_block].next;
			file_size -= sb->block_size;
		}
		rem_bytes_to_fill = sb->block_size - file_size;
	}

	// cout<<"[MY_WRITE] after check, start_block = "<<sb->fd_t[fd]->start_block<<", curr_block = "<<curr_block<<endl;
	// cout<<"[MY_WRITE] rem_bytes_to_fill = "<<rem_bytes_to_fill<<endl;
	int bytes_written = 0;
	char *temp;
	while(nbytes>0) {
		int already_filled = sb->block_size - rem_bytes_to_fill;
		temp = (char *)(myfs_mem+SUPER_BLOCK_MAX_SIZE+sizeof(block_1)+(sb->block_size)*(curr_block-2));
		while(rem_bytes_to_fill>0 && nbytes>0) {
			temp[already_filled] = buf[bytes_written];
			already_filled++;
			rem_bytes_to_fill--;
			nbytes--;
			bytes_written++;
		}
		if(rem_bytes_to_fill>0)
			break;
		curr_block = get_free_block(curr_block);
		if(curr_block==-1) {
			cout<<"MEMORY FULL! \n";
			return -1;
		}
		rem_bytes_to_fill = sb->block_size;
		already_filled = 0;
	}

	sb->fd_t[fd]->file_size += bytes_written;
	// cout<<"curr_block = "<<curr_block<<", rem_bytes_to_fill = "<<rem_bytes_to_fill<<endl;
	// cout<<"written data, temp = "<<temp<<endl;
	return bytes_written;
}

int my_copy(string file_name) {
	FILE *fp = fopen(file_name.c_str(), "r");
	if(fp==NULL) {
		cout<<"NO SUCH FILE IN DISK (PC)! \n";
		return -1;
	}
	super_block_st *sb = (super_block_st *)myfs_mem;
	int fd = my_open(file_name);
	if(fd==-1)
		return -1;
	char buf[sb->block_size];

	while(!feof(fp))
	{
		bzero(buf,sizeof(buf));
		int nbytes = fread(buf,1,80,fp);
		buf[nbytes] = '\0';
		// cout<<"[MY_COPY] going to write "<<nbytes<<" bytes"<<endl;;
		int x = my_write(fd, nbytes, buf);
		if(x==-1)
			return -1;
	}
	fclose(fp);
	return 1;
}

int is_eof(int fd){
	super_block_st *sb = (super_block_st *)myfs_mem;
	if(sb->fd_t[fd]->seek==-1)
		return 1;
	return  0;
}

int my_cat(string file_name) {
	char buff[100];

	int fd = my_open(file_name);
	if(fd==-1) {
		return -1;
	}

	while(!is_eof(fd)) {
		bzero(buff, sizeof(buff));
		int nbytes = my_read(fd, sizeof(buff), buff);
		cout<<buff;
	}
	return 1;
}

#endif









// Part 2 ------------------------------------------

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

int my_rm(char *filename);

int my_ls();

int my_mkdir(char *dirname);

int my_chdir(char* dirname);

int my_rmdir(char *dirname);

int my_open(char *filename, char mode);

int my_close(int fd);

int my_read(int fd, int nbytes, char *buff);

int my_write(int fd, int nbytes, char *buff);

int eof_myfs(int fd);

#endif

#include "mrfs.h"

#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

char* mrfs_mem;
int cur_dir;
FDTable_t fd_table;
sem_t fd_mutex, cd_mutex, mem_mutex;

void error(std::string msg){
	std::cout << msg << std::endl;
	exit(1);
}

void init_sems(){
	sem_init(&fd_mutex, 0, 1);
	sem_init(&cd_mutex, 0, 1);
	sem_init(&mem_mutex, 0, 1);
}

void init_fdt(){
	sem_wait(&fd_mutex);
	for(int i=0;i<MAX_FD_SZ;i++){
		fd_table.entry[i].node = NULL;
	}
	sem_post(&fd_mutex);
}

void init_dirblock(Directory_t* dir){
	for(int i=0;i<FILES_PER_DIR;i++){
		strcpy(dir->entry[i].filename, "");
		dir->entry[i].ptr = -1;
	}
}

int my_create(int size){
	size *= (1<<20);
	if(size < SB_SZ_B + INODEL_SZ_B + DB_SZ){
		error("Error! Size is too small to construct file system");
		return -1;
	}
	init_sems();
	sem_wait(&mem_mutex);
	mrfs_mem = new char[size];

	SuperBlock_t* sb = (SuperBlock_t*)mrfs_mem;
	sb->total_sz = size;
	sb->max_inodes = INODE_MAX;
	sb->cur_inodes = 1; // including root by default
	sb->max_db = size/DB_SZ + ((size%DB_SZ == 0) ? 0:1);
	sb->cur_db = SB_DB + INODEL_DB + 1;
	for(int i=0;i<INODE_BM_SZ;i++)
		sb->inode_bm[i] = 0;
	sb->inode_bm[0] = 0x80;
	int i = 0;
	for(;i<(SB_DB+INODEL_DB+1)/8;i++)
		sb->db_bm[i] = 0xff;
	sb->db_bm[i] = ~((1<<(8-(SB_DB+INODEL_DB+1)%8))-1);

	INodeList_t* inl = (INodeList_t*)(mrfs_mem + SB_SZ_B);
	inl->node[0].ftype = 1;
	inl->node[0].sz = 0;
	inl->node[0].last_modified = time(NULL);
	inl->node[0].last_read = time(NULL);
	inl->node[0].permissions = {6, 6, 6};
	inl->node[0].ptr[0] = sb->cur_db - 1;
	for(i=1;i<10;i++)
		inl->node[0].ptr[i] = -1;

	Directory_t* root_block = (Directory_t*)(mrfs_mem + SB_SZ_B + INODEL_SZ_B);
	init_dirblock(root_block);
	init_fdt();

	sem_post(&mem_mutex);
	return 0;
}

int get_free_inode(){
	SuperBlock_t* sb = (SuperBlock_t*)mrfs_mem;
	for(int i=0;i<INODE_BM_SZ;i++)
		if(sb->inode_bm[i] != (unsigned char)0xff){
			for(int j=7;j>=0;j--){
				if((sb->inode_bm[i]>>j)%2 == 0){
					sb->cur_inodes ++;
					sb->inode_bm[i] += (1<<j);
					return 8*i + (7-j);
				}
			}
		}
	return -1;
}

int get_free_datablock(){
	SuperBlock_t* sb = (SuperBlock_t*)mrfs_mem;
	for(int i=0;i<DB_BM_SZ;i++)
		if(sb->db_bm[i] != (unsigned char)0xff){
			for(int j=7;j>=0;j--){
				if((sb->db_bm[i]>>j)%2 == 0){
					sb->cur_db ++;
					sb->db_bm[i] += (1<<j);
					return 8*i + (7-j);
				}
			}
		}
	return -1;
}

int* get_inode_ptr(INode_t* inode, int idx){
	if(idx < PTR_MAX)
		return (inode->ptr + idx);
	idx -= PTR_MAX;
	if(idx < IPTR_MAX){
		int* ptr = (int*)(mrfs_mem + DB_SZ*inode->ptr[8] + idx*4);
		return ptr;
	}
	idx -= IPTR_MAX;
	int ptr_idx = idx/IPTR_MAX;
	int* iptr = (int*)(mrfs_mem + DB_SZ*inode->ptr[9] + ptr_idx*4);
	int* ptr = (int*)(mrfs_mem + DB_SZ*(*iptr) + (idx%IPTR_MAX)*4);
	return ptr;
}

int get_free_dirblock(Directory_t* dir){
	for(int i=0;i<FILES_PER_DIR;i++){
		if(dir->entry[i].ptr == -1)
			return i;
	}
	return -1;
}

void create_dir_entry(INode_t* dir, char* filename, int fptr){
	int idx = dir->sz / FILES_PER_DIR;
	int* ptr = get_inode_ptr(dir, idx);
	if(*ptr == -1){
		// allocate new block
		*ptr = get_free_datablock();
		Directory_t* _dir = (Directory_t*)(mrfs_mem + (*ptr)*DB_SZ);
		init_dirblock(_dir);
	}
	Directory_t* _dir = (Directory_t*)(mrfs_mem + (*ptr)*DB_SZ);
	int id = get_free_dirblock(_dir);
	strcpy(_dir->entry[id].filename, filename);
	_dir->entry[id].ptr = fptr;
	dir->sz ++;
	dir->last_modified = time(NULL);
}

int my_copy(char* source, char* dest){
	SuperBlock_t* sb = (SuperBlock_t*)mrfs_mem;
	fstream file(source, ios::in|ios::binary);
	if(!file.is_open()){
		cout << "Error! Could not open file " << source << endl;
		return -1;
	}

	file.seekg(0, ios::end);
	int fsz = file.tellg();
	file.seekg(0, ios::beg);
	if(fsz > (sb->max_db-sb->cur_db)*DB_SZ){
		cout << "File size exceeds remaining memory!" << endl;
		return -1;
	}

	int node_idx = get_free_inode();
	if(node_idx == -1){
		cout << "Cannot create new file" << endl;
		return -1;
	}
	sem_wait(&mem_mutex);
	INode_t* inode = &((INodeList_t*)(mrfs_mem+SB_SZ_B))->node[node_idx];
	inode->ftype = 0;
	inode->sz = fsz;
	inode->last_modified = time(NULL);
	inode->last_read = time(NULL);
	inode->permissions = {6, 6, 4};
	//setup ptrs
	int ptrs_req = fsz/DB_SZ + 1;
	if(ptrs_req > 0){
		ptrs_req -= PTR_MAX;
	}
	if(ptrs_req > 0){
		inode->ptr[8] = get_free_datablock();
		ptrs_req -= IPTR_MAX;
	}
	if(ptrs_req > 0){
		inode->ptr[9] = get_free_datablock();
		int *iptr = (int*)(mrfs_mem+DB_SZ*inode->ptr[9]);
		while(ptrs_req > 0){
			*iptr = get_free_datablock();
			ptrs_req -= IPTR_MAX;
			iptr ++;
		}
	}
	int idx = 0;
	while(!file.eof()){
		int db = get_free_datablock();
		if(db == -1){
			cout << "Memory full!" << endl;
			cout << "Possible corruption" << endl;
			return -1;
		}
		file.read(mrfs_mem + DB_SZ*db, DB_SZ);
		int* ptr = get_inode_ptr(inode, idx);
		*ptr = db;
		idx ++;
	}
	
	// setup current directory
	INode_t* cur_inode = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	create_dir_entry(cur_inode, dest, node_idx);
	file.close();
	sem_post(&mem_mutex);
	return 0;
}

INode_t* get_file_inode(INode_t* dir, char* filename){
	int count = 0, idx = 0;
	while(count < dir->sz){
		int* ptr = get_inode_ptr(dir, idx);
		Directory_t* node = (Directory_t*)(mrfs_mem + DB_SZ*(*ptr));
		for(int i=0;i<FILES_PER_DIR;i++){
			if(count >= dir->sz)
				break;
			if(node->entry[i].ptr != -1 && !strcmp(node->entry[i].filename, filename)){
				return &(((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[node->entry[i].ptr]);
			}
			if(node->entry[i].ptr != -1){
				count ++;
			}
		}
		idx++;
	}
	return NULL;
}

// idx is diskblock index
// should be used only for datablock removal
void clear_db(int idx){
	SuperBlock_t* sb = (SuperBlock_t*)mrfs_mem;
	if(idx < SB_DB){
		cout << "Cannot remove a superblock!" << endl;
		return;
	}
	if(idx < SB_DB + INODEL_DB){
		cout << "Trying to remove an inode list block" << endl;
	}
	else{
		sb->cur_db --;
		sb->db_bm[idx/8] -= 1<<(7 - idx%8);
	}
}

void rem_dir_entry(INode_t* dir, char *filename){
	int count = 0, done = 0;
	int idx = 0;
	while(!done && count < dir->sz){
		int* ptr = get_inode_ptr(dir, idx);
		Directory_t* node = (Directory_t*)(mrfs_mem + DB_SZ*(*ptr));
		for(int i=0;i<FILES_PER_DIR;i++){
			if(count >= dir->sz)
				break;
			if(node->entry[i].ptr != -1 && !strcmp(node->entry[i].filename, filename)){
				node->entry[i].ptr = -1;
				done = 1;
				break;
			}
			if(node->entry[i].ptr != -1){
				count ++;
			}
		}
		idx++;
	}
	dir->sz --;
}

int my_rm(char *filename){
	INode_t* cur_inode = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	INode_t* node = get_file_inode(cur_inode, filename);
	if(node == NULL){
		cout << "No file named " << filename << " in current directory" << endl;
		return -1;
	}
	sem_wait(&mem_mutex);
	int sz = node->sz;
	int idx = 0;
	while(sz > 0){
		clear_db(*get_inode_ptr(node, idx));
		sz -= DB_SZ;
		idx ++;
	}
	// remove diptr blocks
	sz = node->sz;
	if(sz > DB_SZ*(PTR_MAX + IPTR_MAX)){
		sz -= DB_SZ*(PTR_MAX + IPTR_MAX);
		int* n = (int*)(mrfs_mem + DB_SZ*node->ptr[9]);
		while(sz > 0){
			clear_db(*n);
			n ++;
			sz -= IPTR_MAX*DB_SZ;
		}
		clear_db(node->ptr[9]);
	}
	//remove iptr block
	sz = node->sz;
	if(sz > DB_SZ*PTR_MAX){
		clear_db(node->ptr[8]);
	}
	// remove inode
	SuperBlock_t* sb = (SuperBlock_t*)mrfs_mem;
	int ind = node - (INode_t*)(mrfs_mem + SB_SZ_B);
	sb->inode_bm[ind/8] -= 1<<(7-ind%8);
	sb->cur_inodes --;

	// update current directory
	INode_t* dir = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	rem_dir_entry(dir, filename);
	sem_post(&mem_mutex);
	return 0;
}

int my_ls(){
	sem_wait(&mem_mutex);
	INode_t* cur_inode = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	int rem_count = cur_inode->sz;
	int idx = 0;
	while(rem_count){
		Directory_t* dir = (Directory_t*)(mrfs_mem + DB_SZ * (*get_inode_ptr(cur_inode, idx)));
		for(int i=0;i<FILES_PER_DIR;i++){
			if(dir->entry[i].ptr != -1){
				cout << dir->entry[i].filename << "\t ";
				cout << endl;
				rem_count --;
			}
		}
		idx ++;
	}
	sem_post(&mem_mutex);
	return 0;
}

int my_mkdir(char *dirname){
	INode_t* dir = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	int node_idx = get_free_inode();
	if(node_idx == -1)
		return -1;
	sem_wait(&mem_mutex);
	INode_t* inode = &((INodeList_t*)(mrfs_mem+SB_SZ_B))->node[node_idx];
	inode->ftype = 1;
	inode->sz = 0;
	inode->last_modified = time(NULL);
	inode->last_read = time(NULL);
	inode->permissions = {6,6,6};
	for(int i=0;i<10;i++)
		inode->ptr[i] = -1;
	create_dir_entry(dir, dirname, node_idx);
	sem_post(&mem_mutex);
	return 0;
}

int my_chdir(char *dirname){
	INode_t* dir = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	INode_t* fnode = get_file_inode(dir, dirname);
	if(fnode == NULL){
		cout << "No file " << dirname << " exists" << endl;
		return -1;
	}
	sem_wait(&cd_mutex);
	cur_dir = fnode - (INode_t*)(mrfs_mem + SB_SZ_B);
	sem_post(&cd_mutex);
	return 0;
}

int my_rmdir(char *dirname){
	INode_t* dir = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	INode_t* fnode = get_file_inode(dir, dirname);
	if(fnode == NULL){
		cout << "No directory " << dirname << " exists" << endl;
		return -1;
	}
	if(fnode->sz != 0){
		cout << "Directory " << dirname << " is not empty!" << endl;
		return -1;
	}
	sem_wait(&mem_mutex);
	for(int i=0;i<8;i++)
		if(fnode->ptr[i] != -1){
			clear_db(fnode->ptr[i]);
		}
	// remove inode
	SuperBlock_t* sb = (SuperBlock_t*)mrfs_mem;
	int ind = fnode - (INode_t*)(mrfs_mem + SB_SZ_B);
	sb->inode_bm[ind/8] -= 1<<(7-ind%8);
	sb->cur_inodes --;
	// remove entry fom current directory
	rem_dir_entry(dir, dirname);
	sem_post(&mem_mutex);
	return 0;
}

// creates emoty file in current directory
INode_t* create_empty_file(char *filename){
	// create new inode and point fnode to it
	int idx = get_free_inode();
	INode_t* fnode = &(((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[idx]);
	fnode->ftype = 0;
	fnode->sz = 0;
	fnode->last_modified = time(NULL);
	fnode->last_read = time(NULL);
	fnode->permissions = {6,6,6};
	// update current directory
	INode_t* cur_inode = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	create_dir_entry(cur_inode, filename, idx);
	return fnode;
}

int my_open(char* filename, char mode){
	int ind;
	for(ind=0;ind<MAX_FD_SZ;ind++)
		if(fd_table.entry[ind].node == NULL)
			break;
	if(ind == MAX_FD_SZ){
		cout << "Error! FD Table full" << endl;
		return -1;
	}
	sem_wait(&fd_mutex);
	INode_t* dir = &((INodeList_t*)(mrfs_mem + SB_SZ_B))->node[cur_dir];
	INode_t* fnode = get_file_inode(dir, filename);

	if(fnode == NULL){
		if(mode == 'r'){
			cout << "Cannot open " << filename << " as it does not exist" << endl;
			return -1;
		}
		else if(mode == 'w'){
			fnode = create_empty_file(filename);
		}
	}
	else{
		if(mode == 'w'){
			sem_post(&fd_mutex);
			my_rm(filename);
			sem_wait(&fd_mutex);
			fnode = create_empty_file(filename);
		}
	}
	fd_table.entry[ind].loc = 0;
	fd_table.entry[ind].node = fnode;
	fd_table.entry[ind].mode = mode;
	sem_post(&fd_mutex);
	return ind;
}

int my_close(int fd){
	if(fd_table.entry[fd].node == NULL)
		return -1;
	sem_wait(&fd_mutex);
	fd_table.entry[fd].node = NULL;
	sem_post(&fd_mutex);
	return 0;
}

int my_read(int fd, int nbytes, char *buff){
	if(fd_table.entry[fd].mode == 'w'){
		cout << "Error! File opened in write mode" << endl;
		return -1;
	}
	sem_wait(&fd_mutex);
	sem_wait(&mem_mutex);
	int* sz = &fd_table.entry[fd].loc;
	int idx = *sz / DB_SZ, offset = *sz % DB_SZ;
	int filed = 0, fsz = fd_table.entry[fd].node->sz - *sz;
	if(offset){
		int cpsz = min(min(nbytes, DB_SZ - offset), fsz);
		memcpy(buff, mrfs_mem + DB_SZ * (*get_inode_ptr(fd_table.entry[fd].node, idx)) + offset, cpsz);
		filed += cpsz;
		nbytes -= cpsz;
		fsz -= cpsz;
		idx ++;
	}
	while(nbytes && fsz){
		int cpsz = min(min(nbytes, DB_SZ), fsz);
		memcpy(buff + filed, mrfs_mem + DB_SZ * (*get_inode_ptr(fd_table.entry[fd].node, idx)), cpsz);
		filed += cpsz;
		nbytes -= cpsz;
		fsz -= cpsz;
		idx ++;
	}
	*sz += filed;
	fd_table.entry[fd].node->last_read = time(NULL);
	sem_post(&fd_mutex);
	sem_post(&mem_mutex);
	return filed;
}

int my_write(int fd, int nbytes, char *buff){
	if(fd_table.entry[fd].mode == 'r'){
		cout << "Error! File opened in read mode" << endl;
		return -1;
	}
	sem_wait(&fd_mutex);
	sem_wait(&mem_mutex);
	int *sz = &fd_table.entry[fd].loc;
	int idx = *sz / DB_SZ, offset = *sz % DB_SZ;
	int filed = 0;
	if(offset){
		int cpsz = min(nbytes, DB_SZ - offset);
		memcpy(mrfs_mem + DB_SZ * (*get_inode_ptr(fd_table.entry[fd].node, idx)) + offset, buff, cpsz);
		filed += cpsz;
		nbytes -= cpsz;
		idx ++;
	}
	while(nbytes){
		int cpsz = min(nbytes, DB_SZ);
		int* ind;
		if(idx == PTR_MAX)
			fd_table.entry[fd].node->ptr[8] = get_free_datablock();
		if(idx == PTR_MAX + IPTR_MAX)
			fd_table.entry[fd].node->ptr[9] = get_free_datablock();
		if(idx >= PTR_MAX + IPTR_MAX){
			if((idx - PTR_MAX - IPTR_MAX)%IPTR_MAX == 0)
				*(int*)(mrfs_mem + DB_SZ * fd_table.entry[fd].node->ptr[9] + 4*(idx - PTR_MAX - IPTR_MAX)/IPTR_MAX) = get_free_datablock();
		}
		ind = get_inode_ptr(fd_table.entry[fd].node, idx);
		*ind = get_free_datablock();
		memcpy(mrfs_mem + DB_SZ * (*ind), buff + filed, cpsz);
		filed += cpsz;
		nbytes -= cpsz;
		idx ++;
	}
	*sz += filed;
	fd_table.entry[fd].node->sz += filed;
	fd_table.entry[fd].node->last_read = time(NULL);
	fd_table.entry[fd].node->last_modified = time(NULL);
	sem_post(&fd_mutex);
	sem_post(&mem_mutex);
	return filed;
}

int eof_myfs(int fd){
	return (fd_table.entry[fd].loc == fd_table.entry[fd].node->sz);
}

int my_cat(char file_name[]) {
	char buff[100];

	int fd = my_open(file_name, 'r');
	if(fd==-1) {
		return -1;
	}

	while(!eof_myfs(fd)) {
		bzero(buff, sizeof(buff));
		int nbytes = my_read(fd, sizeof(buff), buff);
		cout<<buff;
	}
	return 1;
}


