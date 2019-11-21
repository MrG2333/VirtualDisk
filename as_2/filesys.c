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
int number_dir_entries = 1;
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

    short int unused_sector;
    int f_loc = file_in_directory(virtualDisk[rootDirIndex].dir, stream->name);

    virtualDisk[currentDirIndex].dir.entrylist[f_loc].filelength++;
    virtualDisk[currentDirIndex].dir.entrylist[f_loc].modtime = time(0);


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
    writeblock(&stream->buffer.data, stream->blockno);
}

int myfgetc(MyFILE * stream)
{
    int f_loc = file_in_directory(virtualDisk[rootDirIndex].dir,stream->name);

    int f_length = virtualDisk[rootDirIndex].dir.entrylist[f_loc].filelength;
    fatentry_t bln= stream->blockno - virtualDisk[rootDirIndex].dir.entrylist[f_loc].firstblock;
    int c_to_return;

    if(stream->pos == BLOCKSIZE)
    {
        stream->blockno = FAT[stream->blockno];
        stream->pos = 0;
    }

    if(stream->pos + (bln*BLOCKSIZE) < f_length)
        {
            c_to_return =virtualDisk[stream->blockno].data[stream->pos];
            stream->pos++;
            if(c_to_return == 255)
                return -1;
        }
return c_to_return;
}

void myfclose(MyFILE * stream)
{
    ///if there is no space for EOF go to the next block and place EOF
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

    writeblock(&stream->buffer.data,stream->blockno); ///ASSUME ENOUGH SPACE EXISTS
    free(stream);
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

    search_block.dir = virtualDisk[rootDirIndex].dir;

    char path_tokenize[strlen(path)];
    strcpy(path_tokenize,path);


    char* path_dir;
    char* rest = path_tokenize;

    while ((path_dir = strtok_r(rest, "/", &rest))){

        for(int i=0; i < DIRENTRYCOUNT;i++){
                if(strcmp(search_block.dir.entrylist[i].name,path_dir) == 0 && search_block.dir.entrylist[i].isdir == 1){
                    currentDirIndex = search_block.dir.entrylist[i].firstblock;
                    search_block.dir = virtualDisk[currentDirIndex].dir;
                }
            }
    }

    return search_block.dir;
}



MyFILE * myfopen( const char * filename, const char * mode )
{
    MyFILE * file;
    file = malloc( sizeof(MyFILE) ) ;
    int fblocks = (MAXBLOCKS / FATENTRYCOUNT );

    int f_loc = file_in_directory(virtualDisk[rootDirIndex].dir, filename);

    if( (f_loc == -1 && strcmp(mode,"w")==0) || (f_loc == -1 && strcmp(mode,"a")==0) )
    {
        ///File does not exist create it
        short int place_in_fat;
        place_in_fat = retUnusedSector();
        FAT[place_in_fat] = ENDOFCHAIN;
        currentDirIndex++;

        direntry_t entry_for_file;
        entry_for_file.filelength = 1;      ///EOF at the end
        entry_for_file.isdir = 0;


        int pos_last_slash;

        for(int i = 0;i<strlen(filename); i++){
            if(filename[i] == '/')  pos_last_slash = i+1;
        }

        strcpy(entry_for_file.name,filename+pos_last_slash);
        entry_for_file.unused = 1;
        entry_for_file.firstblock = place_in_fat;
        entry_for_file.modtime = time(0);


        diskblock_t writedirentry;

        writedirentry.dir = file_location(filename) ;           ///file name becomes path

        writedirentry.dir.entrylist[writedirentry.dir.nextEntry] = entry_for_file;
        writedirentry.dir.nextEntry++;

        writeblock(&writedirentry, currentDirIndex);

        ///TO DO : implement append

        strcpy(file->mode,mode);
        file->pos = 0;
        file->blockno = place_in_fat;
        strcpy(file->name,filename+pos_last_slash);

        number_dir_entries++;
    }
    else if (f_loc != -1 && strcmp(mode,"r") == 0 ){

        strcpy(file->mode,mode);
        file->pos = 0;
        file->blockno = virtualDisk[rootDirIndex].dir.entrylist[f_loc].firstblock;
        strcpy(file->name,filename);
        return file;
    }
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


/* read and write FAT
 *
 * please note: a FAT entry is a short, this is a 16-bit word, or 2 bytes
 *              our blocksize for the virtual disk is 1024, therefore
 *              we can store 512 FAT entries in one block
 *
 *              how many disk blocks do we need to store the complete FAT:
 *              - our virtual disk has MAXBLOCKS blocks, which is currently 1024
 *                each block is 1024 bytes long
 *              - our FAT has MAXBLOCKS entries, which is currently 1024
 *                each FAT entry is a fatentry_t, which is currently 2 bytes
 *              - we need (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))) blocks to store the
 *                FAT
 *              - each block can hold (BLOCKSIZE / sizeof(fatentry_t)) fat entries
 */

/* implement format()
 */

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


void format ( )
{
   diskblock_t block ;
   direntry_t  rootDir ;                // Create a root directory

   int    fatblocksneeded =  (MAXBLOCKS / FATENTRYCOUNT ) ;
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
    rootDir.firstblock = FAT[3];
    rootDirIndex = 3;


    block.dir.isdir = 1;
    block.dir.nextEntry = 0;

    writeblock(&block,3);
    FAT[3] = ENDOFCHAIN;
    currentDir = &virtualDisk[3].dir;
    copyFAT();
    rootDirIndex = 3;

}

char ** mylistdir(char * path)
{
    /// assume absolute
    diskblock_t block_directory;

    currentDirIndex = rootDirIndex;
    block_directory.dir = virtualDisk[currentDirIndex].dir;


    char path_tokenize[strlen(path)];
    strcpy(path_tokenize,path);

    char* path_dir;
    char* rest = path_tokenize;

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

    char ** ptr_ptr_file_list = (char * ) malloc(sizeof(char) * DIRENTRYCOUNT);

         for(int i = 0; i< DIRENTRYCOUNT;i++)
    {
        if(strlen(block_directory.dir.entrylist[i].name) > 0)
            {
            ptr_ptr_file_list[i] = &block_directory.dir.entrylist[i].name;
            }


    }
    return ptr_ptr_file_list;

}


void mymkdir(char * path)
{
    direntry_t entry_directory;
    diskblock_t block_directory;
    short int unusedSector;


    char path_tokenize[strlen(path)];
    strcpy(path_tokenize,path);

    currentDirIndex = rootDirIndex;
    block_directory.dir = virtualDisk[currentDirIndex].dir;

    char* path_dir;
    char* rest = path_tokenize;

    while ((path_dir = strtok_r(rest, "/", &rest))){

        unusedSector = retUnusedSector();

        FAT[unusedSector]=ENDOFCHAIN;
        copyFAT();

        entry_directory.firstblock = unusedSector;

        strcpy(entry_directory.name,path_dir);
        entry_directory.modtime = time(0);
        entry_directory.isdir = 1;
        entry_directory.unused = 1;

        block_directory.dir.isdir=1;
        block_directory.dir.entrylist[block_directory.dir.nextEntry] = entry_directory;
        block_directory.dir.nextEntry++;
        writeblock(&block_directory,currentDirIndex);

        currentDirIndex = unusedSector;
        block_directory.dir = virtualDisk[currentDirIndex].dir;
    }
}



/* use this for testing
 */

void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

