/*
===================
Team : 3 
Vedic Partap 16CS10053
Rahul Kumar 16CS10042
Assignment : 1b 

++++ How to run ++++
$ g++ -Wall Ass1_3_1b.cpp -o 1b
$./1b
Enter the command : /bin/ls -l 
total 128
-rwxrwxr-x 1 vedic vedic 20664 Jan 13 22:29 1a
-rw-rw-r-- 1 vedic vedic  3598 Jan 13 21:55 1a.cpp
-rwxrwxr-x 1 vedic vedic 39904 Jan 13 22:26 1b
-rw-rw-r-- 1 vedic vedic  1338 Jan 13 22:31 1b.cpp
-rwxrwxr-x 1 vedic vedic  9104 Jan 13 22:04 example1
-rw-rw-r-- 1 vedic vedic   245 Jan 13 22:04 example1.cpp
drwxrwxr-x 2 vedic vedic  4096 Jan 13 22:02 tmp

Enter the command : ./example1 vedic loves coding
You have entered 4 arguments:
./example1
vedic
loves
coding

Enter the command : quit
=================== 
*/
#include <iostream>
#include <string>
#include <stdio.h>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>

using namespace std;
int main()
{
    string sentence;
    while(true)
    {
        cout << "Enter the command : ";
        getline(cin, sentence);/*Reading the sentence. getline is used for spaces*/
        istringstream iss(sentence);
        pid_t child_pid;
        //Break on getting "quit"
        if(sentence=="quit") break;
        //tokenise the sentences
        vector<string> tokens;
        copy(istream_iterator<string>(iss),
             istream_iterator<string>(),
             back_inserter(tokens));
        // tokens[0] = "./"+tokens[0];
        //form teh arguments
        const char **argv = new const char *[tokens.size()+1];
        for (unsigned int i = 0; i < tokens.size(); i++)
        {
            argv[i] = tokens[i].c_str();
            // cout<<argv[i]<<"--\n";
        }
        argv[tokens.size()]=NULL;
        // fork the child process
        child_pid = fork();
        if(child_pid==0)
        {
            execv(tokens[0].c_str(),(char**)argv);
            _exit(EXIT_FAILURE);
        }
        
        else
        {
            // wait untill child process is not over
            wait(NULL);
            cout << "\n";
        }
        
    }
    return 0;

}