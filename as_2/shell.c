#include "filesys.h"

int main()
{
    format();
    MyFILE * ptr_main;
    ptr_main = myfopen("file_ter","w");
    printf("%d",ptr_main->pos);

    writedisk("virtualdiskC3_C1");

return 0;
}
