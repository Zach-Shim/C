// ============================================================================
// fs.c - user FileSytem API
// ============================================================================

#include "bfs.h"
#include "fs.h"

// ============================================================================
// Close the file currently open on file descriptor 'fd'.
// ============================================================================
i32 fsClose(i32 fd) { 
  i32 inum = bfsFdToInum(fd);
  bfsDerefOFT(inum);
  return 0; 
}



// ============================================================================
// Create the file called 'fname'.  Overwrite, if it already exsists.
// On success, return its file descriptor.  On failure, EFNF
// ============================================================================
i32 fsCreate(str fname) {
  i32 inum = bfsCreateFile(fname);
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Format the BFS disk by initializing the SuperBlock, Inodes, Directory and 
// Freelist.  On succes, return 0.  On failure, abort
// ============================================================================
i32 fsFormat() {
  FILE* fp = fopen(BFSDISK, "w+b");
  if (fp == NULL) FATAL(EDISKCREATE);

  i32 ret = bfsInitSuper(fp);               // initialize Super block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitInodes(fp);                  // initialize Inodes block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitDir(fp);                     // initialize Dir block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitFreeList();                  // initialize Freelist
  if (ret != 0) { fclose(fp); FATAL(ret); }

  fclose(fp);
  return 0;
}


// ============================================================================
// Mount the BFS disk.  It must already exist
// ============================================================================
i32 fsMount() {
  FILE* fp = fopen(BFSDISK, "rb");
  if (fp == NULL) FATAL(ENODISK);           // BFSDISK not found
  fclose(fp);
  return 0;
}



// ============================================================================
// Open the existing file called 'fname'.  On success, return its file 
// descriptor.  On failure, return EFNF
// ============================================================================
i32 fsOpen(str fname) {
  i32 inum = bfsLookupFile(fname);        // lookup 'fname' in Directory
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Read 'numb' bytes of data from the cursor in the file currently fsOpen'd on
// File Descriptor 'fd' into 'user_Buf'.  On success, return actual number of bytes
// read (may be less than 'numb' if we hit EOF).  On failure, abort
// read up to count bytes from file descriptor fd
// ============================================================================
i32 fsRead(i32 fd, i32 bytes_To_Read, void* buf) {
  // error check
  if(bytes_To_Read <= 0)  return 0;

  // open file table already has my file desciptor inside itself;   reference the system open file table using my fd
  i32 inum = bfsFdToInum(fd);                                       // convert the file descriptor to an inode number used to index the system wide open file table  
  i32 cursor_start_ = bfsTell(fd);                                  // now i have the cursor of the file with file descirptor fd; cursor was held in the file's inode
  i32 cursor_end_ = cursor_start_ + bytes_To_Read;

  // error checks
  if(cursor_start_ == bfsGetSize(inum))                             // if the cursor (offset) is already at the end of the file, then return 0 (no bytes read)
    return 0;                                                       // no bytes read because we are at the end of the file
  if(cursor_end_ > bfsGetSize(inum))                                // beyond size of file
    cursor_end_ = bfsGetSize(inum);                                 // make the cursor read up to only the end of the file

  i32 startingBlock = floor(cursor_start_ / BYTESPERBLOCK);         // gets the FBN that we start from (we start at the current position of the cursor)
  i32 endingBlock = floor(cursor_end_ / BYTESPERBLOCK);           // get the FBN of last block that will be read (add the amount of bytes that you want to read onto the current byte address to get ending address)
  //printf("starting block number:  %i\n", cursor_start_ / BYTESPERBLOCK);
  //printf("ending block number:  %i\n", cursor_end_ / BYTESPERBLOCK);
  i32 cStart_Offset  = cursor_start_ % BYTESPERBLOCK;               // the number of bytes into the starting logical block of the cursor pointer. Example: If cursor is currently at byte 300 (logical block), then 300 % 512 = 300 
  i32 cEnd_Offset = cursor_end_ % BYTESPERBLOCK;                    // the number of bytes into the end logical block of the cursor pointer after bytes_To_Read is added on. 

  i32 numOfBytesRead = 0;  
  
  i8 bioBuf[bytes_To_Read];
  for(int i = startingBlock; i <= endingBlock; i++) {                            // pass in FBN i to read the corresponding disk block number, read the necessary amount of bytes, and copy into user's buffer
    if(bytes_To_Read - numOfBytesRead == 0) break;
    // read in FBN i from disk
    i8 readBuf[BYTESPERBLOCK];                                                 // can only read whole disk block (all or nothing)
    bfsRead(inum, i, readBuf);                                                   // function reads from raw disk block, readBuf should be populated with data\
    
    if(startingBlock == endingBlock)                                             // cursor starts in the middle of a block and bytes to read ends in the same block (i.e., cursor = 300, bytes_To_Read = 100)
    {
      i32 target_bytes = cEnd_Offset - cStart_Offset;
      memcpy(buf, readBuf+cStart_Offset, target_bytes);                       // append what we read onto the user's buffer; copies (cEnd_Offset - cStart_Offset) many bytes starting from the starting position of the cursor 
      numOfBytesRead += target_bytes;
    }
    else if(i == startingBlock && cStart_Offset % BYTESPERBLOCK != 0)            // if cursor doesnt begin at the start of a block
    {
      memcpy(buf, readBuf+cStart_Offset, BYTESPERBLOCK - cStart_Offset);        // append what we read onto the user's buffer; copies everything from cursor position to end of block
      numOfBytesRead += BYTESPERBLOCK - cStart_Offset;
    }
    else if(i == endingBlock && cEnd_Offset % BYTESPERBLOCK != 0)                // if cursor does not end at the start/end of a block
    {               
      memcpy(buf+numOfBytesRead, readBuf, cEnd_Offset);                        // append what we read up to the cursor's end address/offset onto the user's buffer
      numOfBytesRead += (cursor_start_ + bytes_To_Read) - (endingBlock * BYTESPERBLOCK);
    }
    else                                                                         // else read a full block; inefficient, might change later    
    {
      memcpy(buf+numOfBytesRead, readBuf, BYTESPERBLOCK);                       // append what we read onto the user's buffer
      numOfBytesRead += BYTESPERBLOCK;
    }
  } // end of loop
  
  bfsSetCursor(inum, (cursor_start_ + numOfBytesRead));     // update cursor
  return numOfBytesRead;
}



// ============================================================================
// Write 'numb' bytes of data from 'buf' into the file currently fsOpen'd on
// filedescriptor 'fd'.  The write starts at the current file offset for the
// destination file.  On success, return 0.  On failure, abort
// ============================================================================
i32 fsWrite(i32 fd, i32 bytes_To_Write, void* buf) {
  // error check; none or negative number of bytes to transfer
  if(bytes_To_Write <= 0)  FATAL(ENEGNUMB);

  // open file table already has my file desciptor inside itself;   reference the system open file table using my fd
  i32 inum = bfsFdToInum(fd);                                       // convert the file descriptor to an inode number used to index the system wide open file table  

  i32 cursor_start_ = bfsTell(fd);                                  // Find current cursor position 
  i32 cursor_end_ = cursor_start_ + bytes_To_Write;

  i32 startingFBN = floor(cursor_start_ / BYTESPERBLOCK);           // Calculate which FBN holds that offset 
  i32 endingFBN = floor((cursor_end_) / BYTESPERBLOCK);           // get the FBN of last block that will be read (add the amount of bytes that you want to read onto the current byte address to get ending address)

  i32 cStart_Offset  = cursor_start_ % BYTESPERBLOCK;               // the number of bytes into the starting logical block of the cursor pointer. Example: If cursor is currently at byte 300 (logical block), then 300 % 512 = 300 
  i32 cEnd_Offset = cursor_end_ % BYTESPERBLOCK;                    // the number of bytes into the end logical block of the cursor pointer after bytes_To_Read is added on. 

  i32 startingDBN = bfsFbnToDbn(inum, startingFBN);
  i32 endingDBN = bfsFbnToDbn(inum, endingFBN);
  // if user wants to write beyond current size of file (aka doesnt only want to overwrite old data) and an indirect block has not been intitialized
  i32 newBlock = 0;                                               // bool check
  if(endingDBN == ENODBN)                                         // ending offset is > current size of file AND end cursor position block cannot be found in the first five indexes of the inode (direct access) AND indirect block not yet allocated
  {
    bfsExtend(inum, endingFBN);                                   // handles extending the file block allocation for us
    newBlock = 1;
  }
  //printf("check1\n");
  i32 totalSize = (((endingFBN-startingFBN)+1) * BYTESPERBLOCK);
  i8 bioBuf[totalSize];                               // Allocate a buffer of the entire file size (in terms of entire blocks)
  
  bfsRead(inum, startingFBN, bioBuf);                                                 // function reads from raw disk block, the beginning of bioBuf should be populated with data
  bfsRead(inum, endingFBN, bioBuf+(endingFBN * BYTESPERBLOCK));                       // function reads from raw disk block, the beginning of bioBuf shouldbe populated with data
  
  memcpy(bioBuf+cursor_start_, buf, bytes_To_Write); 
  //printf("check2\n");
  i32 numOfBytesWritten = 0;
  for(int i = startingDBN; i <= endingDBN; i++) {
    i8 toWriteBuf[BYTESPERBLOCK] = {0};
    memcpy(toWriteBuf, bioBuf+(numOfBytesWritten), BYTESPERBLOCK);          // copy next block to write to disk; all 512 bytes
    bioWrite(i, toWriteBuf);                                                // write next block to disk
    numOfBytesWritten += BYTESPERBLOCK;
  }
  
  // if size of the file has increased then update size of file's inode to exact size
  i32 oldSize = bfsGetSize(inum);
  if(bfsGetSize(inum) < cursor_end_) {
    bfsSetSize(inum, bfsGetSize(inum) + (cursor_end_ - bfsGetSize(inum)));
  }  
    
  bfsSetCursor(inum, (cursor_start_ + bytes_To_Write));     // update cursor
  return 0;   
}



// ============================================================================
// Move the cursor for the file currently open on File Descriptor 'fd' to the
// byte-offset 'offset'.  'whence' can be any of:
//
//  SEEK_SET : set cursor to 'offset'
//  SEEK_CUR : add 'offset' to the current cursor
//  SEEK_END : add 'offset' to the size of the file
//
// On success, return 0.  On failure, abort
// ============================================================================
i32 fsSeek(i32 fd, i32 offset, i32 whence) {

  if (offset < 0) FATAL(EBADCURS);
 
  i32 inum = bfsFdToInum(fd);
  i32 ofte = bfsFindOFTE(inum);
  
  switch(whence) {
    case SEEK_SET:
      g_oft[ofte].curs = offset;
      break;
    case SEEK_CUR:
      g_oft[ofte].curs += offset;
      break;
    case SEEK_END: {
        i32 end = fsSize(fd);
        g_oft[ofte].curs = end + offset;
        break;
      }
    default:
        FATAL(EBADWHENCE);
  }
  return 0;
}



// ============================================================================
// Return the cursor position for the file open on File Descriptor 'fd'
// ============================================================================
i32 fsTell(i32 fd) {
  return bfsTell(fd);
}



// ============================================================================
// Retrieve the current file size in bytes.  This depends on the highest offset
// written to the file, or the highest offset set with the fsSeek function.  On
// success, return the file size.  On failure, abort
// ============================================================================
i32 fsSize(i32 fd) {
  i32 inum = bfsFdToInum(fd);
  return bfsGetSize(inum);
}
