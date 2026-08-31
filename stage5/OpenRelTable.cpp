#include "OpenRelTable.h"
#include <cstdlib>
#include <cstring>
OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
OpenRelTable::OpenRelTable() 
{

	// initialize relCache and attrCache with nullptr
	for (int i = 0; i < MAX_OPEN; ++i) 
	{
		RelCacheTable::relCache[i] = nullptr;
		AttrCacheTable::attrCache[i] = nullptr;
		
		tableMetaInfo[i].free = true;
	}

	/************ Setting up Relation Cache entries ************/
	// (we need to populate relation cache with entries for the relation catalog
	//  and attribute catalog.)

	/**** setting up Relation Catalog relation in the Relation Cache Table****/
	RecBuffer relCatBlock(RELCAT_BLOCK);

	Attribute relCatRecord[RELCAT_NO_ATTRS];
	relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

	struct RelCacheEntry relCacheEntry;
	RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
	relCacheEntry.recId.block = RELCAT_BLOCK;
	relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

	// allocate this on the heap because we want it to persist outside this function
	RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
	*(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

	/**** setting up Attribute Catalog relation in the Relation Cache Table ****/

	// set up the relation cache entry for the attribute catalog similarly
	// from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
	
	relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
	RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
	relCacheEntry.recId.block = RELCAT_BLOCK;
	relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
	
	// set the value at RelCacheTable::relCache[ATTRCAT_RELID]
	RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
	*(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;


	/************ Setting up Attribute cache entries ************/
	// (we need to populate attribute cache with entries for the relation catalog
	//  and attribute catalog.)

	/**** setting up Relation Catalog relation in the Attribute Cache Table ****/
	RecBuffer attrCatBlock(ATTRCAT_BLOCK);

	Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

	// iterate through all the attributes of the relation catalog and create a linked
	// list of AttrCacheEntry (slots 0 to 5)
	// for each of the entries, set
	//    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
	//    attrCacheEntry.recId.slot = i   (0 to 5)
	//    and attrCacheEntry.next appropriately
	// NOTE: allocate each entry dynamically using malloc

	// set the next field in the last entry to nullptr
	
	AttrCacheEntry* prev=nullptr;
	AttrCacheEntry* head=nullptr;
	
	
	for(int i=0;i<ATTRCAT_NO_ATTRS;i++)
	{
		attrCatBlock.getRecord(attrCatRecord,i);
		
		AttrCacheEntry* curr=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
		
		
		AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&curr->attrCatEntry);
		
		curr->recId.block=ATTRCAT_BLOCK;
		curr->recId.slot=i;
		curr->next=nullptr;
		
		if(head==nullptr)
		{
			head=curr;
			prev=curr;
		}
		else
		{
			prev->next=curr;
			prev=prev->next;
		}		
	}

	AttrCacheTable::attrCache[RELCAT_RELID] = head;/* head of the linked list */

	/**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

	// set up the attributes of the attribute cache similarly.
	// read slots 6-11 from attrCatBlock and initialise recId appropriately
	
	prev=nullptr;
	head=nullptr;
	
	for(int i=6;i<6+ATTRCAT_NO_ATTRS;i++)
	{
		attrCatBlock.getRecord(attrCatRecord,i);
		
		AttrCacheEntry* curr=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
		
		
		AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&curr->attrCatEntry);
		
		curr->recId.block=ATTRCAT_BLOCK;
		curr->recId.slot=i;
		curr->next=nullptr;
		
		if(head==nullptr)
		{
			head=curr;
			prev=curr;
		}
		else
		{
			prev->next=curr;
			prev=prev->next;
		}		
	}

	// set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
	AttrCacheTable::attrCache[ATTRCAT_RELID] = head;
	
	tableMetaInfo[RELCAT_RELID].free=false;
	strcpy(tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);
	
	tableMetaInfo[ATTRCAT_RELID].free=false;
	strcpy(tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);
}

OpenRelTable::~OpenRelTable() 
{
	// free all the memory that you allocated in the constructor
	
	// close all open relations (from rel-id = 2 onwards. Why?)
	for (int i = 2; i < MAX_OPEN; ++i) 
	{
		if (!tableMetaInfo[i].free) 
		{
			OpenRelTable::closeRel(i); // we will implement this function later
		}
	}

	// free the memory allocated for rel-id 0 and 1 in the caches
	
	if(RelCacheTable::relCache[RELCAT_RELID]!=nullptr)
	{
		free(RelCacheTable::relCache[RELCAT_RELID]);
		RelCacheTable::relCache[RELCAT_RELID]=nullptr;
	}
	
	if(RelCacheTable::relCache[ATTRCAT_RELID]!=nullptr)
	{
		free(RelCacheTable::relCache[ATTRCAT_RELID]);
		RelCacheTable::relCache[ATTRCAT_RELID]=nullptr;
	}
	
	AttrCacheEntry* temp=AttrCacheTable::attrCache[RELCAT_RELID];
	
	while(temp!=nullptr)
	{
		AttrCacheEntry* curr=temp->next;
		free(temp);
		temp=curr;
	}
	
	temp=AttrCacheTable::attrCache[ATTRCAT_RELID];
	
	while(temp!=nullptr)
	{
		AttrCacheEntry* curr=temp->next;
		free(temp);
		temp=curr;
	}
}
/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) 
{

	// if relname is RELCAT_RELNAME, return RELCAT_RELID
	// if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID
	for (int i = 0; i < MAX_OPEN; ++i) 
	{
		if(tableMetaInfo[i].free==false && strcmp(tableMetaInfo[i].relName,relName)==0)
		return i;
	}
	return E_RELNOTOPEN;
}
int OpenRelTable::openRel(char relName[ATTR_SIZE]) 
{
	int id=getRelId(relName);

	if(id!=E_RELNOTOPEN)/* the relation `relName` already has an entry in the Open Relation Table */
	{
		// (checked using OpenRelTable::getRelId())

		// return that relation id;
		return id;
	}

	/* find a free slot in the Open Relation Table
		 using OpenRelTable::getFreeOpenRelTableEntry(). */
	id=getFreeOpenRelTableEntry();

	if (id==E_CACHEFULL)/* free slot not available */
	{
		return E_CACHEFULL;
	}

	// let relId be used to store the free slot.
	int relId;
	relId=id;

	/****** Setting up Relation Cache entry for the relation ******/

	/* search for the entry with relation name, relName, in the Relation Catalog using
			BlockAccess::linearSearch().
			Care should be taken to reset the searchIndex of the relation RELCAT_RELID
			before calling linearSearch().*/
			
	RelCacheTable::relCache[RELCAT_RELID]->searchIndex={-1,-1};

	// relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
	RecId relcatRecId;
	Attribute attr;
	strcpy(attr.sVal,relName);
	
	relcatRecId=BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,attr,EQ);

	if (relcatRecId.block==-1 && relcatRecId.slot==-1) /* relcatRecId == {-1, -1} */
	{
		// (the relation is not found in the Relation Catalog.)
		return E_RELNOTEXIST;
	}

	/* read the record entry corresponding to relcatRecId and create a relCacheEntry
			on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
			update the recId field of this Relation Cache entry to relcatRecId.
			use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
		NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
	*/
	RecBuffer record(relcatRecId.block);
	//HeadInfo header;
	//record.getHeader(&head);
	
	Attribute record_content[RELCAT_NO_ATTRS];
	record.getRecord(record_content,relcatRecId.slot);
	
	struct RelCacheEntry relCacheEntry;
	RelCacheTable::recordToRelCatEntry(record_content, &relCacheEntry.relCatEntry);
	relCacheEntry.recId.block = relcatRecId.block;
	relCacheEntry.recId.slot = relcatRecId.slot;

	RelCacheTable::relCache[relId] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
	*(RelCacheTable::relCache[relId]) = relCacheEntry;
	
	
	/****** Setting up Attribute Cache entry for the relation ******/

	// let listHead be used to hold the head of the linked list of attrCache entries.
	AttrCacheEntry* listHead;

	/*iterate over all the entries in the Attribute Catalog corresponding to each
	attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
	care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
	corresponding to Attribute Catalog before the first call to linearSearch().*/
	
	RelCacheTable::relCache[ATTRCAT_RELID]->searchIndex={-1,-1};
	
	AttrCacheEntry* prev=nullptr;
	AttrCacheEntry* head=nullptr;
	
	while(1)
	{
			/* let attrcatRecId store a valid record id an entry of the relation, relName,
			in the Attribute Catalog.*/
			RecId attrcatRecId;
			attrcatRecId=BlockAccess::linearSearch(ATTRCAT_RELID,(char*)ATTRCAT_ATTR_RELNAME,attr,EQ);
			
			if(attrcatRecId.block==-1 && attrcatRecId.slot==-1)
			break;

			/* read the record entry corresponding to attrcatRecId and create an
			Attribute Cache entry on it using RecBuffer::getRecord() and
			AttrCacheTable::recordToAttrCatEntry().
			update the recId field of this Attribute Cache entry to attrcatRecId.
			add the Attribute Cache entry to the linked list of listHead .*/
			// NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
			
			RecBuffer attrCatBlock(attrcatRecId.block);
			Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
			
			attrCatBlock.getRecord(attrCatRecord,attrcatRecId.slot);
		
			AttrCacheEntry* curr=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
			
			
			AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&curr->attrCatEntry);
			
			curr->recId.block=attrcatRecId.block;
			curr->recId.slot=attrcatRecId.slot;
			curr->next=nullptr;
			
			if(head==nullptr)
			{
				head=curr;
				prev=curr;
			}
			else
			{
				prev->next=curr;
				prev=prev->next;
			}
	}
	AttrCacheTable::attrCache[relId] = head;

	// set the relIdth entry of the AttrCacheTable to listHead.

	/****** Setting up metadata in the Open Relation Table for the relation******/

	// update the relIdth entry of the tableMetaInfo with free as false and
	// relName as the input.
	
	tableMetaInfo[relId].free=false;
	strcpy(tableMetaInfo[relId].relName,relName);

	return relId;
}
int OpenRelTable::closeRel(int relId) 
{
	if (relId==0 || relId==1 ) /* rel-id corresponds to relation catalog or attribute catalog*/
	{
		return E_NOTPERMITTED;
	}

	if (relId<0 || relId>=MAX_OPEN) /* 0 <= relId < MAX_OPEN */
	{
		return E_OUTOFBOUND;
	}

	if (tableMetaInfo[relId].free==true) /* rel-id corresponds to a free slot*/
	{
		return E_RELNOTOPEN;
	}

	// free the memory allocated in the relation and attribute caches which was
	// allocated in the OpenRelTable::openRel() function
	free(RelCacheTable::relCache[relId]);
	
	AttrCacheEntry* temp=AttrCacheTable::attrCache[relId];
	
	while(temp!=nullptr)
	{
		AttrCacheEntry* curr=temp->next;
		free(temp);
		temp=curr;
	}

	// update `tableMetaInfo` to set `relId` as a free slot
	// update `relCache` and `attrCache` to set the entry at `relId` to nullptr
	tableMetaInfo[relId].free=true;
	
	RelCacheTable::relCache[relId]=nullptr;
	AttrCacheTable::attrCache[relId]=nullptr;

	return SUCCESS;
}
int OpenRelTable::getFreeOpenRelTableEntry() 
{

	/* traverse through the tableMetaInfo array,
		find a free entry in the Open Relation Table.*/

	// if found return the relation id, else return E_CACHEFULL.
	
	for (int i = 0; i < MAX_OPEN; ++i) 
	{
		if(tableMetaInfo[i].free==true)
		return i;
	}
	return E_CACHEFULL;
}
