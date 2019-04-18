#include "myfs.h"

int main(){
	int fs_size = 10000, block_size = 1000;

	cout<<"creating myfs \n";
	int x = create_myfs(fs_size, block_size);
	cout<<"created myfs, x = "<<x<<" \n";

	cout<<"opening file \n";
	int fd = my_open("rahul.txt");
	cout<<"opened file, fd = "<<fd<<endl;

	char buff[100] = "Rahul Vernwal";

	cout<<"Writing buff = "<<buff<<endl;
	x = my_write(fd, strlen(buff), buff);

	char buff1[100];
	x = my_read(fd, 10, buff1);
	buff1[x] = '\0';
	cout<<"Read: "<<buff1<<endl;
	return 0;
}