#include "filesys.h"
#include <stdio.h>

int main()
{
    MyFILE * file1;
    MyFILE * file2;
    MyFILE * file3;

    char ** list;
    int  test_char;

    printf("=====================================================\n\n");
    printf(" The following part demonstrates that the functions \n listed below work as expected\n\n");
    printf(" -format()\n -myfopen\n- myfputc()\n- myfgetc()\n- mymkdir()\n- mychdir()\n -mylistdir()\n\n");
    printf("=====================================================\n\n");


    format();
    mymkdir("/firstdir/seconddir");


    file1 = myfopen("/firstdir/seconddir/testfile1.txt","w");
    printf("\nPlace CS3026 one character at a time in file1\n\n");
    myfputc('C',file1);
    myfputc('S',file1);
    myfputc('3',file1);
    myfputc('0',file1);
    myfputc('2',file1);
    myfputc('6',file1);

    test_char = myfgetc(file1);

    printf("\n\nLast character placed in file1 :%c\n\n", (char) test_char );
    myfclose(file1);

    list = mylistdir(".");
    printf("\n\n My listdir list: ");
    for(int i=0;list[i]!=NULL;i++) printf(" %s ",list[i]);
    printf("\n\n");

    mychdir("/firstdir/seconddir");

    list = mylistdir("/firstdir/seconddir");
    printf("\n\n My listdir list: ");
    for(int i=0;list[i]!=NULL;i++) printf(" %s ",list[i]);
    printf("\n\n");



    printf("\nPlace WINTER one character at a time in file2\n\n");
    file2 = myfopen("testfile2.txt","w");
    myfputc('W',file2);
    myfputc('I',file2);
    myfputc('N',file2);
    myfputc('T',file2);
    myfputc('E',file2);
    myfputc('R',file2);
    myfclose(file2);

    printf("\nCreate directory third\n\n");
    mymkdir("third");

    printf("\nPlace AUCS one character at a time in file3 located in directory\n'third' with the relative path third/testfile3.txt\n\n");
    file3 = myfopen("third/testfile3.txt","w");
    myfputc('A',file3);
    myfputc('U',file3);
    myfputc('C',file3);
    myfputc('S',file3);
    myfclose(file3);
    printf("\n\n Write part 'a' to the real disk\n\n");
    writedisk("virtualdisk_a");


    printf("=====================================================\n\n");
    printf(" The following part demonstrates that the functions \n listed below work as expected\n\n");
    printf(" -myremove()\n\n");
    printf("=====================================================\n\n");


    printf("\n\n remove testfile1.txt and testfile2.txt\n\n");
    myremove("testfile1.txt");
    myremove("testfile2.txt");

    printf("\n\n Write part 'b' to the real disk\n\n");
    writedisk("virtualdisk_b");
    printf("=====================================================\n\n");
    printf(" The following part demonstrates that after removing files the directory\n structure can still be navigated and files can still be accesed\n\n");
    printf("=====================================================\n\n");


    printf("\n\n cd into third directory and remove testfile.txt");
    mychdir("third");
    myremove("testfile3.txt");

    printf("\n\n Write part 'c' to the real disk\n\n");
    writedisk("virtualdisk_c");
    printf("=====================================================\n\n");
    printf(" The following part demonstrates that after removing all the directories\n and all the files remaining the memory returns to the inital state of just\n one root directory with no clutter remaining\n\n");
    printf("=====================================================\n\n");


    mychdir("/firstdir/seconddir");
    myremdir("third");
    mychdir("/firstdir");
    myremdir("seconddir");
    mychdir("/");
    myremdir("firstdir");
    printf("\n\n Write part 'd' to the real disk\n\n");

    writedisk("virtualdisk_d");
return 0;
}
