# Unix File System
This program was written for CSS430

## Updates:   
Merged fsRead_pseudo with main branch   
This branch will continue to build off of the fsWrite() pseudocode   
   
## fsRead()   
The function first converts a given fd to the file's inode number, called inum.    
Inum is used to find the file's inode in the system wide open table.    
Then, calculate the starting and ending FBNs that will map to DBNs.    
Read the backed, physical block equivalent of each logical block into memory (with a buffer)      
   
Depending on if the cursor starts and ends in the middle of blocks, we only append part of the data (from physical memory) into the user's buffer.   
The current scheme I have is simply iterating through every logical file block and retreiving data from it's equivalent disk block because as far as i am aware, you can only read one block from disk at a time    
   
Example:   
   Lets say the cursor is currently at 300 and the user wants to read in 1500 bytes   
   Then we calculate the FBNs. So we will go from FBNs 0-3   
   These FBNs map to a specific DBN in the inode   
   FBN[0] could map to DBN[123] in this example. 
   We proceed to read in data from FBN[0], then FBN[1], then FBN[2] and FBN[3] from disk.    
   After each block is read, the data we read from disk is appeneded to the user's buffer depending on some conditions    
   If the user's cursor starts or ends in the middle of a block, only take the part we need    
   Append the data onto the user's buffer     
      
   After loop, update cursor (seek pointer)
