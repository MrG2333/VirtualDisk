/* filesys.c
 *
 * provides interface to virtual disk
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "filesys.h"


diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
direntry_t * currentDir              = NULL ;
fatentry_t   currentDirIndex         = 0 ;
fatentry_t   realDirblockno          = 0 ;
 /*
  *
  * File management functions
  *
  */


 /*
  * Write the file descriptor and return a pointer to where in memory it is.
  */

int retUnusedSector()
{
    int i=0;
    while(FAT[i]!=UNUSED) i++;
    return i;
}

void myfputc(int b, MyFILE * stream)
{
    printf("> myfputc start\n");

    short int unused_sector;

    int f_loc = file_in_directory(virtualDisk[stream->dir_start].dir,stream->name);


    virtualDisk[stream->dir_start].dir.entrylist[f_loc].filelength++;
    virtualDisk[stream->dir_start].dir.entrylist[f_loc].modtime = time(0);


    if( strcmp(stream->mode,"w") == 0)
    {
    unused_sector = retUnusedSector();

    if(stream->pos == BLOCKSIZE)    ///leave space for EOL and a EOF
    {

        FAT[stream->blockno] = unused_sector;
        FAT[unused_sector] = ENDOFCHAIN;

        stream->blockno = unused_sector;
        stream->pos = 0;
        copyFAT();
        for(int i=0;i<BLOCKSIZE;i++) stream->buffer.data[i]='\0';

    }


    stream->buffer.data[stream->pos] = b;
    stream->pos++;

    }

    writeblock(&stream->buffer, stream->blockno);
    printf("> myfputc stop\n");

}


int myfgetc(MyFILE * stream)
{
    int f_loc = file_in_directory(virtualDisk[stream->dir_start].dir,stream->name);

    int f_length = virtualDisk[stream->dir_start].dir.entrylist[f_loc].filelength;


    fatentry_t bln= f_length/BLOCKSIZE;                                             ///number of blocks

    int c_to_return;

    if(stream->pos == BLOCKSIZE)
    {
        stream->blockno = FAT[stream->blockno];
        stream->pos = 0;
    }

    if(stream->pos + (bln*BLOCKSIZE) <= f_length)
        {
            c_to_return =virtualDisk[stream->blockno].data[stream->pos-1];

            stream->pos++;
            if(c_to_return == 255)
                return -1;
            return c_to_return;
        }
}

void myfclose(MyFILE * stream)
{
    ///if there is no space for EOF go to the next block and place EOF
    printf("> myfclose start\n");
    short int unused_sector;
    if(stream->pos == BLOCKSIZE)
    {
        unused_sector = retUnusedSector();
        FAT[stream->blockno] = unused_sector;
        FAT[unused_sector] = ENDOFCHAIN;

        stream->blockno = unused_sector;
        stream->pos = 0;
        copyFAT();
        for(int i=0;i<BLOCKSIZE;i++) stream->buffer.data[i]='\0';
    }

    for(int j = BLOCKSIZE; j>=0;j--)
        if(stream->buffer.data[j] == 255)
            break;
        else
        {
            if(stream->buffer.data[j] != 0)
            {
                stream->buffer.data[j+1] = -1;
                break;
            }
            if(j == 0)
            {
                stream->buffer.data[j] = -1;
            }
        }

    writeblock(&stream->buffer,stream->blockno); ///ASSUME ENOUGH SPACE EXISTS
    free(stream);
    printf("> myfclose stop\n");
}

int file_in_directory( dirblock_t local_dir ,const char * filename )
{
for(int i =0; i < DIRENTRYCOUNT;i++)
   {
    if( strcmp(local_dir.entrylist[i].name, filename) ==0)
        {
        return i;
        }
   }
return -1;

}



dirblock_t file_location(const char * path)
{

    diskblock_t search_block;
    int found = 0;

    if(path[0] == '/')
        currentDirIndex = rootDirIndex;
    else
        currentDirIndex = realDirblockno;

    search_block.dir = virtualDisk[currentDirIndex].dir;

    char path_tokenize[strlen(path)];
    strcpy(path_tokenize,path);


    char* path_dir;
    char* rest = path_tokenize;

    while ((path_dir = strtok_r(rest, "/", &rest))){

        for(int i=0; i < DIRENTRYCOUNT;i++){
                if(strcmp(search_block.dir.entrylist[i].name,path_dir) == 0 && search_block.dir.entrylist[i].isdir == 1){
                    found = 1;
                    currentDirIndex = search_block.dir.entrylist[i].firstblock;
                    search_block.dir = virtualDisk[currentDirIndex].dir;
                }
            }
    }


    return search_block.dir;



}

void myremdir(char * dirname)
{
    diskblock_t navigating_directory;

    ///this has to go to the parrent assume user does not try to delete root
    if(dirname[0]=='/')
        {
            navigating_directory = virtualDisk[rootDirIndex];
        }
    else
        navigating_directory = virtualDisk[realDirblockno ] ;    ///go to parent so that we get the entry from the previous directory

    int pos_last_slash=0;
    for(int i = 0;i<strlen(dirname); i++){
        if(dirname[i] == '/')  pos_last_slash = i+1;
    }


    char path_tokenize[strlen(dirname)];
    strcpy(path_tokenize,dirname);

    char* path_dir;
    char* rest = path_tokenize;

     while ((path_dir = strtok_r(rest, "/", &rest))){

        for(int i = 0 ;i <=DIRENTRYCOUNT;i++)
        {
            if(strcmp(path_dir, navigating_directory.dir.entrylist[i].name)==0) break;

            if(strcmp(navigating_directory.dir.entrylist[i].name, path_dir )==0 && navigating_directory.dir.entrylist[i].isdir == 1 )         ///it should stop before returning the file
            {

                navigating_directory.dir = virtualDisk[navigating_directory.dir.entrylist[i].firstblock].dir;
                break;
            }

        }

    }

    for(int i=0;i<DIRENTRYCOUNT;i++)
    {   //printf("I am inside");
        if(strcmp(dirname+pos_last_slash, navigating_directory.dir.entrylist[i].name)==0)
        {
            FAT[navigating_directory.dir.entrylist[i].firstblock] = UNUSED;
            copyFAT();
            for(int j = 0;j<BLOCKSIZE;j++) virtualDisk[navigating_directory.dir.entrylist[i].firstblock].data[j]='\0';

            for(int j=i;j < navigating_directory.dir.nextEntry;j++) navigating_directory.dir.entrylist[j] =navigating_directory.dir.entrylist[j+1];
            navigating_directory.dir.nextEntry--;

            writeblock(&navigating_directory.data, navigating_directory.dir.start);

        }
    }
}

void myremove(char * pathfile)
{
    printf("> myremove start\n");
    diskblock_t navigating_dir;
    int pos_last_slash=0;
    int fat_chain_to_unused[30];
    int fat_chain_counter = 0;
    int fat_checker;
    ///get current directory
    if(pathfile[0]== '/')
        navigating_dir = virtualDisk[rootDirIndex];
    else
        navigating_dir = virtualDisk[realDirblockno];

    /// look for the entry of the file you want to delete
    ///we know that the file will not be

    char path_tokenize[strlen(pathfile)];
    strcpy(path_tokenize,pathfile);

    char* path_dir;
    char* rest = path_tokenize;

     while ((path_dir = strtok_r(rest, "/", &rest))){
        for(int i = 0 ;i <=DIRENTRYCOUNT;i++)
        {
            if(strcmp(navigating_dir.dir.entrylist[i].name, path_dir )==0 && navigating_dir.dir.entrylist[i].isdir == 1 )         ///it should stop before returning the file
            {
                navigating_dir.dir = virtualDisk[navigating_dir.dir.entrylist[i].firstblock].dir;

                break;
            }
        }

    }

    for(int i = 0;i<strlen(pathfile); i++){
        if(pathfile[i] == '/')  pos_last_slash = i+1;
    }

    ///Add expansion for
    for(int i = 0 ;i <=DIRENTRYCOUNT;i++)
       {
            if(strcmp(pathfile + pos_last_slash, navigating_dir.dir.entrylist[i].name) == 0)
                {
                 ///start by clearing the memory not necessary but good to see it in the hexdump

                for(int j = 0;j<BLOCKSIZE;j++) virtualDisk[navigating_dir.dir.entrylist[i].firstblock].data[j]='\0';

                    ///set file FAT to UNUSED

                    fat_checker = navigating_dir.dir.entrylist[i].firstblock;
                    while( fat_checker != ENDOFCHAIN)
                    {
                        fat_chain_to_unused[fat_chain_counter]=fat_checker;
                        fat_chain_counter++;
                        fat_checker=FAT[fat_checker];
                    }

                    for(int j=0;j<fat_chain_counter;j++)
                    {
                        printf("fat_checker: %d",fat_chain_to_unused[j]);

                        FAT[ fat_chain_to_unused[j] ]=UNUSED;

                    }
                    copyFAT();

                    /// start colapsing the entries could implement colapsing for expanded directories too but too much work and no points extra

                    for(int j=i;j < navigating_dir.dir.nextEntry;j++) navigating_dir.dir.entrylist[j] =navigating_dir.dir.entrylist[j+1];
                    navigating_dir.dir.nextEntry--;
                    writeblock(&navigating_dir.dir,navigating_dir.dir.start);



                }
       }
    printf("> myremove stop\n");
}

MyFILE * myfopen( const char * filename, const char * mode )
{
    printf("> myfopen start\n");
	MyFILE * file;

    file = malloc( sizeof(MyFILE) ) ;
    int fblocks = (MAXBLOCKS / FATENTRYCOUNT );

    if(filename[0]=='/')
        currentDirIndex = rootDirIndex;
    else
        currentDirIndex = realDirblockno;


    int f_loc = file_in_directory(virtualDisk[currentDirIndex].dir, filename);

    if( (f_loc == -1 && strcmp(mode,"w")==0) || (f_loc == -1 && strcmp(mode,"a")==0) )
    {

        ///File does not exist create it
        short int place_in_fat;
        place_in_fat = retUnusedSector();
        FAT[place_in_fat] = ENDOFCHAIN;
        currentDirIndex++;

        direntry_t entry_for_file;
        entry_for_file.filelength = 0;      ///EOF at the end
        entry_for_file.isdir = 0;

        char * name_from_path=NULL;
        int pos_last_slash=0;

        for(int i = 0;i<strlen(filename); i++){
            if(filename[i] == '/')  pos_last_slash = i+1;
        }

        strcpy(entry_for_file.name,filename+pos_last_slash);
        entry_for_file.unused = 1;
        entry_for_file.firstblock = place_in_fat;
        entry_for_file.modtime = time(0);


        diskblock_t writedirentry;
        for(int i=0;i<BLOCKSIZE;i++) writedirentry.data[i]='\0';
        writedirentry.dir = file_location(filename) ;

        if(writedirentry.dir.nextEntry == DIRENTRYCOUNT)
            if(FAT[writedirentry.dir.start] != ENDOFCHAIN)
                writedirentry.dir = virtualDisk[FAT[writedirentry.dir.start]].dir;
            else
                writedirentry.dir = expandDirectory(writedirentry.dir);

        writedirentry.dir.entrylist[writedirentry.dir.nextEntry] = entry_for_file;
        writedirentry.dir.nextEntry++;


        writeblock(&writedirentry, writedirentry.dir.start);

        ///TO DO : implement append

        strcpy(file->mode,mode);
        file->pos = 0;
        file->blockno = place_in_fat;
        strcpy(file->name,filename+pos_last_slash);
        file->dir_start = currentDirIndex;
        for(int i=0;i<BLOCKSIZE;i++) file->buffer.data[i]='\0';
    }
    else if (f_loc != -1 && strcmp(mode,"r") == 0 ){

        strcpy(file->mode,mode);
        file->pos = 0;
        file->blockno = virtualDisk[rootDirIndex].dir.entrylist[f_loc].firstblock;
        strcpy(file->name,filename);
    	printf("> myfopen stop\n");
		return file;
    }
	printf("> myfopen stop\n");
    return(file);
}

/* writedisk : writes virtual disk out to physical disk
 *
 * in: file name of stored virtual disk
 */

void writedisk ( const char * filename )
{
   //printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;

}

void readdisk ( const char * filename )
{
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
      fclose(dest) ;
}


/* the basic interface to the virtual disk
 * this moves memory around
 */

void writeblock ( diskblock_t * block, int block_address )
{
   //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
   memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
   //printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}


 void copyFAT()
{
    diskblock_t block;
    int fblocks = (MAXBLOCKS / FATENTRYCOUNT );
    for(int i=0; i < fblocks;i++)
        {
        for(int j = 0; j < FATENTRYCOUNT;j++)
            block.fat[j] = FAT[j + FATENTRYCOUNT*i];
        writeblock(&block,i+1);     ///first block is reserved
        }
}

void mychdir(char * path)
{
    printf("> mychdir start\n");
    int dirIndexer;
    if(path[0] == '/')
        dirIndexer = rootDirIndex;
    else
        dirIndexer = realDirblockno;

    ///navigate trough the directories to the wanted one end up in up in the directory block

    diskblock_t navigating_disk;
    navigating_disk  =  virtualDisk[dirIndexer];

    char path_tokenize[strlen(path)];
    strcpy(path_tokenize,path);
    char* path_dir;
    char* rest = path_tokenize;
    int check_dir = 0;
    check_dir = navigating_disk.dir.start;

    while ((path_dir = strtok_r(rest, "/", &rest))){
        //while(check_dir != 0)
        //{

            for(int i=0;i<=DIRENTRYCOUNT;i++)
            {
                if(strcmp(navigating_disk.dir.entrylist[i].name,path_dir)==0)
                {
                    navigating_disk = virtualDisk[ navigating_disk.dir.entrylist[i].firstblock ];

                    break ;
                }
            //}
            //check_dir = FAT[check_dir];
          //  navigating_disk = virtualDisk[check_dir];
        }

    }
    realDirblockno = navigating_disk.dir.entrylist[0].firstblock;


    printf("> mychdir stop\n");
}


void format ( )
{
    printf("> format start\n");
   diskblock_t block ;
   direntry_t  rootDir ;                // Create a root directory
   int         pos             = 0 ;
   int         fatentry        = 0 ;
   int         fatblocksneeded =  (MAXBLOCKS / FATENTRYCOUNT ) ;
   /* prepare block 0 : fill it with '\0',
    * use strcpy() to copy some text to it for test purposes
	* write block 0 to virtual disk
	*/
	for(int i = 0; i<=BLOCKSIZE; i++) block.data[i] = '\0';


    strcpy(block.data,"CS3026 Virtual Disk Headder" );      ///hmm lots of dead space buffer???
    writeblock(&block,0);


	/* prepare FAT table
	 * write FAT blocks to virtual disk
	 */
    for(int i = 0;i<MAXBLOCKS;i++) FAT[i] = UNUSED;


    FAT[0] = ENDOFCHAIN;
    FAT[1] = 2;
    FAT[2] = ENDOFCHAIN;
    copyFAT();


	 /* prepare root directory
	  * write root directory block to virtual disk
	  */

    for(int i = 0; i<=BLOCKSIZE; i++) block.data[i] = '\0';

    rootDir.isdir = 1;
    strcpy(rootDir.name,"/");
    rootDirIndex = 3;
    currentDir = &rootDir;

    block.dir.isdir = 1;
    direntry_t to_itself;               ///I know to parent exists in the real filesystem but useless implementation in this case
    to_itself.firstblock = rootDirIndex;
    strcpy(to_itself.name,rootDir.name);
    to_itself.isdir = 1;
    to_itself.unused = 1;
    to_itself.modtime = time(0);

    block.dir.entrylist[0] = to_itself;
    block.dir.nextEntry = 1;
    block.dir.start = rootDirIndex;
    writeblock(&block,3);
    FAT[3] = ENDOFCHAIN;
    copyFAT();
    rootDirIndex = 3;
    realDirblockno = 3;
	printf("> format stop\n");
}

char ** mylistdir(char * path)
{
    /// assume absolute
	printf("> mylistdir start\n");

    diskblock_t block_directory;

    if(path[0] == '/')
        currentDirIndex = rootDirIndex;
    else
        currentDirIndex = realDirblockno;
    block_directory.dir = virtualDisk[currentDirIndex].dir;
    //for(int i = 0; i< DIRENTRYCOUNT;i++)
    //    printf("check the entries in the current directory %s\n",block_directory.dir.entrylist[i].name);


    char path_tokenize[strlen(path)];
    strcpy(path_tokenize,path);

    char* path_dir;
    char* rest = path_tokenize;
    ///TO DO: Make it look into the expansion while returning the head of directory
    while ((path_dir = strtok_r(rest, "/", &rest))){
        for(int i = 0 ;i <=DIRENTRYCOUNT;i++)
        {
            if(strcmp(block_directory.dir.entrylist[i].name,path_dir )==0 )
            {
                block_directory.dir = virtualDisk[block_directory.dir.entrylist[i].firstblock].dir;
                break;
            }
        }

    }


    char ** ptr_ptr_file_list ;
    ptr_ptr_file_list = malloc(sizeof(char*)*10) ;
    for(int i = 0;i<10;i++) ptr_ptr_file_list[i]=  malloc(256*sizeof(char));

    int lst_counter=0;
    int check_dir;
    check_dir = block_directory.dir.start;


    block_directory.dir = virtualDisk[check_dir].dir;

    while(check_dir!=0)
    {
    for(int i = 0; i< DIRENTRYCOUNT;i++)
    {
        if(strlen(block_directory.dir.entrylist[i].name) > 0  )
            {
            strcpy(ptr_ptr_file_list[lst_counter], block_directory.dir.entrylist[i].name);
            lst_counter++;

            }
    }
    check_dir = FAT[check_dir];
    block_directory.dir = virtualDisk[check_dir ].dir;
    }
    printf("> mylistdir stop\n");

	return ptr_ptr_file_list;

}

dirblock_t expandDirectory(dirblock_t dir_to_epxand)
{
    diskblock_t expansion_dir;
    fatentry_t unused_sector;
    unused_sector = retUnusedSector();

    FAT[dir_to_epxand.start] = unused_sector;
    FAT[unused_sector]=ENDOFCHAIN;
    copyFAT();

    for(int i=0;i<BLOCKSIZE;i++) expansion_dir.data[i]='\0';
    expansion_dir.dir.isdir = 1;
    expansion_dir.dir.start = unused_sector;
    expansion_dir.dir.nextEntry = 0;
    return expansion_dir.dir;
}


void mymkdir(char * path)
{
	printf("> mymkdir start \n");

    direntry_t entry_directory;

    direntry_t to_itself;               ///I have the dirblock.start so this is kinda useless
    direntry_t to_parent;
    diskblock_t block_directory;
    diskblock_t new_block;

    for(int i=0;i<BLOCKSIZE;i++) block_directory.data[i]='\0';
    for(int i=0;i<BLOCKSIZE;i++) new_block.data[i]='\0';

    short int unusedSector;

    char path_tokenize[strlen(path)];
    strcpy(path_tokenize,path);


    if(path[0] == '/')
        currentDirIndex = rootDirIndex;
    else
        currentDirIndex = realDirblockno;
    for(int i=0;i<BLOCKSIZE;i++) block_directory.data[i]='\0';
    block_directory.dir = virtualDisk[currentDirIndex].dir;
    char* path_dir;
    char* rest = path_tokenize;

    while ((path_dir = strtok_r(rest, "/", &rest))){
        unusedSector = retUnusedSector();
        FAT[unusedSector]=ENDOFCHAIN;
        copyFAT();


        //update current directory to contain the new entry
        entry_directory.firstblock = unusedSector;

        strcpy(entry_directory.name,path_dir);

        entry_directory.modtime = time(0);
        entry_directory.isdir = 1;
        entry_directory.unused = 1;

         if(block_directory.dir.nextEntry == DIRENTRYCOUNT)
            if(FAT[block_directory.dir.start] != ENDOFCHAIN)
                block_directory = virtualDisk[FAT[block_directory.dir.start]];
            else
                block_directory.dir = expandDirectory(block_directory.dir);

        block_directory.dir.entrylist[block_directory.dir.nextEntry] = entry_directory;

        block_directory.dir.nextEntry++;
        block_directory.dir.start = currentDirIndex;
        writeblock(&block_directory.dir,currentDirIndex);

        ///format the space for the next directory should also add the . and .. here
        to_parent.firstblock = currentDirIndex;
        currentDirIndex = unusedSector;

        to_itself.firstblock = unusedSector;
        for(int i=0;i<MAXNAME;i++) to_parent.name[0]='\0';
        for(int i=0;i<MAXNAME;i++) to_itself.name[0]='\0';
        strcpy(to_parent.name,"..");
        strcpy(to_itself.name,".");

        to_itself.isdir=1;
        to_parent.isdir=1;
        to_itself.unused=1;
        to_parent.unused=1;
        to_itself.modtime=time(0);

        block_directory.dir = virtualDisk[currentDirIndex].dir;

        block_directory.dir.start = currentDirIndex;
        block_directory.dir.isdir = 1;
        block_directory.dir.nextEntry=0;

        block_directory.dir.entrylist[0] = to_itself;
        block_directory.dir.nextEntry++;
        block_directory.dir.entrylist[1] = to_parent;
        block_directory.dir.nextEntry++;

        writeblock(&block_directory.dir,currentDirIndex);

    }
	printf("> mymkdir stop\n");
}
