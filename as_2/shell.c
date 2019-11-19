#include "filesys.h"
#include <stdio.h>

int main()
{
 FILE *fp;

    fp = fopen("testfileC3_C1_copy.txt", "w+");

    char  data_filler[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZZ";
    char big_array [4*BLOCKSIZE];

    int size_fille = strlen(data_filler) - 1;

    format();

    MyFILE * testingFile;

    testingFile = myfopen("file_tet.txt","w");
    for (int i = 0; i < (4 * BLOCKSIZE); i++){
        big_array[i] = data_filler[i%size_fille];
    }


    for(int i = 0; i < (4*BLOCKSIZE);i++){
        myfputc(big_array[i], testingFile);

    }
    writedisk("virtualdiskC3_C1");


    myfclose(testingFile);



    testingFile = myfopen("file_tet.txt","r");

    int return_c;

    return_c = myfgetc(testingFile);
    //printf("returned character : %c",return_c);
    //return_c = myfgetc(testingFile);
    //printf("returned character : %c",return_c);

    writedisk("virtualdiskC3_C1");

    myfclose(testingFile);
    fclose(fp);

return 0;
}
