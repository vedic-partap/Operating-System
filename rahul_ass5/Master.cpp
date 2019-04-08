#include "Library.h"
using namespace std;

char *str[MAX],command[MAX];
int k,m,f,s;
PAGE_TABLE *page_table;
FREE_FRAME *free_frame_list;

MESSAGE_BUFFER MQ;

void init_str(){
    for(int i=0;i<6;i++)
        str[i]=new char[1000];
}

string Generate_Page_Ref_String(int no_of_pages,int page_ref_str_len){
    string ans="";
    default_random_engine  generator;
    double mu_page_ref=no_of_pages/2.0,sigma_page_ref=1.0;
    for(int i=0;i<page_ref_str_len;i++){
        normal_distribution<double> distribution(mu_page_ref,sigma_page_ref);
        double val=distribution(generator);
        if(int(val)>=no_of_pages){
            val = (int)no_of_pages-1;
        }
        if(int(val)<0){
            val = 0;
        }
        ans.append(to_string((int)val)+((i==page_ref_str_len-1)?"":","));
        mu_page_ref=val;
    }
    return ans;
}

int create_shared_page_table(){
    key_t  key=ftok("SM1",65); 
    int shmid=shmget(key,sizeof(PAGE_TABLE[MAX]),0666|IPC_CREAT);
    
    if(shmid<0){
        printf("Creation of shared page table file error!!\n");
        return -1;
    }
    
    page_table=(PAGE_TABLE*)shmat(shmid,NULL,0);
    for(int i=0;i<k;i++){
        page_table[i].valid=new int[m+1];
        page_table[i].frame_no=new int[m+1];
        page_table[i].timestamp=new int[m+1];
        page_table[i].page_no=new int[m+1];
    }
    memset(page_table,-1,sizeof page_table);
    return shmid; 
}
int create_free_frame_list(){
    key_t  key=ftok("SM2",65); 
    int shmid=shmget(key,MAX*sizeof(FREE_FRAME),0666|IPC_CREAT);
    if(shmid<0){
        printf("Creation of shared free frame list file error!!\n");
        return -1;
    }
    free_frame_list=(FREE_FRAME*)shmat(shmid,NULL,0);
    for(int i=0;i<MAX;i++){
        free_frame_list[i].isfree=0;
    }
    
    return shmid; 
}
int create_ready_queue_MQ1(){
    key_t key;int msgid; 
    key = ftok("MQ1", 65); 

    msgid = msgget(key, 0666 | IPC_CREAT); 
    return msgid;
    //printf("Write Data : "); 
    //gets(message.mesg_text); 
  
    // msgsnd to send message 
    //msgsnd(msgid, &message, sizeof(message), 0); 
}
int create_Scheduler_MMU_MQ2(){
    key_t key;int msgid; 
    key = ftok("MQ2", 65); 

    msgid = msgget(key, 0666 | IPC_CREAT); 
    return msgid;
}
int create_Process_MMU_MQ3(){
    key_t key;int msgid; 
    key = ftok("MQ3", 65); 
    msgid = msgget(key, 0666 | IPC_CREAT); 
    return msgid;
}
int create_notification_Scheduler_Master(){
    key_t key;int msgid; 
    key = ftok("notification_sched_master", 65); 
    msgid = msgget(key, 0666 | IPC_CREAT); 
    return msgid;
}
int main(){
    // Taking inputs to the program
    cout<<"Enter the value of k:\n";
    cin>>k;
    cout<<"Enter the value of m:\n";
    cin>>m;
    cout<<"Enter the value of f:\n";
    cin>>f;

    /* Data Structures Initialization */
    // Creation of message queues
    int ID_MQ1=create_ready_queue_MQ1(),ID_MQ2=create_Scheduler_MMU_MQ2(),ID_MQ3=create_Process_MMU_MQ3();
    // Creaton of shared memories
    int ID_SM1=create_shared_page_table(),ID_SM2=create_free_frame_list();
    int ID_notif=create_notification_Scheduler_Master();
    cout<<"Data Structures Initialized\n";
    cout<<"(Master) ID_SM1: "<<ID_SM1<<" ID_SM2: "<<ID_SM2<<" ID_MQ1: "<<ID_MQ1<<" ID_MQ2: "<<ID_MQ2<<" ID_MQ3: "<<ID_MQ3<<endl;
    int pid_MMU;
    // cout<<"(Master) Testing: "<<to_string(ID_MQ2)<<" in int "<<atoi(to_string(ID_MQ2).c_str())<<endl;
    // Run the MMU process
    if(fork()==0){    
        init_str();
        strcpy(str[0],"g++");
        strcpy(str[1],"--std=c++14");
        strcpy(str[2],"-oMMU");
        strcpy(str[3],"MMU.cpp");
        str[4]=NULL;  
        strcpy(command,"g++");
        execvp(command,str);
        cout<<"Excevp failed to compile MMU!!\n";
        exit(0);
    }
    else{
        wait(NULL);
        if((pid_MMU=fork())==0){
            init_str();
            strcpy(command,"./MMU");
            strcpy(str[0],to_string(ID_MQ1).c_str());
            strcpy(str[1],to_string(ID_MQ3).c_str());
            strcpy(str[2],to_string(ID_SM1).c_str());
            strcpy(str[3],to_string(ID_SM2).c_str());
            str[4]=NULL;
            execvp(command,str);
            cout<<"Excevp failed to execute MMU!!\n";
            exit(0);
        }

    }
    cout<<"MMU running\n";
    // cout<<"(Master) $$$$$$$$$$ ID_MQ1: "<<ID_MQ1<<","<<ID_MQ3<<endl;

    // Create k Processes 
    for(int i=0;i<k;i++){
         int pid_child,pc;
        if((pc=fork())==0){  
            init_str();  
            strcpy(str[0],"g++");
            strcpy(str[1],"--std=c++14");
            strcpy(str[2],("-oProcess"+to_string(i)).c_str());
            strcpy(str[3],"Process.cpp");
            str[4]=NULL;  
            strcpy(command,"g++");
            execvp(command,str);
            cout<<"Excevp failed to compile Process:"<<i<<"\n";
            exit(0);
        }
        // Waiting for compilation to get over
        int status=0;
        waitpid(pc,&status,0);
        sleep(1);
        if((pid_child=fork())==0){
            init_str();
            int mm=rand()%m+1,pp=rand()%(8*m)+2*m;
            strcpy(command,("./Process"+to_string(i)).c_str());
            // Instead of i, sending the pid_child is a good idea
            strcpy(str[0],to_string(i).c_str());
            strcpy(str[1],to_string(ID_MQ1).c_str());
            strcpy(str[2],to_string(ID_MQ3).c_str());
            // cout<<"(Master) ########## "<<ID_MQ1<<","<<ID_MQ3<<endl;
            strcpy(str[3],Generate_Page_Ref_String(mm,pp).c_str());
            // cout<<"(Master) *********** "<<str[0]<<","<<str[1]<<","<<str[2]<<","<<str[3]<<endl;
            str[4]=NULL;
            execvp(command,str);
            printf("Error is %s\n",strerror(errno));
            cout<<"Excevp failed to run Process:"<<i<<"\n";
            exit(0);
        }
       // sleep(0.1);
        // Putting the process to sleep as soon as it is put into ready queue
        // so that for scheduler to schedule it, it can wake it up again
        //kill(pid_child,SIGUSR1);


        // Adding the process ID in the ready in FCFS fashion
        //char process_id[MAX];
        // Sleep for 0.25 then again continue to generate new processes

    }
        // Run the Scheduler
    int pid_scheduler;
    if(fork()==0){    
        init_str();
        strcpy(str[0],"g++");
        strcpy(str[1],"--std=c++14");
        strcpy(str[2],"-oscheduler");
        strcpy(str[3],"Scheduler.cpp");
        str[4]=NULL;  
        strcpy(command,"g++");
        execvp(command,str);
        cout<<"Excevp failed to compile Scheduler!!\n";
        exit(0);
    }
    else{
        wait(NULL);
        if((pid_scheduler=fork())==0){
            init_str();
            strcpy(command,"./scheduler");
            strcpy(str[0],to_string(ID_MQ1).c_str());
            strcpy(str[1],to_string(ID_MQ2).c_str());
            str[2]=NULL;
            execvp(command,str);
            cout<<"Excevp failed to execute Scheduler!!\n";
            exit(0);
        }

    }
    cout<<"Scheduler Running\n";
    pause();


    //while(1);
   // kill(pid_scheduler,SIGINT);
    //kill(pid_MMU,SIGINT);

    return 0;
}