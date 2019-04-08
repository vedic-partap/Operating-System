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
	int MQ1 = atoi(argv[1]);

	int MQ2 = atoi(argv[2]);

	int k = atoi(argv[3]), no_terminated = 0;

	int master_pid = atoi(argv[4]);

	msgrcv(MQ1, &message_rq, sizeof(message_rq), 1, 0);
	kill(message_rq.mpid, SIGUSR1);

	while(no_terminated<k){ // how will the scheduler know if all the process has terminated??
		msgrcv(MQ2, &message_mmu, sizeof(message_mmu), 1, 0);
		if(strcmp(message_mmu.mtext, "PAGE FAULT HANDLED")==0) {
			msgsnd(MQ1, &message_rq, sizeof(message_rq), 0);
			msgrcv(MQ1, &message_rq, sizeof(message_rq), 1, 0);
			kill(message_rq.mpid, SIGUSR1);
		}
		else if(strcmp(message_mmu.mtext, "TERMINATED")==0) {
			msgrcv(MQ1, &message_rq, sizeof(message_rq), 1, 0);
			kill(message_rq.mpid, SIGUSR1);
			no_terminated++;
		}
		else {
			cout<<" INVALID MESSAGE FROM MMU, msg: "<<message_mmu.mtext<<endl;
		}
	}

	kill(master_pid, SIGUSR1);
	pause();

	return 0;
}