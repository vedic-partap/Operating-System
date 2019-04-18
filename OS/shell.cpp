//@to-do - add stderr
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
int main()
{
    string sentence;
    while (true)
    {
        cout << "[user] $ ";
        getline(cin, sentence); 
        istringstream iss(sentence);
        pid_t child_pid;
        //Break on getting "quit"
        if (sentence == "quit")
            break;
        //tokenise the sentences
        vector<string> tokens;
        copy(istream_iterator<string>(iss),
             istream_iterator<string>(),
             back_inserter(tokens));
        int wait_flag = 1;
     int l =0, r=0, bg=0,end=0;
        string l_file, r_file;

        for(unsigned int i=0;i<tokens.size()-1;i++)
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
        }
        if(tokens[tokens.size()-1]=="&")
            bg = 1;

        const char **argv = new const char *[end+1];
        for (unsigned int i = 0; i < end; i++)
        {
            argv[i] = tokens[i].c_str();
        }
        argv[end] = NULL;
        
        // cout<<bg<<"+\n";
        wait_flag = !bg;
        if ((child_pid = fork()) == 0)
        {
            if(r)
            {
                int fd = open(r_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
                // cout<<"r_file: "<<r_file<<" fd = "<<fd<<endl;
                dup2(fd, 1);
                // close(fd);
            }
            if(l)
            {
                int fd = open(l_file.c_str(), O_RDONLY);
                // cout<<"l_file: "<<l_file<<" fd = "<<fd<<endl;
                dup2(fd, 0);
                // close(fd);
            }
           
            execvp(tokens[0].c_str(), (char **)argv);
            _exit(EXIT_FAILURE);
        }
        
        else
        {
            // wait untill child process is not over
            if(wait_flag)
            {
                wait(NULL);
            }
            else
            {
                cout<< "[ "<<getpid()<<" ] Background\n";
            }
            cout << "\n";
        }
    }
    return 0;
}
