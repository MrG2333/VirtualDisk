#include "filesys.h"
#include <stdio.h>

int main()
{
    MyFILE * file1;
    MyFILE * file2;
    MyFILE * file3;

    char ** list;

    format();
    mymkdir("/firstdir/seconddir");

    file1 = myfopen("/firstdir/seconddir/testfile1.txt","w");
    myfputc('C',file1);
    myfputc('S',file1);
    myfputc('3',file1);
    myfputc('0',file1);
    myfputc('2',file1);
    myfputc('6',file1);
    myfclose(file1);

    //list = mylistdir("/firstdir/seconddir");
    //printf("\n\n My listdir list: ");
    //for(int i=0;list[i]!=NULL;i++) printf("%s ",list[i]);
    //printf("\n\n");

    mychdir("/firstdir/seconddir");


    file2 = myfopen("testfile2.txt","w");
    myfputc('W',file2);
    myfputc('I',file2);
    myfputc('N',file2);
    myfputc('T',file2);
    myfputc('E',file2);
    myfputc('R',file2);
    myfclose(file2);


    list = mylistdir(".");
    printf("\n\n My listdir list: ");
    for(int i=0;list[i]!=NULL;i++) printf(" %s ",list[i]);
    printf("\n\n");

    mymkdir("third");


    list = mylistdir("/firstdir/seconddir");
    printf("\n\n My listdir list: ");
    for(int i=0;list[i]!=NULL;i++) printf(" %s ",list[i]);
    printf("\n\n");

    file3 = myfopen("third/testfile3.txt","w");
    myfputc('A',file3);
    myfputc('U',file3);
    myfputc('C',file3);
    myfputc('S',file3);
    myfclose(file3);

    writedisk("virtualdiskA5_A1_a");

    myremove("testfile1.txt");
    myremove("testfile2.txt");

    writedisk("virtualdiskA5_A1_b");

    mychdir("third");
    myremove("testfile3.txt");

    writedisk("virtualdiskA5_A1_c");

    mychdir("/firstdir/seconddir");
    myremdir("third");
    mychdir("/firstdir");
    myremdir("seconddir");
    mychdir("/");
    myremdir("firstdir");
    writedisk("virtualdiskA5_A1_d");
return 0;
}
