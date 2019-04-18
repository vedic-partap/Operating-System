#include <bits/stdc++.h>
using namespace std;

int main(){
	vector<int> A(5);
	A[0] = 1;
	vector<int> *temp;
	temp = &A;
	temp->at(0) = 2;
	cout<<A[0]<<","<<temp->at(0)<<endl;
	return 0;
}