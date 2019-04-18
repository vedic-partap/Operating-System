#include<bits/stdc++.h>
#include<unistd.h>
#include<stdio.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<string.h> 
#include<sys/wait.h> 
#include<sys/shm.h>
#include <stdio.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include<sys/time.h>
#include<signal.h>
#include<sys/ipc.h>
#include<stdlib.h>
#define MAX 100
#define REQUEST_MADE 1
#define REQUEST_FULLFIL 2
#define NO_REQUEST 0

typedef struct _MESSAGE_BUFFER{
    long message_type;
    char text[MAX];
} MESSAGE_BUFFER;
// for a page_table T, and process i, T[i].frame[j] gives the frame_no for the page_no j and T[i].valid[j] gives whether it is
// valid or not
typedef struct _PAGE_TABLE{
	int *valid;
	int *page_no;
	int *frame_no;	
    int *timestamp;
} PAGE_TABLE;

typedef struct _process_node{
	int pid;
	struct _process_node *next;
}process_node;

typedef struct _process_mem_access{
	int valid;
	int page_no;
	int frame_no;
}process_mem_access;

typedef struct{
	int *page_no;
	int *frame_no;
}TLB;
// Timestamp is required to perform LRU page replacement algorithm
typedef struct _FREE_FRAME{
	int isfree;
} FREE_FRAME;

