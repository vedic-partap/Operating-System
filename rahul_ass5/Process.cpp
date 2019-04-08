#include "Library.h"
#include<errno.h>
using namespace std;
char *MQ1,*MQ3,*SM1,*SM2;
MESSAGE_BUFFER message;
void run(int id){
    ;
    //cout<<"In run signal handler\n";
    // signal(SIGUSR1,run);
    // return;
}
void pause(int id){
	pause();
}

int page_ref_no[MAX];
int acces_no,total_no;
process_mem_access* Process_mem_access;


int attach_process_mmu_shared_mem(char* MQ3){
    key_t  key=ftok("MQ3",65);int msgid; 
    msgid = msgget(key, IPC_CREAT|0666); 
    return msgid;
}
int attach_MQ1(){
    key_t  key=ftok("MQ1",65);int msgid; 
    msgid = msgget(key, IPC_CREAT|0666); 
    return msgid;
}
int main(int argv,char **argc){
    // cout<<"(Process) *********** "<<argc[0]<<","<<argc[1]<<","<<argc[2]<<","<<argc[3]<<endl;

    cout<<"in Process "<<getpid()<<"\n";
    signal(SIGUSR1,run);
    signal(SIGUSR2,pause);

    char *MQ1=argc[1];
    char *MQ3=argc[2];
    int ID_MQ1=atoi(MQ1);
    int ID_MQ3=atoi(MQ3);
    cout<<"(Process) ID_MQ1: "<<ID_MQ1<<", ID_MQ3: "<<ID_MQ3<<endl;
    MESSAGE_BUFFER MQ;
    strcpy(MQ.text,to_string(getpid()).c_str());
    MQ.message_type=4;
    cout<<"(Process) process_id: "<<getpid()<<" sending ID_MQ1 type 4, text "<<MQ.text<<endl;
    msgsnd(ID_MQ1,&MQ,sizeof(MQ),0);
    perror("(Process) after msgsend");
    // cout<<"Process Id: "<<MQ.text<<", send in ID_MQ1 "<<ID_MQ1<<", going to sleep\n";
    pause();
    cout<<"(Process) process_id: "<<getpid()<<" woke up\n";
    int process_id=0;
    try{
        process_id=stoi(string(argc[0]),NULL,10);
    }
    catch(int x){
        process_id=0;
    }
    string page_ref_str(argc[3]);


 
    // cout<<"going to parse page_ref_str\n";
    // cout<<" received page_ref_str = "<<page_ref_str<<endl;
    for(int i=0;i<page_ref_str.length();){
        int j=i;
        while(j<page_ref_str.length() && page_ref_str[j]!=',')j++;
        int page_val;
        try{
            page_val=stoi(page_ref_str.substr(i,j-i),NULL,10);
        }
        catch(int x){
            page_val=0;
            continue;
        }
        // cout<<" got page_val \n";
        page_ref_no[i]=page_val;
        cout<<"Requested:"<<page_val<<endl;
        message.message_type=6;
        strcpy(message.text,to_string(process_id).c_str());
        msgsnd(ID_MQ3,&message,sizeof(message),0);
        

        message.message_type=1;
        strcpy(message.text,page_ref_str.substr(i,j-i).c_str());
        msgsnd(ID_MQ3,&message,sizeof(message),0);
    
        MESSAGE_BUFFER receive;
        msgrcv(ID_MQ3,&receive,sizeof(receive),2,0);
        int frame_no;
        try{
            frame_no=stoi((string(receive.text)),NULL,10);
        }
        catch(int x){
            frame_no=0;
            continue;
        }
        if(frame_no==-1){
            cout<<"Page Fault occured retrying again\n";
            pause();
            continue;
        }
        else if(frame_no<=-2){
            message.message_type=1;
            strcpy(message.text,"-9");
            int r=msgsnd(ID_MQ3,&message,sizeof(message),0);
            
            return 0;
        }
        cout<<"Received frame_no:"<<frame_no<<" for page_no:"<<page_val<<endl;
        i=j+1;
    }   

    message.message_type=6;
    strcpy(message.text,to_string(process_id).c_str());
    int r=msgsnd(ID_MQ3,&message,sizeof(message),0);
    
    message.message_type=1;
    strcpy(message.text,"-9");
    r=msgsnd(ID_MQ3,&message,sizeof(message),0);
    
    return 0;
}