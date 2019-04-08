#include "myfs.h"

int main(){
	int fs_size = 10000, block_size = 1000;

	cout<<"creating myfs \n";
	int x = create_myfs(fs_size, block_size);
	if(x==-1) {
		cout<<" UNABLE TO CREATE MYFS \n";
		return -1;
	}

	x = my_copy("sample_file.txt");

	cout<<"opening file \n";
	int fd = my_open("sample_file.txt");
	cout<<"opened file, fd = "<<fd<<endl;

	char buff1[605];
	x = my_read(fd, 605, buff1);
	buff1[x] = '\0';
	cout<<"Read: "<<buff1<<endl;
	return 0;
}