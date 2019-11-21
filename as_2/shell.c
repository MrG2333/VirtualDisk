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
    myfputc('V',testingFile);
    myfputc('V',testingFile);
    myfputc('V',testingFile);
    myfputc('V',testingFile);
    list = mylistdir("/myfirstdir/myseconddir");
    printf("My list dir: ");
    for(int i = 0; list[i]!=NULL;i++ ) printf(" %s ",list[i]);






    writedisk("virtualdiskB3_B1");

return 0;
}
