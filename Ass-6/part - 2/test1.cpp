#include "mrfs.cpp"

#include <iostream>
#include <bitset>

using namespace std;

int main(int argc, char const *argv[])
{
	int size;
	cout<<"Enter file system size (in MB): ";
	cin>>size;
	my_create(size);
	for(int i=0;i<8;i++){
		char fname[7] = "rahul1";
		fname[5] += i;
		my_copy("abc.cpp", fname);
	}
	my_ls();
	cout << "Enter file name to delete: ";
	char fname[20] = "rahul3";
	cin >> fname;
	my_rm(fname);
	my_ls();

	cout<<"Enter a file name to read: ";
	cin>>fname;
	// strcpy(fname, "rahul2");
	int fd = my_open(fname, 'r');
	char buff[100];
	my_read(fd, 50, buff);
	my_close(fd);
	cout<<"Reading 50 bytes : \n"<<buff<<endl<<endl;

	cout<<"\nCalling my_cat: \n";
	
	my_cat(fname);
	return 0;
}