#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;


int main(int argc, char* argv[])
{
	printf("<h2>ycqiu</h2>\n");
	for(int i = 0; i < argc; ++i)
	{
		printf("<h2>%s</h2>\n", argv[i]);
	}

	printf("<h2>REMOTE_METHOD = \"%s\"</h2>\n", getenv("REMOTE_METHOD"));
	printf("<h2>QUERY_STRING = \"%s\"</h2>\n", getenv("QUERY_STRING"));

	return 0;
}

