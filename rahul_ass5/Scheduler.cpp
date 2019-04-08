#include "Library.h"
using namespace std;
#define PAGE 1
#define FRAME 0

// key_t key;
// int shmBuf1id;
// int *buf1Ptr;

MESSAGE_BUFFER MQ;
key_t key;
int msgid;
int create_ready_queue_MQ1(char *MQ1){
    key=ftok(MQ1,65);
    msgid=msgget(key,0666|IPC_CREAT);
    return msgid;
}

int create_message_queue_MQ2(char *MQ2){
    key=ftok(MQ2,65);
    msgid=msgget(key,0666|IPC_CREAT);
    return msgid;
}


int main(int argc,char *argv[]){
   //freopen ("Scheduler_out.txt", "w", stdout);
    //printf("here\n");
    cout<<"in Scheduler\n";
    char *MQ1=argv[0];
    char *MQ2=argv[1];
    // int ID_MQ1=create_ready_queue_MQ1(MQ1);
    int ID_MQ1 = atoi(MQ1);
    cout<<"(Scheduler) ID_MQ1: "<<ID_MQ1<<endl;
    // int ID_MQ2=create_message_queue_MQ2(MQ2);
    int ID_MQ2 = atoi(MQ2);
    cout<<"(Scheduler) ID_MQ2: "<<ID_MQ2<<endl;
    // cout<<"ID_MQ2 is:"<<ID_MQ2<<endl;
    while(1){
        // Scheduling operation to be done,waiting on the ready queue
        // cout<<"(Scheduler) waiting to recv from ID_MQ1\n";
        msgrcv(ID_MQ1,&MQ,sizeof(MQ),4,0);  
        cout<<"(Scheduler) MQ_1 type: 4 received msg : "<<MQ.text<<", type: "<<MQ.message_type<<endl;
        int pid_process;
        try{
            //cout<<"DEBUG:"<<stoi(string(MQ.text),NULL,10)<<endl;
            pid_process=stoi(string(MQ.text),NULL,10);
        }
        catch(int c){
            pid_process=0;
            continue;
        }
        if(pid_process==-1)break;
        //printf("The Process to be scheduled is :%d\n",pid_process);
        
        // Signalling the process to start
        cout<<"(Scheduler) waking up: "<<pid_process<<endl;
        kill(pid_process,SIGUSR1);
        
        // Keeping the MMU queue watch
        msgrcv(ID_MQ2,&MQ,sizeof(MQ),10,0);
        //cout<<"Got from MMU "<<string(MQ.text)<<endl;
        // After the first page fault the message goes to sleep, so we have to wake it up again
        
        if(strcmp(MQ.text,"PAGE_FAULT_HANDLED")==0){
            // Page fault handled so should again queue the process again,so adding again to message queue
            kill(pid_process,SIGUSR2);
            strcpy(MQ.text,to_string(pid_process).c_str());
            MQ.message_type=4;
            msgsnd(ID_MQ1,&MQ,sizeof(MQ),0);
        }
        //cout<<string(MQ.text)<<endl;   
        // If terminated is received then no need of doing all of this, just schedule next process

    }
    //Sprintf("done");
    // printf("Scheduler called!\n");
    // printf("The data entered is :");
    return 0;
}