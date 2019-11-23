#include "filesys.h"
#include <stdio.h>

int main()
{

    format();
    mymkdir("/myfirstdir/myseconddir/mythirddir");
    char ** list;
    list = mylistdir("/myfirstdir/myseconddir");
    printf("My list dir: ");
    for(int i = 0; list[i]!=NULL;i++ ) printf(" %s ",list[i]);
    printf("\n");
    MyFILE * testingFile;

    testingFile = myfopen("/myfirstdir/myseconddir/testfile.txt","w");

    myfputc('A',testingFile);


    list = mylistdir("/myfirstdir/myseconddir");
    printf("My list dir: ");

    for(int i = 0; list[i]!=NULL;i++ ) printf(" %s ",list[i]);

    writedisk("virtualdiskA5_A1");

return 0;
}
