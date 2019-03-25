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

struct msg_buff1{
	long mtype;
	string mtext;
}message_mmu;

int main(int argc, char **argv){
	string MQ1s;
	strcpy(MQ1s, argv[1]);
	int MQ1 = atoi(MQ1s);

	string MQ2s;
	strcpy(MQ2s, argv[2]);
	int MQ2 = atoi(MQ2s);

	msgrcv(MQ1, &message_rq, sizeof(message_rq), 1, 0);
	kill(message_rq.mpid, SIGUSR1);

	while(true){ // how will the scheduler know if all the process has terminated??
		msgrcv(MQ2, &message_mmu, sizeof(message_mmu), 1, 0);
		if(strcmp(message_mmu.mtext, "PAGE FAULT HANDLED")==0) {
			msgsnd(MQ1, &message_rq, sizeof(message_rq), 0);
			msgrcv(MQ1, &message_rq, sizeof(message_rq), 1, 0);
			kill(message_rq.mpid, SIGUSR1);
		}
		else if(strcmp(message_mmu.mtext, "TERMINATED")==0) {
			msgrcv(MQ1, &message_rq, sizeof(message_rq), 1, 0);
			kill(message_rq.mpid, SIGUSR1);
		}
		else {
			cout<<" INVALID MESSAGE FROM MMU, msg: "<<message_mmu.mtext<<endl;
		}
	}

	return 0;
}