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
    int unused_sector;
    if( strcmp(stream->mode,"w") == 0)
    {

    unused_sector = retUnusedSector();

    if(stream->pos + 1 == BLOCKSIZE)
    {

        FAT[stream->blockno] = unused_sector;
        FAT[unused_sector] = ENDOFCHAIN;
        stream->blockno = unused_sector;
        stream->pos = 0;
    }


    stream->buffer.data[stream->pos] = b;
    stream->pos++;
    }
    writeblock(&stream->buffer.data, stream->blockno);

}

int myfgetc(MyFILE * stream)
{
    if(stream->pos == BLOCKSIZE-1)          ///1023
    {
        stream->blockno = FAT[stream->blockno];
        stream->pos = 0;
    }

    if(virtualDisk[stream->blockno].data[stream->pos] == EOF)
        return EOF;
    else
    {
        stream->pos++;
        return virtualDisk[stream->blockno].data[stream->pos];
    }

}

void myfclose(MyFILE * stream)
{
    ///make sure that you have space to write that EOF
    if(stream->pos != 0 )
    {
        stream->buffer.data[stream->pos] = EOF;
    }
    writeblock(&stream->buffer.data,stream->blockno); ///ASSUME ENOUGH SPACE EXISTS
    free(stream);
}


MyFILE * myfopen( const char * filename, const char * mode )
{
    MyFILE * file;
    file = malloc( sizeof(MyFILE) ) ;
    int fblocks = (MAXBLOCKS / FATENTRYCOUNT );
    int file_exists = 0;
    if(strcmp(mode,"r") == 0){


        for(int i = 4; i < MAXBLOCKS; i++)
        {
            if(strcmp(virtualDisk[i].data, filename) == 0)
            {
                file_exists = 1;
                file->blockno = i;
                file->pos = strlen(virtualDisk[i].data) +1 ;    ///go over the null byte
            }
        }

        if(file_exists == 0)
            return NULL;
    printf("\n\n\n Hey I am in reading \n\n\n");
    }
    else if(strcmp(mode,"a") == 0 ){
    } else{
        //alocate memory for a file with malloc

        strcpy(file->mode,mode);


        file->blockno = retUnusedSector();       /// position in memory

        file->pos = 0;
        strcpy(file->buffer.data + file->pos,filename);

        file->pos = strlen(filename)+1;

        FAT[file->blockno] = ENDOFCHAIN;
        copyFAT();
        writeblock(&file->buffer.data, file->blockno);
        }

    return(file);
}



/* writedisk : writes virtual disk out to physical disk
 *
 * in: file name of stored virtual disk
 */


void writedisk ( const char * filename )
{
   printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
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

    block.dir.isdir = 1;
    block.dir.nextEntry = ENDOFCHAIN;

    writeblock(&block,3);
    FAT[3] = ENDOFCHAIN;
    copyFAT();
    rootDirIndex = 3;

}


/* use this for testing
 */

void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

