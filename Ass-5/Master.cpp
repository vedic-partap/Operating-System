#include <bits/stdc++.h>
#include <pthread.h>
#include <signal.h>
#include <sys/shm.h>
#include <ctime>
#include <unistd.h> 

using namespace std;

struct pt_entry{
	int f_no;
	int status;
};

void initialize_pt(struct pt_entry *pt, int len){
	for(int i=0;i<len;i++)
		pt[i].f_no = -1;
}

struct msg_buff{
	long mtype;
	pid_t mpid;
};

struct process_p{
	pid_t pid;
	int index;
	int npr;
};

int rand_bet(int a, int b){
	return rand()%(b-a+1)+a;
}

string get_prs(int l, int m){
	string ans = "";
	for(int i=0;i<l;i++){
		string curr_v = to_string(rand()%m);
		strcat(ans, curr_v);
		strcat(ans, ",");
	}
	return ans;
}


int scheduler_pid, mmu_pid;
void terminate_all(){
	kill(scheduler_pid, SIGINT);
	kill(mmu_pid, SIGINT);
	exit(0);
}

int main(){
	int k, m, f, s;
	cout<<"Total no of processes: ";
	cin>>k;
	cout<<"\nVirtual address space: ";
	cin>>m;
	cout<<"\nPhysical address space: ";
	cin>>f;
	cout<<"\nSize of the TLB: ";
	cin>>s;

	key_t SM1_key = ftok("shmfile1",65); 
	int SM1 = shmget(SM1_key, k*m*sizeof(pt_entry), 0666|IPC_CREAT);
	struct pt_entry *pt = (struct pt_entry*)shmat(SM1, (void *)0, 0);
	initialize_pt(pt, k*m);

	key_t SM2_key = ftok("shmfile2", 65);
	int SM2 = shmget(SM2_key, s*sizeof(int), 0666|IPC_CREAT);
	int *ffl = (int*)shmat(SM2, (void*)0, 0);
	for(int i=0;i<s;i++)
		ffl[i] = 1;

	key_t MQ1_key = ftok("progfile", 65);
	int MQ1 = msgget(MQ1_key, 0666|IPC_CREAT);

	key_t MQ2_key = ftok("progfile1", 65);
	int MQ2 = msgget(MQ2_key, 0666|IPC_CREAT);

	key_t MQ3_key = ftok("progfile2", 65);
	int MQ3 = msgget(MQ3_key, 0666|IPC_CREAT);

	int f_scheduler = fork();
	if(f_scheduler==0){
		execlp("./scheduler", "./scheduler", to_string(MQ1), to_string(MQ2), (char*)NULL);
	}
	scheduler_pid = f_scheduler;

	int f_mmu = fork();
	if(f_mmu==0){
		execlp("./mmu", "./mmu", to_string(MQ2), to_string(MQ3), to_string(SM1), to_string(SM2), (char*)NULL);
	}
	mmu_pid = f_mmu;

	key_t process_page = ftok("progfile3", 65);
	int process_page_id = shmget(process_page, k*sizeof(process_p), 0666|IPC_CREAT);
	struct process *p_details = (struct process_p*)shmat(process_page_id, (void*)0,0);

	for(int i=0;i<k;i++){
		int mi = rand_bet(1,m);
		int l = rand_bet(2*mi, 10*mi);
		strcpy(Ri, get_prs(l,m));

		int p_i = fork();
		if(p_i==0) {
			execlp("./process", "./process", Ri, to_string(MQ1), to_string(MQ3), (char*)NULL);
		}

		p_details[i].pid = p_i;
		p_details[i].index = i;
		p_details[i].npr = mi;

		usleep(250*1000);
	}

	signal(SIGUSR1, terminate_all);
	pause();

	return 0;
}