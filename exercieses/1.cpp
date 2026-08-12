#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) 
{
	Disk disk_run;
	for(int j=0;j<4;j++)
	{
  		unsigned char buffer2[BLOCK_SIZE];
  		Disk::readBlock(buffer2,j);
  		
        for(int i = 0; i <BLOCK_SIZE; i++)
		std::cout <<"Block "<<j<<" Byte "<<i<<" -"<<(int)buffer2[i] <<;
	}

	return 0;
}
