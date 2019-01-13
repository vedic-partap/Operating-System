/*
===================
Team : 3 
Vedic Partap 16CS10053
Rahul Kumar 16CS10042
Assignment : 1a

++++ Important Points ++++
1. The main process is A
2. B, C, D, E are the child process of A

++++ How to run ++++
$ g++ -Wall Ass1_3_1a.cpp -o 1a
$./1a
=================== 
*/
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <bits/stdc++.h>
#include <sys/wait.h>
#define SIZE 50
using namespace std;
/*
Function to merge the two sorted array
===================
Input:
arr[] - the array to be merged
n - total size fo array
m - the partiton 
===================
*/
void merge(int arr[], int temp[], int n, int m)
{
    int i=0,j=m;
    for(int k=0;k<n;k++)
    {
        if(i>=m||(i<m&&j<n&&arr[i]>arr[j]))
            temp[k]=arr[j++];
        else
            temp[k]=arr[i++];
    }
}
int main()
{
    pid_t B, C, D, E;
    int pipAD[2], pipBD[2], pipCE[2], pipDE[2];
    // forming the pipes
    int pipedAD = pipe(pipAD), pipedBD = pipe(pipBD), pipedDE = pipe(pipDE), pipedCE = pipe(pipCE);
    B = fork();
    if (B == 0)
    {
        int numberB[SIZE];
        // ramdom number generation
        srand(100);
        for (int i = 0; i < SIZE; i++)
        {
            numberB[i] = rand() % 100+1;
        }
        // sort the array
        sort(numberB, numberB + SIZE);
        if (pipedBD!=-1)
        {
            close(pipBD[0]);
            // write to the pipe
            write(pipBD[1], numberB, SIZE * sizeof(int));
        }
        // exit the process
        exit(0);   
    }
    
    else
    {
        int numberA[SIZE];
        srand(200);
        // random number generation
        for (int i = 0; i < SIZE; i++)
        {
            
            numberA[i] = rand() % 100 +1;
        }
        sort(numberA,numberA+SIZE);
        if (pipedAD != -1)
        {
            // close(pipAD[0]);
            write(pipAD[1], numberA, SIZE * sizeof(int));
        }
        // process D is formed
        int status;
        waitpid(B,&status,0);
        D = fork();
        if(D==0)
        {
            int temp[2*SIZE], temp2[2*SIZE];
            // get data from pipe connected to A and B process;
            if (pipedBD != -1)
            {
                close(pipBD[1]);
                read(pipBD[0], temp, SIZE * sizeof(int));
                close(pipBD[0]);
            }
            if(pipedAD!=-1)
            {
                close(pipAD[1]);
                read(pipAD[0], temp + SIZE, SIZE * sizeof(int));
                close(pipAD[0]);
            }  
            // merge the sorted arrays
            merge(temp,temp2,2*SIZE,SIZE);
            // for (int i = 0; i < 2 * SIZE; i++)
            // {
            //     cout << temp2[i] << "++\n";
            // }
            if(pipedDE!=-1)
            {
                close(pipDE[0]);
                write(pipDE[1], temp2, 2 * SIZE * sizeof(int));
            }
            //exit to process D
            exit(0);
            
        }
        
        else
        {
            // process C is formed in parallel
            C = fork();
            if(C==0)
            {
                int numberC[SIZE];
                srand(235);
                // random number gen.
                for (int i = 0; i < SIZE; i++)
                {

                    numberC[i] = rand() % 100 +1;
                }
                // sort
                sort(numberC, numberC + SIZE);
                if(pipedCE!=-1)
                {
                    // write to pipe
                    close(pipCE[0]);
                    write(pipCE[1], numberC, SIZE * sizeof(int));
                }
                exit(0);
            }
            
            else
            {
                // process E is formed
                int statusC,statusD;
                waitpid(C, &statusC, 0);
                waitpid(D, &statusD, 0);
                E = fork();
                if(E==0)
                {
                    int temp[3 * SIZE], temp2[3*SIZE];
                    // get data from pipes conbnected to D and C
                    if(pipedCE!=-1)
                    {
                        close(pipCE[1]);
                        read(pipCE[0], temp, SIZE * sizeof(int));
                        close(pipCE[0]);
                    }
                    if(pipedDE!=-1)
                    {
                        close(pipDE[1]);
                        read(pipDE[0], temp + SIZE, 2 * SIZE * sizeof(int));
                        close(pipDE[0]);
                    }
                    merge(temp,temp2, 3*SIZE, SIZE);
                    cout<<"The final Sorted Array is :\n";
                    for (int i = 0; i < 3*SIZE; i++)
                    {
                        cout << temp2[i] << "\n";
                    }
                    exit(0);
                    
                }
                    
            }
            
        }
        

    }
    return 0;

}