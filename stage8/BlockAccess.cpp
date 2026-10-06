#include "BlockAccess.h"

#include <cstring>
#include <iostream>
RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) 
{
	
		// get the previous search index of the relation relId from the relation cache
		// (use RelCacheTable::getSearchIndex() function)
		
		RecId prevRecId;
		RelCacheTable::getSearchIndex(relId,&prevRecId);

		// let block and slot denote the record id of the record being currently checked
		int block=-1;
		int slot=-1;

		// if the current search index record is invalid(i.e. both block and slot = -1)
		if (prevRecId.block == -1 && prevRecId.slot == -1)
		{
				// (no hits from previous search; search should start from the
				// first record itself)

				// get the first record block of the relation from the relation cache
				// (use RelCacheTable::getRelCatEntry() function of Cache Layer)
				RelCatEntry relCatBuf;
				RelCacheTable::getRelCatEntry(relId,&relCatBuf);

				// block = first record block of the relation
				// slot = 0
				block=relCatBuf.firstBlk;
				slot=0;
		}
		else
		{
				// (there is a hit from previous search; search should start from
				// the record next to the search index record)

				// block = search index's block
				// slot = search index's slot + 1
				block=prevRecId.block;
				slot=prevRecId.slot+1;
		}

		/* The following code searches for the next record in the relation
			 that satisfies the given condition
			 We start from the record id (block, slot) and iterate over the remaining
			 records of the relation
		*/
		while (block != -1)
		{
				/* create a RecBuffer object for block (use RecBuffer Constructor for
					 existing block) */
				RecBuffer relBuffer(block);
				HeadInfo relHeader;
					
				// get the record with id (block, slot) using RecBuffer::getRecord()
				// get header of the block using RecBuffer::getHeader() function
				// get slot map of the block using RecBuffer::getSlotMap() function
				relBuffer.getHeader(&relHeader);
				
				unsigned char slotmap[relHeader.numSlots];
				Attribute attr[relHeader.numAttrs];
				
				relBuffer.getSlotMap(slotmap);
				

				// If slot >= the number of slots per block(i.e. no more slots in this block)
				if(slot>=relHeader.numSlots)
				{
						// update block = right block of block
						// update slot = 0
						block=relHeader.rblock;
						slot=0;
						
						continue;  // continue to the beginning of this while loop
				}

				// if slot is free skip the loop
				// (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
				if(slotmap[slot]==SLOT_UNOCCUPIED)
				{
						// increment slot and continue to the next record slot
						slot++;
						continue;
				}
				relBuffer.getRecord(attr,slot);

				// compare record's attribute value to the the given attrVal as below:
				/*
						firstly get the attribute offset for the attrName attribute
						from the attribute cache entry of the relation using
						AttrCacheTable::getAttrCatEntry()
				*/
				AttrCatEntry attrCatBuf;
				AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuf);
				/* use the attribute offset to get the value of the attribute from
					 current record */
				
				Attribute temp=attr[attrCatBuf.offset];

				int cmpVal;  // will store the difference between the attributes
				// set cmpVal using compareAttrs()
				cmpVal=compareAttrs(temp,attrVal,attrCatBuf.attrType);

				/* Next task is to check whether this record satisfies the given condition.
					 It is determined based on the output of previous comparison and
					 the op value received.
					 The following code sets the cond variable if the condition is satisfied.
				*/
				if (
						(op == NE && cmpVal != 0) ||    // if op is "not equal to"
						(op == LT && cmpVal < 0) ||     // if op is "less than"
						(op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
						(op == EQ && cmpVal == 0) ||    // if op is "equal to"
						(op == GT && cmpVal > 0) ||     // if op is "greater than"
						(op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
				) {
						/*
						set the search index in the relation cache as
						the record id of the record that satisfies the given condition
						(use RelCacheTable::setSearchIndex function)
						*/
						RecId temp0;
						temp0.block=block;
						temp0.slot=slot;
						RelCacheTable::setSearchIndex(relId,&temp0);

						return RecId{block, slot};
				}

				slot++;
		}

		// no record in the relation with Id relid satisfies the given condition
		return RecId{-1, -1};
}
int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{	
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
       
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal,newName);

    // search the relation catalog for an entry with "RelName" = newRelationName
    
    RecId searchRes = linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, newRelationName, EQ);

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    
    if(searchRes.block!=-1 || searchRes.slot!=-1)
    return E_RELEXIST;


    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal,oldName);

    // search the relation catalog for an entry with "RelName" = 
    searchRes = linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, oldRelationName, EQ);

    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    if(searchRes.block==-1 || searchRes.slot==-1)
    return E_RELNOTEXIST;

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord
    
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute record[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(record,searchRes.slot);
    
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal,newRelationName.sVal);
    relCatBlock.setRecord(record,searchRes.slot);
    
    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
       
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord
    
    while(1)
    {
		
		searchRes=linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);
		
		if(searchRes.block==-1 || searchRes.slot==-1)
		break;
		
		RecBuffer attrCatBlock(searchRes.block);
		Attribute record[ATTRCAT_NO_ATTRS];
		attrCatBlock.getRecord(record,searchRes.slot);
		strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal,newRelationName.sVal);
		attrCatBlock.setRecord(record,searchRes.slot);
		
	}

    return SUCCESS;
}
int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) 
{

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
       
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal,relName);

    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    RecId searchRes = linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    
    if(searchRes.block==-1 || searchRes.slot==-1)
    return E_RELNOTEXIST;

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (1) 
    {
        // linear search on the attribute catalog for RelName = relNameAttr
        searchRes=linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        
        if(searchRes.block==-1 || searchRes.slot==-1)
        break;
        

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
          
        RecBuffer attrCatBlock(searchRes.block);
		attrCatBlock.getRecord(attrCatEntryRecord,searchRes.slot);

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)==0)
        {
			return E_ATTREXIST;
		}
		
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName)==0)
        {
			attrToRenameRecId.block=searchRes.block;
			attrToRenameRecId.slot=searchRes.slot;
		}

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
        
        
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    
    if(attrToRenameRecId.block==-1 && attrToRenameRecId.slot==-1)
    return E_ATTRNOTEXIST;


    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord
    
    
    RecBuffer attrCatBlock(attrToRenameRecId.block);
	Attribute record[ATTRCAT_NO_ATTRS];
	attrCatBlock.getRecord(record,attrToRenameRecId.slot);
	strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
	attrCatBlock.setRecord(record,attrToRenameRecId.slot);
    
    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) 
{
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
    
    RelCatEntry relCatEntry;
    
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);

    int blockNum = relCatEntry.firstBlk;/* first record block of the relation (from the rel-cat entry)*/;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;/* number of slots per record block */;
    int numOfAttributes = relCatEntry.numAttrs;/* number of attributes of the relation */;

    int prevBlockNum =-1; /* block number of the last element in the linked list = -1 */;

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */
    while (blockNum != -1) 
    {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        
        RecBuffer recBuffer(blockNum);

        // get header of block(blockNum) using RecBuffer::getHeader() function
        
        struct HeadInfo head;
        recBuffer.getHeader(&head);

        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        unsigned char slot[numOfSlots];
        
        recBuffer.getSlotMap(slot);

        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */
           
        int i=0;
           
        for(i=0;i<numOfSlots;i++)
        {
			if(slot[i]==SLOT_UNOCCUPIED)
			break;
			
		}

        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked
                                               list of record blocks)
        */
        
        if(i<numOfSlots)
        {
			rec_id.block=blockNum;
			rec_id.slot=i;
			break;
		}
		else
		{
			prevBlockNum=blockNum;
			blockNum=head.rblock;
			//std::cout << "Prev block: " << prevBlockNum;
		}
		
				
			
    }

    if(rec_id.block==-1 && rec_id.slot==-1)//  if no free slot is found in existing record blocks (rec_id = {-1, -1})
    {
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
        
        if(strcmp(relCatEntry.relName,RELCAT_RELNAME)==0)
        return E_MAXRELATIONS;

        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
        
        RecBuffer recBuffer;
        int ret=recBuffer.getBlockNum();
        
        
        if (ret == E_DISKFULL) 
        {
            return E_DISKFULL;
        }

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        rec_id.block=ret;
        rec_id.slot = 0;

        /*
            set the header of the new record block such that it links with
            existing record blocks of the relation
            set the block's header as follows:
            blockType: REC, pblock: -1
            lblock
                  = -1 (if linked list of existing record blocks was empty
                         i.e this is the first insertion into the relation)
                  = prevBlockNum (otherwise),
            rblock: -1, numEntries: 0,
            numSlots: numOfSlots, numAttrs: numOfAttributes
            (use BlockBuffer::setHeader() function)
        */
        
        struct HeadInfo head;
        head.blockType=REC;
        head.pblock=-1;
        head.lblock=prevBlockNum;
        head.rblock=-1;
        head.numEntries=0;
        head.numSlots=numOfSlots;
        head.numAttrs=numOfAttributes;
        
        recBuffer.setHeader(&head);
        

        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
        
        unsigned char slot[numOfSlots];
        
        for(int i=0;i<numOfSlots;i++)
        slot[i]=SLOT_UNOCCUPIED;
        
        recBuffer.setSlotMap(slot);

        if(prevBlockNum != -1)// if prevBlockNum != -1
        {
            // create a RecBuffer object for prevBlockNum
            // get the header of the block prevBlockNum and
            // update the rblock field of the header to the new block
            // number i.e. rec_id.block
            // (use BlockBuffer::setHeader() function)
            
            RecBuffer temp(prevBlockNum);
            struct HeadInfo temp1;
            temp.getHeader(&temp1);
            temp1.rblock=rec_id.block;
            temp.setHeader(&temp1);
        }
        else// else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
            
            relCatEntry.firstBlk=rec_id.block;
            
            RelCacheTable::setRelCatEntry(relId,&relCatEntry);
        }
        
        

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
        
        relCatEntry.lastBlk=rec_id.block;
        RelCacheTable::setRelCatEntry(relId,&relCatEntry);
    }

    // create a RecBuffer object for rec_id.block
    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    
    RecBuffer recBuffer(rec_id.block);
    recBuffer.setRecord(record,rec_id.slot);

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
    
    unsigned char slot[numOfSlots];
    recBuffer.getSlotMap(slot);
    slot[rec_id.slot]=SLOT_OCCUPIED;
    recBuffer.setSlotMap(slot);

    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    
    struct HeadInfo head;
    recBuffer.getHeader(&head);
    head.numEntries=head.numEntries+1;
    recBuffer.setHeader(&head);
    

    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    
    relCatEntry.numRecs=relCatEntry.numRecs+1;
    RelCacheTable::setRelCatEntry(relId,&relCatEntry);
    

    return SUCCESS;
}
/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) 
{
    // Declare a variable called recid to store the searched record
    RecId recId;

    /* search for the record id (recid) corresponding to the attribute with
    attribute name attrName, with value attrval and satisfying the condition op
    using linearSearch() */
    
    recId=linearSearch(relId,attrName,attrVal,op);

    // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;

    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */
    
    if(recId.block==-1 || recId.slot==-1)
    return E_NOTFOUND;
    
    RecBuffer recBuffer(recId.block);
    
    recBuffer.getRecord(record,recId.slot);
    

    return SUCCESS;
}
int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) 
{
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
        
    if (strcmp(RELCAT_RELNAME,relName)==0 || strcmp(ATTRCAT_RELNAME,relName)==0)
    return E_NOTPERMITTED;

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
       
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
    strcpy(relNameAttr.sVal,relName);

    //  linearSearch on the relation catalog for RelName = relNameAttr
    RecId searchRes=linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    

    // if the relation does not exist (linearSearch returned {-1, -1})
    //     return E_RELNOTEXIST
    if(searchRes.block==-1 || searchRes.slot==-1)
    return E_RELNOTEXIST;

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
    RecBuffer reCBuffer(searchRes.block);
    reCBuffer.getRecord(relCatEntryRecord, searchRes.slot);

    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */
    int firstBlk=relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs=relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    /*
     Delete all the record blocks of the relation
    */
    // for each record block of the relation:
    //     get block header using BlockBuffer.getHeader
    //     get the next block from the header (rblock)
    //     release the block using BlockBuffer.releaseBlock
    //
    //     Hint: to know if we reached the end, check if nextBlock = -1
    
    while(firstBlk!=-1)
    {
		RecBuffer blockBuffer(firstBlk);
		struct HeadInfo head;
		blockBuffer.getHeader(&head);
		int nextBlock=head.rblock;
		blockBuffer.releaseBlock();
		
		if(nextBlock==-1)
		break;
		else
		firstBlk=nextBlock;
	}
		


    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/

    // reset the searchIndex of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    while(true) 
    {
        RecId attrCatRecId;
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr
        attrCatRecId=linearSearch(ATTRCAT_RELID,(char*)ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);

        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;
        if(attrCatRecId.block==-1 && attrCatRecId.slot==-1)
        break;
        
        numberOfAttributesDeleted++;

        // create a RecBuffer for attrCatRecId.block
        // get the header of the block
        // get the record corresponding to attrCatRecId.slot
        RecBuffer recBuffer(attrCatRecId.block);
        struct HeadInfo head;
        recBuffer.getHeader(&head);
        Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
        recBuffer.getRecord(attrCatEntryRecord,attrCatRecId.slot);

        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
        int rootBlock = attrCatEntryRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;/* get root block from the record */;
        // (This will be used later to delete any indexes if it exists)

        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
        // Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap
        unsigned char slotMap[SLOTMAP_SIZE_RELCAT_ATTRCAT];
        recBuffer.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot]=SLOT_UNOCCUPIED;
        recBuffer.setSlotMap(slotMap);
        

        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header
           using RecBuffer.setHeader */
        head.numEntries-=1;
        recBuffer.setHeader(&head);

        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list.
        */
        if (head.numEntries==0)/*   header.numEntries == 0  */
        {
            /* Standard Linked List Delete for a Block
               Get the header of the left block and set it's rblock to this
               block's rblock
            */
            struct HeadInfo head_left;
            RecBuffer recBuffer_left(head.lblock);
            recBuffer_left.getHeader(&head_left);
            head_left.rblock=head.rblock;
            recBuffer_left.setHeader(&head_left);
            

            // create a RecBuffer for lblock and call appropriate methods

            if (head.rblock!=-1) /* header.rblock != -1 */
            {
				struct HeadInfo head_right;
				RecBuffer recBuffer_right(head.rblock);
				recBuffer_right.getHeader(&head_right);
				head_right.lblock=head.lblock;
				recBuffer_right.setHeader(&head_right);
				
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
                // create a RecBuffer for rblock and call appropriate methods

            } 
            else 
            {
                // (the block being released is the "Last Block" of the relation.)
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
                   
                RelCatEntry relCatBuf;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatBuf);
                relCatBuf.lastBlk=head.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID,&relCatBuf);
                
            }

            // (Since the attribute catalog will never be empty(why?), we do not
            //  need to handle the case of the linked list becoming empty - i.e
            //  every block of the attribute catalog gets released.)

            // call releaseBlock()
            recBuffer.releaseBlock();
            
        }

        // (the following part is only relevant once indexing has been implemented)
        // if index exists for the attribute (rootBlock != -1), call bplus destroy
        if (rootBlock != -1) {
            // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
        }
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    // Fetch the header of Relcat block
    RecBuffer recbuffer(searchRes.block);
    struct HeadInfo header;
    recbuffer.getHeader(&header);

    /* Decrement the numEntries in the header of the block corresponding to the
       relation catalog entry and set it back */
    header.numEntries-=1;
    recbuffer.setHeader(&header);   

    /* Get the slotmap in relation catalog, update it by marking the slot as
       free(SLOT_UNOCCUPIED) and set it back. */
    unsigned char slotMap[SLOTMAP_SIZE_RELCAT_ATTRCAT];
    recbuffer.getSlotMap(slotMap);
    slotMap[searchRes.slot]=SLOT_UNOCCUPIED;
    recbuffer.setSlotMap(slotMap);

    /*** Updating the Relation Cache Table ***/
    /** Update relation catalog record entry (number of records in relation
        catalog is decreased by 1) **/
    // Get the entry corresponding to relation catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(RELCAT_RELID,&relCatBuf);
    relCatBuf.numRecs-=1;
    RelCacheTable::setRelCatEntry(RELCAT_RELID,&relCatBuf);
    

    /** Update attribute catalog entry (number of records in attribute catalog
        is decreased by numberOfAttributesDeleted) **/
    // i.e., #Records = #Records - numberOfAttributesDeleted

    // Get the entry corresponding to attribute catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatBuf);
    relCatBuf.numRecs-=numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID,&relCatBuf);

    return SUCCESS;
}
