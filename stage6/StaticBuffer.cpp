#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer()
{

	// initialise all blocks as free
	for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
	{
		metainfo[bufferIndex].free = true;
		metainfo[bufferIndex].dirty = false;
		metainfo[bufferIndex].blockNum=-1;
		metainfo[bufferIndex].timeStamp=-1;
	}
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer()
{
	/*iterate through all the buffer blocks,
    write back blocks with metainfo as free=false,dirty=true
    using Disk::writeBlock()
    */
    
    for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
	{
		if(metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty == true)
		{
			Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
			metainfo[bufferIndex].dirty=false;
			
		}
		
	}
}
int StaticBuffer::getFreeBuffer(int blockNum)
{
    // Check if blockNum is valid (non zero and less than DISK_BLOCKS)
    // and return E_OUTOFBOUND if not valid.
    
    if (blockNum < 0 || blockNum > DISK_BLOCKS)
	{
		return E_OUTOFBOUND;
	}
	int allocatedBuffer;
	
    // increase the timeStamp in metaInfo of all occupied buffers.
    
    int bufferIndex=0;
    
    for (;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
	{
		if(metainfo[bufferIndex].free==true)
		metainfo[bufferIndex].timeStamp+=1;
		
	}


    // let bufferNum be used to store the buffer number of the free/freed buffer.
    int bufferNum;
    
    
    bufferIndex=0;

    // iterate through metainfo and check if there is any buffer free
    
    for (;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
	{
		if(metainfo[bufferIndex].free==true)
		{
			
			break;
		}
		
	}

    // if a free buffer is available, set bufferNum = index of that free buffer.
    
    // if a free buffer is not available,
    //     find the buffer with the largest timestamp
    //     IF IT IS DIRTY, write back to the disk using Disk::writeBlock()
    //     set bufferNum = index of this buffer
    
    
    if(bufferIndex<BUFFER_CAPACITY)
    bufferNum=bufferIndex;
    else
    {
		bufferIndex=0;
		int max=0;
		int k=0;
		
		for (;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
		{
			if(metainfo[bufferIndex].timeStamp>max)
			{
				max=metainfo[bufferIndex].timeStamp;
				k=bufferIndex;
			}			
		}
		
		bufferIndex=k;
		
		if(metainfo[k].dirty==true)
		{
			Disk::writeBlock(blocks[k],metainfo[k].blockNum);
			metainfo[k].dirty=false;
		}
		bufferNum=bufferIndex;
		
	}

    // update the metaInfo entry corresponding to bufferNum with
    // free:false, dirty:false, blockNum:the input block number, timeStamp:0.
    
    metainfo[bufferNum].free=false;
    metainfo[bufferNum].dirty=false;
    metainfo[bufferNum].blockNum=blockNum;
    metainfo[bufferNum].timeStamp=0;
    
    
    return bufferNum;

    // return the bufferNum.
}
/* Get the buffer index where a particular block is stored
	 or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum)
{
	// Check if blockNum is valid (between zero and DISK_BLOCKS)
	// and return E_OUTOFBOUND if not valid.
	
	if (blockNum < 0 || blockNum > DISK_BLOCKS)
	{
		return E_OUTOFBOUND;
	}

	// find and return the bufferIndex which corresponds to blockNum (check metainfo)
	
	for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
	{
		if(metainfo[bufferIndex].free==false && metainfo[bufferIndex].blockNum == blockNum)
		return bufferIndex;
	}

	// if block is not in the buffer
	return E_BLOCKNOTINBUFFER;
}
int StaticBuffer::setDirtyBit(int blockNum)
{
    // find the buffer index corresponding to the block using getBufferNum().
    int bufferNum=StaticBuffer::getBufferNum(blockNum);

    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
    
    if(bufferNum==E_BLOCKNOTINBUFFER)
    return bufferNum;

    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND

    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo

    // return SUCCESS
    
    if(bufferNum==E_OUTOFBOUND)
		return bufferNum;
    else
    metainfo[bufferNum].dirty=true;
    
    return SUCCESS;
}
