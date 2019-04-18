#include "mrfs.cpp"

int main(int argc, char const *argv[])
{
	my_create(10);
	my_mkdir("mydocs");
	my_mkdir("mycode");
	cout<<"$ In main directory, listing files: \n";
	my_ls();
	my_chdir("mydocs");
	cout<<"\n$ Changed directory to mydocs\n";
	my_mkdir("mytext");
	my_mkdir("mypapers");
	cout<<"$ Listing file in mydocs\n";
	my_ls();
	cout<<"\n$ enter a directory name to remove : ";
	char dir_name[50];
	cin>>dir_name;
	my_rmdir(dir_name);
	cout<<"\n$ Listing file in mydocs post removal\n";
	my_ls();
	return 0;
}