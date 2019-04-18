#include "myfs.h"

int main(){
	int fs_size = 10000, block_size = 100;

	cout<<"creating myfs \n";
	int x = create_myfs(fs_size, block_size);
	if(x==-1) {
		cout<<" UNABLE TO CREATE MYFS \n";
		return -1;
	}

	x = my_copy("sample_file.txt");

	my_cat("sample_file.txt");
	return 0;
}