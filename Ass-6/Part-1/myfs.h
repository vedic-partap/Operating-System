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