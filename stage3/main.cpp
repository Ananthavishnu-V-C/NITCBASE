#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) 
{
	Disk disk_run;
	StaticBuffer buffer;
	OpenRelTable cache;

	// create objects for the relation catalog and attribute catalog
	//RecBuffer relCatBuffer(RELCAT_BLOCK);
	//RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

	
	RelCatEntry relCatBuf;

	for (int i=0;i<2;i++) 
	{
		RelCacheTable::getRelCatEntry(i,&relCatBuf);
		printf("Relation: %s\n", relCatBuf.relName);
		
		AttrCatEntry attrCatBuf;

		for (int j=0;j<relCatBuf.numAttrs;j++) 
		{
			AttrCacheTable::getAttrCatEntry(i,j,&attrCatBuf);
			
			const char *type = attrCatBuf.attrType == NUMBER ? "NUM" : "STR";
			
			printf("  %s: %s\n",attrCatBuf.attrName, type);
			
			
		}
		printf("\n");
	}

	return 0;
}



