#include <iostream>
#include <string>
#include <stdio.h>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#define MAX_CHAR 100
using namespace std;

void execute(string sentence, int in_fd, int out_fd, int pre_out, int next_in) {
    istringstream iss(sentence);

    pid_t child_pid;
    int wait_flag = 1;
    int l =0, r=0, bg=0,end=0;
    string l_file, r_file;
    vector<string> tokens;
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter(tokens));
    for(unsigned int i=0;i<tokens.size();i++)
        {
            if(tokens[i]=="<")
            {
                l =1;
                l_file = tokens[i+1];
            }
            if(tokens[i]==">")
            {
                r=1;
                r_file = tokens[i+1];
            }
            if(tokens[i]=="&")
            {
                bg=1;
            }
            if(!l&&!r&&!bg)
                end++;
        }
        const char **argv = new const char *[end+1];
        for (int i = 0; i < end; i++)
        {
            argv[i] = tokens[i].c_str();
        }
        argv[end] = NULL;
        wait_flag = !bg;
        if ((child_pid = fork()) == 0)
        {

            if(r)
                out_fd = open(r_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
           
            if(out_fd!=1) {
                dup2(out_fd, 1);
            }
            if(l) 
                in_fd = open(l_file.c_str(), O_RDONLY);
            
            if(in_fd!=0) {
                dup2(in_fd, 0);
            }
            

            execvp(tokens[0].c_str(), (char **)argv);
            exit(0);
        }
        
        else
        {
            // wait untill child process is not over
            if(wait_flag)
            {
                wait(NULL);
                if(in_fd!=0)
                    close(in_fd);
                if(out_fd!=1)
                    close(out_fd);

            }
            else
            {
                cout<< "[ Process ] Background\n";
            }
        }
    return;
}
int main()
{
    string sentence;
    cout << "\n-----------------------------------------------\n-----------------------------------------------\n\n";
    cout << "----------  Welcome to Group 3 Shell ----------\n\n";
    cout << "-----------------------------------------------\n-----------------------------------------------\n\n";
    while (true)
    {
        cout << "[user] >>> ";
        getline(cin, sentence); 
        
        
        //Break on getting "quit"
        if (sentence == "quit")
            break;
        
        vector<string> cmds;
        int b_cmd = 0, e_cmd = 0;
        for(int i=0;sentence[i]!='\0';i++){
            if(sentence[i]=='|') {
                cmds.push_back(sentence.substr(b_cmd, e_cmd-b_cmd));
                b_cmd = e_cmd = i+1;
            }
            else
                e_cmd++;
        }
        cmds.push_back(sentence.substr(b_cmd, e_cmd-b_cmd));

        int no_pipes = cmds.size()-1;
        int **pipes = new int *[no_pipes];
        for(int j=0;j<no_pipes;j++){
                pipes[j]=new int[2];
                pipe(pipes[j]);
        }

        for(unsigned int i=0;i<cmds.size();i++) {
            int in_fd = 0, out_fd = 1;
            int pre_out = -1, next_in = -1;
            if(i>0) {
                in_fd = pipes[i-1][0];
                pre_out = pipes[i-1][1];
            }
            if(i<cmds.size()-1) {
                out_fd = pipes[i][1];
                next_in = pipes[i][0];
            }


            execute(cmds[i], in_fd, out_fd, pre_out, next_in);

        }
       fflush(stdin);
       fflush(stdout);
    }
    return 0;
}
