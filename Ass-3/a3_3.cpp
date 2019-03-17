#include <bits/stdc++.h>
#include <time.h>

using namespace std;
#define MEAN 0.001
#define DELTA 2

double FCFS(double arrival[], double burst[],int n)
{
    double atn,f = burst[0];
    atn = f;
    for (int i = 1; i < n; i++)
    {
        (arrival[i] > f) ? f = arrival[i] + burst[i] : f += burst[i];            
        atn += f - arrival[i];
    }
    atn = atn / (double)n;
    return atn;
}

double SJF_NP(double arrival[], double burst[], int n)
{
    double atn = 0, f = arrival[0];
    int complete[n] ={},cnt=0;
    while(cnt<n)
    {
        double min_resp = 30;
        int idx=0;
        for(int j=0;j<n&&arrival[j]<=f;j++)
        {
            if(!complete[j]&&burst[j]<=min_resp)
            {
                min_resp = burst[j];
                idx = j;
            }
        }
        if(min_resp==30)
        {
            for(int i=0;i<n;i++)
            {
                if(complete[i])
                idx = i+1;
            }
            f = arrival[idx] + burst[idx];
        }
        else
        {
            f += burst[idx];
        }
        // cout<<idx<<" ";
        complete[idx]=1;
        atn += f-arrival[idx];
        cnt++;
    }
    // cout<<"\n";
    atn = atn / (double)n;
    return atn;
}

double SJF_P(double arrival[], double burst[], int n) 
{
    double atn=0,f=arrival[0];
    int cnt = 0,next_arrival=1;
    priority_queue<pair<double,int> > pq;
    // for(int i=0;i<n;i++)
    // {
    //     if(arrival[i]>0)break;
    //     else pq.push({-arrival[i],i});
    // }
    pq.push({-burst[0], 0}); 
    while (cnt < n)
    {
        if(pq.empty())
        {
            pq.push({-burst[next_arrival], next_arrival});
            f = arrival[next_arrival];
            next_arrival++;
        }
        pair<double,int> temp = pq.top();
        pq.pop();
        if(next_arrival<n&&(f-temp.first)>arrival[next_arrival])
        {
            pq.push({-burst[next_arrival], next_arrival});
            pq.push({-((f - temp.first) - arrival[next_arrival]),temp.second});
            f = arrival[next_arrival];
            next_arrival++;
        }
        else
        {
            cnt++;
            f += -temp.first;
            // cout<<temp.second<<" ";
            atn+=f-arrival[temp.second];
        }
        
    }
    // cout<<"\n";
    atn = atn / (double)n;
    return atn;
}
double RR(double arrival[], double burst[], int n)
{
    double atn=0,f=arrival[0];
    int cnt = 0, next_arrival = 1;
    queue<pair<double,int> > q;
    q.push({burst[0],0});
    while(cnt<n)
    {
        if(q.empty())
        {
            q.push({burst[next_arrival], next_arrival});
            f = arrival[next_arrival];
            next_arrival++;
        }
        pair<double,int> temp = q.front();
        q.pop();
        if(temp.first>DELTA)
        {
            for(;next_arrival<n;next_arrival++)
            {
                if(arrival[next_arrival]<=f+DELTA)
                    q.push({burst[next_arrival],next_arrival});
                else
                    break;
                
            }
            q.push({temp.first-DELTA,temp.second});
            f += DELTA;
            
        }
        else
        {
            f += temp.first;
            for (; next_arrival < n; next_arrival++)
            {
                if (arrival[next_arrival] <= f)
                    q.push({burst[next_arrival], next_arrival});
                else
                    break;
            }
            cnt++;
            // cout << temp.second << " ";
            atn += f - arrival[temp.second];
        }
        
    }
    // cout << "\n";
    atn = atn / (double)n;
    return atn;
}

double HRRN(double arrival[], double burst[], int n)
{
    double atn = 0, f = arrival[0];
    int complete[n] = {}, cnt = 0;
    while (cnt < n)
    {
        double min_response = 0;
        int idx = 0;
        for (int j = 0; j < n && arrival[j] <= f; j++)
        {
            if (!complete[j] && (f-arrival[j]+burst[j])/burst[j] >= min_response)
            {
                min_response = (f-arrival[j]+burst[j])/burst[j];
                idx = j;
            }
        }
        if (min_response == 0)
        {
            for (int i = 0; i < n; i++)
            {
                if (complete[i])
                    idx = i + 1;
            }
            f = arrival[idx] + burst[idx];
        }
        else
        {
            f += burst[idx];
        }
        // cout << idx << " ";
        complete[idx] = 1;
        atn += f - arrival[idx];
        cnt++;
    }
    // cout << "\n";
    atn = atn / (double)n;
    return atn;
}
int main(int argc, char *argv[])
{
    int n=stoi(argv[1]);
    string filename ="data.csv";
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand((time_t)ts.tv_nsec);
    ofstream outfile;
    outfile.open(filename); 
    outfile<<"Id, Arrival Time, Burst Time\n";
    double arrival[n], burst[n], lower_limit = exp(-10.0*MEAN);
    for(int i=0;i<n;i++)
    {
        double r = lower_limit+ ((double)(rand())/RAND_MAX)*(1.0-lower_limit);

        arrival[i] = ((i == 0) ? 0 : -1.0 / MEAN * log(r) + arrival[i - 1]);
        burst[i] = 1+((double)rand()/RAND_MAX)*19.0;
        // cout<<arrival[i]<<"\t"<<burst[i]<<"\t"<<r<<"\n";
        outfile<<i<<", "<<arrival[i]<<", "<<burst[i]<<"\n";
    }
    cout << FCFS(arrival, burst, n) << "\n";
    cout << SJF_NP(arrival, burst, n) << "\n";
    cout << SJF_P(arrival, burst, n) << "\n";
    cout << RR(arrival, burst, n) << "\n";
    cout << HRRN(arrival, burst, n) << "\n";

    return 0;
}