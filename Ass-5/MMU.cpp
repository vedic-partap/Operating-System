#include "Library.h"
using namespace std;
MESSAGE_BUFFER message;
int process_id;
TLB tlb[1];
int tlb_size = 1;
PAGE_TABLE *page_table;
FREE_FRAME *free_frame_list;
int timestamp_id,tmp;
// tlb [i] is the tlb for each process i
void init(){
    for(int i=0;i<tlb_size;i++){
        tlb[i].page_no=new int[MAX];
        tlb[i].frame_no=new int[MAX];
        for(int j=0;j<MAX;j++){
            tlb[i].frame_no[j]=tlb[i].page_no[j]=-1;
        }
        //memset(tlb[i].page_no,-1,sizeof tlb[i].page_no);
        //memset(tlb[i].frame_no,-1,sizeof tlb[i].frame_no);
    }
}
void update_tlb(int page_no,int frame_no){
    for(int i=0;i<tlb_size;i++){
        if(tlb[process_id].page_no[i]==page_no){
            tlb[process_id].page_no[i]=page_no;
            tlb[process_id].frame_no[i]=frame_no;
            return;
        }
    }
    tlb[process_id].page_no[0]=page_no;
    tlb[process_id].frame_no[0]=frame_no;
}
int search_TLB(int page_no){
    for(int i=0;i<tlb_size;i++){
        if(tlb[process_id].page_no[i]==page_no && tlb[process_id].frame_no[i]!=-1){
            return tlb[process_id].frame_no[i];
        }
    }
    
    return -1;
}
int create_shared_page_table(){
    key_t  key=ftok("SM1",65); 
    int shmid=shmget(key,sizeof(PAGE_TABLE[MAX]),0666|IPC_CREAT);
    
    if(shmid<0){
        printf("Creation of shared page table file error!!\n");
        return -1;
    } 
    page_table=(PAGE_TABLE*)shmat(shmid,NULL,0);
    for(int i=0;i<MAX;i++){
        page_table[i].valid=new int[MAX];
        page_table[i].frame_no=new int[MAX];
        page_table[i].timestamp=new int[MAX];
        page_table[i].page_no=new int[MAX];
        for(int j=0;j<MAX;j++){
            page_table[i].valid[j]=-1;
            page_table[i].frame_no[j]=-1;
            page_table[i].timestamp[j]=-1;
            page_table[i].page_no[j]=-1;
        }
    }

    
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
        free_frame_list[i].isfree=1;
    }
    return shmid; 
}
int attach_process_mmu_msg_queue(char *MQ3){
    key_t  key=ftok("MQ3",65); int msgid;
    msgid = msgget(key,IPC_CREAT|0666); 
    return msgid;
}
int attach_mmu_scheduler_msg_queue(char *MQ2){
    key_t  key=ftok("MQ2",65); int msgid;
    msgid = msgget(key,IPC_CREAT|0666); 
    return msgid;
}
int main(int argc,char **argv){

    cout<<"in MMU\n";
    char *MQ2=argv[0];
    char *MQ3=argv[1];
    char *SM1=argv[2];
    char *SM2=argv[3];
    init();
    // int ID_MQ3=attach_process_mmu_msg_queue(MQ3),ID_MQ2=attach_mmu_scheduler_msg_queue(MQ2);
    int ID_MQ3=atoi(MQ3),ID_MQ2=atoi(MQ2);
    cout<<"(MMU) ID_MQ2: "<<ID_MQ2<<", ID_MQ3: "<<ID_MQ3<<endl;

    int page_id=create_shared_page_table();
    int frame_id=create_free_frame_list();

    
    while(1){

        msgrcv(ID_MQ3,&message,sizeof(message),6,0);
        cout<<"(MMU) received msg from process : "<<message.text<<", type = "<<message.message_type<<endl;
        try{
            process_id=stoi((string(message.text)),NULL,10);
        }
        catch(int x){
            process_id=0;
            continue;
        }
        msgrcv(ID_MQ3,&message,sizeof(message),1,0);
        cout<<"(MMU) received msg from process : "<<message.text<<", type = "<<message.message_type<<endl;
        if(strcmp(message.text,"-9")==0){

            strcpy(message.text,"TERMINATED");
            message.message_type=10;
            msgsnd(ID_MQ2,&message,sizeof(message),0);   

            continue;
        }

        int page_no=0;
        try{
            page_no=stoi((string(message.text)),NULL,10);
        }
        catch(int x){
            page_no=0;
            continue;
        }
        cout<<"(MMU) searching TLB\n";
        int frame_no=search_TLB(page_no);
        cout<<"(MMU) page_no: "<<page_no<<", frame_no: "<<frame_no<<endl;
        if(frame_no!=-1){

            strcpy(message.text,to_string(frame_no).c_str());
            message.message_type=2;
            msgsnd(ID_MQ3,&message,sizeof(message),0);
            cout<<"Process_ID:"<<process_id<<" Page_No:"<<page_no<<" Frame_No:"<<frame_no<<" Timestamp:"<<(++tmp)<<endl;
        }
        else{

            int flag=0;
            for(int i=0;i<MAX;i++){

                if(page_table[process_id].page_no[i]==page_no && page_table[process_id].frame_no[i]!=-1){
                    frame_no=page_table[process_id].frame_no[i];
                    strcpy(message.text,to_string(frame_no).c_str());
                    message.message_type=2;
                    msgsnd(ID_MQ3,&message,sizeof(message),0);
                    flag=1;
                    update_tlb(page_no,frame_no);
                    break;
                }
            }
            if(!flag){

                strcpy(message.text,"-1");
                message.message_type=2;
                msgsnd(ID_MQ3,&message,sizeof(message),0);

                int free_frame=-1;
                for(int i=0;i<MAX;i++){
                    if(free_frame_list[i].isfree!=-1){
                        free_frame=i;
                        free_frame_list[i].isfree=-1;
                        break;
                    }
                }

                if(free_frame!=-1){
                    int flag3=0,flag4=0;
                    for(int i=0;i<MAX;i++){
                        if(tlb[process_id].frame_no[i]==-1 && tlb[process_id].page_no[i]==-1){
                            tlb[process_id].frame_no[i]=free_frame;
                            tlb[process_id].page_no[i]=page_no;
                            flag3=1;
                            break;
                        }
                    }
                    if(!flag3){
                        tlb[process_id].frame_no[0]=free_frame;
                        tlb[process_id].page_no[0]=page_no;
                    }

                    for(int i=0;i<MAX;i++){
                        if(page_table[process_id].page_no[i]==-1 && page_table[process_id].frame_no[i]==-1){
                            page_table[process_id].frame_no[i]=free_frame;
                            page_table[process_id].page_no[i]=page_no;
                            page_table[process_id].timestamp[i]=(++timestamp_id);
                            flag4=1;
                            break;
                        }
                    }
                    if(!flag4){
                        page_table[process_id].frame_no[0]=free_frame;
                        page_table[process_id].page_no[0]=page_no;
                        page_table[process_id].timestamp[0]=(++timestamp_id);
                    }

                }
                else{
                    int LRU_idx=0,timestamp=1e9+7,replaced_frame=-2;
                    for(int i=0;i<MAX;i++){
                        if(page_table[process_id].timestamp[i]<=timestamp){
                            LRU_idx=i;
                            timestamp=page_table[process_id].timestamp[i];
                            replaced_frame=page_table[process_id].frame_no[i];
                        }
                    }
                    page_table[process_id].page_no[LRU_idx]=page_no;
                    page_table[process_id].frame_no[LRU_idx]=replaced_frame;
                    
                    for(int i=0;i<MAX;i++){
                        if(tlb[process_id].page_no[i]==page_no){
                            tlb[process_id].frame_no[i]=replaced_frame;
                            break;
                        }
                    }

                }
                strcpy(message.text,"PAGE_FAULT_HANDLED");
                message.message_type=10;
                msgsnd(ID_MQ2,&message,sizeof(message),0);

            }

        }
   }

    return 0;
}