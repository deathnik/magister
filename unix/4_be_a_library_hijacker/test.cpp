#include "iostream"
#include "string.h"

using namespace std;
int main(){
	char *a = new char[10];
	char *b = new char [10];
	//will be removed by gcc
	memcpy(a,a,10);

	cout <<"delimiter"<<endl;
	memcpy(a,b,10);

	cout << "you should see 1 warning from memcpy before \"delimiter\" msg if gcc wont optiize expression" <<endl;
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    cout << (now->tm_year + 1900) << '-'
         << (now->tm_mon + 1) << '-'
         <<  now->tm_mday
         << endl;

	return 0;
}
