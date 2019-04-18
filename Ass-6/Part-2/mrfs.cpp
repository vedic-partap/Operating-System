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
	// inl->node[0].permissions = {6, 6, 6};
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
	// inode->permissions = {6,6,6};
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
	// fnode->permissions = {6,6,6};
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
