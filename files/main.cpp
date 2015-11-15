#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;


int main(int argc, char* argv[])
{
	printf("<h1>ycqiu</h1>\n");
	for(int i = 0; i < argc; ++i)
	{
	//	cout << argv[i] << endl;
		printf("<h1>%s</h1>\n", argv[i]);
	}

//	cout << getenv("REMOTE_METHOD") << endl;

	return 0;
}

