#include <bits/stdc++.h>
#include <pthread.h>
#include <signal.h>
#include <sys/shm.h>
#include <ctime>
#include <unistd.h> 

using namespace std;

struct msg_buff{
	long mtype;
	pid_t mpid;
}message_rq;

struct msg_buff2{
	long mtype;
	string mtext;
}message_process;

int main(int argc, char **argv){
	string prs;
	strcpy(prs, argv[1]);

	int MQ1 = atoi(argv[2]);
	int MQ3 = atoi(argv[3]);

	int i=0;
	int next_to_exec = -1;
	while(true) {
		if(next_to_exec==-1)
			next_to_exec = get_pno(Ri, &i);
		if(pn==-1){
			message_process.mtype = 1;
			strcpy(message_process.mtext, "-9");
			msgsnd(MQ3, &message_process, sizeof(message_process), 0);
			pause(); // change this "waiting for master to kill this process"
		}
		else {
			message_process.mtype = 1;
			strcpy(message_process.mtext, to_string(next_to_exec));
			msgsnd(MQ3, &message_process, sizeof(message_process), 0);
		}
		msgrcv(MQ3, &message_process, sizeof(message_process), 1, 0);

		int msg_recv = atoi(message_process.mtext);
		if(msg_recv>=0) {
			next_to_exec = -1;
		}
		else if(strcmp(message_process.mtext, "-1")==0) {
			pause();
		}
		else if(strcmp(message_process.mtext, "-2")==0) {
			exit(1);
		}
		else {
			cout<<" INVALID MESSAGE FROM MMU, msg: "<<message_process.mtext<<endl;
		}

	}
	return 0;
}