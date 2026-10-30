#include "Algebra.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
bool isNumber(char *str) 
{
	int len;
	float ignore;
	/*
		sscanf returns the number of elements read, so if there is no float matching
		the first %f, ret will be 0, else it'll be 1

		%n gets the number of characters read. this scanf sequence will read the
		first float ignoring all the whitespace before and after. and the number of
		characters read that far will be stored in len. if len == strlen(str), then
		the string only contains a float with/without whitespace. else, there's other
		characters.
	*/
	int ret = sscanf(str, "%f %n", &ignore, &len);
	return ret == 1 && len == strlen(str);
}
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) 
{
    // get the srcRel's rel-id (let it be srcRelid), using OpenRelTable::getRelId()
    // if srcRel is not open in open relation table, return E_RELNOTOPEN
    int srcRelId = OpenRelTable::getRelId(srcRel);
    
    if (srcRelId == E_RELNOTOPEN) 
	{
		return E_RELNOTOPEN;
	}
	
    // get the attr-cat entry for attr, using AttrCacheTable::getAttrCatEntry()
    // if getAttrcatEntry() call fails return E_ATTRNOTEXIST
    AttrCatEntry attrCatEntry;
    int result=AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
	
	if(result==E_ATTRNOTEXIST)
	return result;


    /*** Convert strVal to an attribute of data type NUMBER or STRING ***/

    Attribute attrVal;
    int type = attrCatEntry.attrType;

    if (type == NUMBER)
    {
        // if the input argument strVal can be converted to a number
        if (isNumber(strVal)) // (check this using isNumber() function)
        {
            attrVal.nVal = atof(strVal);// convert strVal to double and store it at attrVal.nVal using atof()
        }
        else// else
        {
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if (type == STRING)
    {
        strcpy(attrVal.sVal, strVal);// copy strVal to attrVal.sVal
    }

    /*** Creating and opening the target relation ***/
    // Prepare arguments for createRel() in the following way:
    // get RelcatEntry of srcRel using RelCacheTable::getRelCatEntry()
    
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatBuf);
    int src_nAttrs =relCatBuf.numAttrs; /* the no. of attributes present in src relation */


    /* let attr_names[src_nAttrs][ATTR_SIZE] be a 2D array of type char
        (will store the attribute names of rel). */
    // let attr_types[src_nAttrs] be an array of type int
    
    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    /*iterate through 0 to src_nAttrs-1 :
        get the i'th attribute's AttrCatEntry using AttrCacheTable::getAttrCatEntry()
        fill the attr_names, attr_types arrays that we declared with the entries
        of corresponding attributes
    */
    
    for(int i=0;i<src_nAttrs;i++)
    {
		AttrCatEntry AttrCatBuf;
		AttrCacheTable::getAttrCatEntry(srcRelId,i,&AttrCatBuf);
		attr_types[i]=AttrCatBuf.attrType;
		strcpy(attr_names[i],AttrCatBuf.attrName);
	}	


    /* Create the relation for target relation by calling Schema::createRel()
       by providing appropriate arguments */
    // if the createRel returns an error code, then return that value.
    
    int ret=Schema::createRel(targetRel,src_nAttrs,attr_names,attr_types);
    
    if(ret<0)
    return ret;

    /* Open the newly created target relation by calling OpenRelTable::openRel()
       method and store the target relid */
    /* If opening fails, delete the target relation by calling Schema::deleteRel()
       and return the error value returned from openRel() */
					
    int targetRelId=OpenRelTable::openRel(targetRel);
			
    if(targetRelId<0)
    {
		Schema::deleteRel(targetRel);
		return targetRelId;
	}

    /*** Selecting and inserting records into the target relation ***/
    /* Before calling the search function, reset the search to start from the
       first using RelCacheTable::resetSearchIndex() */

    Attribute record[src_nAttrs];

    /*
        The BlockAccess::search() function can either do a linearSearch or
        a B+ tree search. Hence, reset the search index of the relation in the
        relation cache using RelCacheTable::resetSearchIndex().
        Also, reset the search index in the attribute cache for the select
        condition attribute with name given by the argument `attr`. Use
        AttrCacheTable::resetSearchIndex().
        Both these calls are necessary to ensure that search begins from the
        first record.
    */
    
    RelCacheTable::resetSearchIndex(srcRelId);/* fill arguments */
    AttrCacheTable::resetSearchIndex(srcRelId,attr);/* fill arguments */

    // read every record that satisfies the condition by repeatedly calling
    // BlockAccess::search() until there are no more records to be read

    while (BlockAccess::search(srcRelId,record,attr,attrVal,op)==SUCCESS) /* BlockAccess::search() returns success */
    {

        // ret = BlockAccess::insert(targetRelId, record);
        ret = BlockAccess::insert(targetRelId, record);

        // if (insert fails) {
        //     close the targetrel(by calling Schema::closeRel(targetrel))
        //     delete targetrel (by calling Schema::deleteRel(targetrel))
        //     return ret;
        // }
        
        if(ret<0)
        {
			Schema::closeRel(targetRel);
			Schema::deleteRel(targetRel);
			return ret;
		}
			
			
    }

    // Close the targetRel by calling closeRel() method of schema layer
    Schema::closeRel(targetRel);
    
    // return SUCCESS.
    return SUCCESS;
}

// will return if a string can be parsed as a floating point 

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE])
{
    // if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
    // return E_NOTPERMITTED;
    
    if(strcmp(relName,"RELATIONCAT")==0 || strcmp(relName,"ATTRIBUTECAT")==0)
    return E_NOTPERMITTED;

    // get the relation's rel-id using OpenRelTable::getRelId() method
    int relId = OpenRelTable::getRelId(relName);

    // if relation is not open in open relation table, return E_RELNOTOPEN
    // (check if the value returned from getRelId function call = E_RELNOTOPEN)
    // get the relation catalog entry from relation cache
    // (use RelCacheTable::getRelCatEntry() of Cache Layer)
    
    if(relId==E_RELNOTOPEN)
    return E_RELNOTOPEN;
    
    RelCatEntry relCatEntry;
    
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    

    /* if relCatEntry.numAttrs != numberOfAttributes in relation,
       return E_NATTRMISMATCH */
       
    if(relCatEntry.numAttrs != nAttrs)
    return E_NATTRMISMATCH;

    // let recordValues[numberOfAttributes] be an array of type union Attribute
    
    union Attribute recordValues[nAttrs];

    /*
        Converting 2D char array of record values to Attribute array recordValues
     */
    for(int i=0;i<nAttrs;i++)// iterate through 0 to nAttrs-1: (let i be the iterator)
    {
        // get the attr-cat entry for the i'th attribute from the attr-cache
        // (use AttrCacheTable::getAttrCatEntry())
        
        AttrCatEntry attrCatEntry;
        
        AttrCacheTable::getAttrCatEntry(relId,i,&attrCatEntry);

        // let type = attrCatEntry.attrType
        int type=attrCatEntry.attrType;

        if (type == NUMBER)
        {
            // if the char array record[i] can be converted to a number
            if(isNumber(record[i]))// (check this using isNumber() function)
            {
                /* convert the char array to numeral and store it
                   at recordValues[i].nVal using atof() */
                   recordValues[i].nVal=atof(record[i]);
                   
            }
            else// else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            // copy record[i] to recordValues[i].sVal
            strcpy(recordValues[i].sVal,record[i]);
        }
    }

    // insert the record by calling BlockAccess::insert() function
    // let retVal denote the return value of insert call
    int retVal=BlockAccess::insert(relId,recordValues);

    return retVal;
}
int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) 
{

    int srcRelId =OpenRelTable::getRelId(srcRel); /*srcRel's rel-id (use OpenRelTable::getRelId() function)*/

    // if srcRel is not open in open relation table, return E_RELNOTOPEN
    if (srcRelId == E_RELNOTOPEN) 
	{
		return E_RELNOTOPEN;
	}

    // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatBuf);
    

    // get the no. of attributes present in relation from the fetched RelCatEntry.
    int numAttrs=relCatBuf.numAttrs;

    // attrNames and attrTypes will be used to store the attribute names
    // and types of the source relation respectively
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];

    /*iterate through every attribute of the source relation :
        - get the AttributeCat entry of the attribute with offset.
          (using AttrCacheTable::getAttrCatEntry())
        - fill the arrays `attrNames` and `attrTypes` that we declared earlier
          with the data about each attribute
    */
    
    for(int i=0;i<numAttrs;i++)
    {
		AttrCatEntry AttrCatBuf;
		AttrCacheTable::getAttrCatEntry(srcRelId,i,&AttrCatBuf);
		attrTypes[i]=AttrCatBuf.attrType;
		strcpy(attrNames[i],AttrCatBuf.attrName);
	}


    /*** Creating and opening the target relation ***/

    // Create a relation for target relation by calling Schema::createRel()
    int ret=Schema::createRel(targetRel,numAttrs,attrNames,attrTypes);

    // if the createRel returns an error code, then return that value.
    if(ret<0)
    return ret;

    // Open the newly created target relation by calling OpenRelTable::openRel()
    // and get the target relid
    int targetRelId=OpenRelTable::openRel(targetRel);

    // If opening fails, delete the target relation by calling Schema::deleteRel() of
    // return the error value returned from openRel().
    if(targetRelId<0)
    {
		Schema::deleteRel(targetRel);
		return targetRelId;
	}


    /*** Inserting projected records into the target relation ***/

    // Take care to reset the searchIndex before calling the project function
    // using RelCacheTable::resetSearchIndex()
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[numAttrs];


    while (BlockAccess::project(srcRelId, record)==SUCCESS)/* BlockAccess::project(srcRelId, record) returns SUCCESS */
    {
        // record will contain the next record

        // ret = BlockAccess::insert(targetRelId, proj_record);
        ret=BlockAccess::insert(targetRelId,record);

        if (ret<0) /* insert fails */
        {
            // close the targetrel by calling Schema::closeRel()
            // delete targetrel by calling Schema::deleteRel()
            // return ret;
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    // Close the targetRel by calling Schema::closeRel()
    Schema::closeRel(targetRel);

    // return SUCCESS.
    return SUCCESS;
}
int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) 
{

    int srcRelId = OpenRelTable::getRelId(srcRel);/*srcRel's rel-id (use OpenRelTable::getRelId() function)*/

    // if srcRel is not open in open relation table, return E_RELNOTOPEN
    if (srcRelId == E_RELNOTOPEN) 
	{
		return E_RELNOTOPEN;
	}

    // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatBuf);

    // get the no. of attributes present in relation from the fetched RelCatEntry.
    int numAttrs=relCatBuf.numAttrs;

    // declare attr_offset[tar_nAttrs] an array of type int.
    // where i-th entry will store the offset in a record of srcRel for the
    // i-th attribute in the target relation.
    int attr_offset[tar_nAttrs];

    // let attr_types[tar_nAttrs] be an array of type int.
    // where i-th entry will store the type of the i-th attribute in the
    // target relation.
    int attr_types[tar_nAttrs];


    /*** Checking if attributes of target are present in the source relation
         and storing its offsets and types ***/

    /*iterate through 0 to tar_nAttrs-1 :
        - get the attribute catalog entry of the attribute with name tar_attrs[i].
        - if the attribute is not found return E_ATTRNOTEXIST
        - fill the attr_offset, attr_types arrays of target relation from the
          corresponding attribute catalog entries of source relation
    */
    for(int i=0;i<tar_nAttrs;i++)
    {
		AttrCatEntry AttrCatBuf;
		int ret=AttrCacheTable::getAttrCatEntry(srcRelId,tar_Attrs[i],&AttrCatBuf);
		
		if(ret<0)
		return E_ATTRNOTEXIST;
		
		attr_offset[i]=AttrCatBuf.offset;
		attr_types[i]=AttrCatBuf.attrType;		
	}


    /*** Creating and opening the target relation ***/

    // Create a relation for target relation by calling Schema::createRel()
    int ret=Schema::createRel(targetRel,tar_nAttrs,tar_Attrs,attr_types);

    // if the createRel returns an error code, then return that value.
    if(ret<0)
    return ret;

    // Open the newly created target relation by calling OpenRelTable::openRel()
    // and get the target relid
    int targetRelId=OpenRelTable::openRel(targetRel);
    

    // If opening fails, delete the target relation by calling Schema::deleteRel()
    // and return the error value from openRel()
    if(targetRelId<0)
    {
		Schema::deleteRel(targetRel);
		return targetRelId;
	}


    /*** Inserting projected records into the target relation ***/

    // Take care to reset the searchIndex before calling the project function
    // using RelCacheTable::resetSearchIndex()
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[numAttrs];

    while (BlockAccess::project(srcRelId, record)==SUCCESS) /* BlockAccess::project(srcRelId, record) returns SUCCESS */
    {
        // the variable `record` will contain the next record

        Attribute proj_record[tar_nAttrs];

        //iterate through 0 to tar_attrs-1:
        //    proj_record[attr_iter] = record[attr_offset[attr_iter]]
        
        for(int i=0;i<tar_nAttrs;i++)
        proj_record[i]=record[attr_offset[i]];

        // ret = BlockAccess::insert(targetRelId, proj_record);
        ret=BlockAccess::insert(targetRelId,proj_record);

        if (ret<0) /* insert fails */
        {
            // close the targetrel by calling Schema::closeRel()
            // delete targetrel by calling Schema::deleteRel()
            // return ret;
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    // Close the targetRel by calling Schema::closeRel()
    Schema::closeRel(targetRel);

    // return SUCCESS.
    return SUCCESS;
}
int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE]) 
{

    // get the srcRelation1's rel-id using OpenRelTable::getRelId() method
    int srcrelId_1=OpenRelTable::getRelId(srcRelation1);

    // get the srcRelation2's rel-id using OpenRelTable::getRelId() method
    int srcrelId_2=OpenRelTable::getRelId(srcRelation2);

    // if either of the two source relations is not open
    //     return E_RELNOTOPEN
    
    if(srcrelId_1==E_RELNOTOPEN || srcrelId_2==E_RELNOTOPEN)
    return E_RELNOTOPEN;

    AttrCatEntry attrCatEntry1, attrCatEntry2;
    // get the attribute catalog entries for the following from the attribute cache
    // (using AttrCacheTable::getAttrCatEntry())
    // - attrCatEntry1 = attribute1 of srcRelation1
    // - attrCatEntry2 = attribute2 of srcRelation2
    
    int ret=AttrCacheTable::getAttrCatEntry(srcrelId_1,attribute1,&attrCatEntry1);
    
    if(ret==E_ATTRNOTEXIST)
    return E_ATTRNOTEXIST;
    
    ret=AttrCacheTable::getAttrCatEntry(srcrelId_2,attribute2,&attrCatEntry2);
    
    if(ret==E_ATTRNOTEXIST)
    return E_ATTRNOTEXIST;

    // if attribute1 is not present in srcRelation1 or attribute2 is not
    // present in srcRelation2 (getAttrCatEntry() returned E_ATTRNOTEXIST)
    //     return E_ATTRNOTEXIST.

    // if attribute1 and attribute2 are of different types return E_ATTRTYPEMISMATCH
    
    if(attrCatEntry1.attrType!=attrCatEntry2.attrType)
    return E_ATTRTYPEMISMATCH;
    
    RelCatEntry relcat1,relcat2;
    
    RelCacheTable::getRelCatEntry(srcrelId_1,&relcat1);
    RelCacheTable::getRelCatEntry(srcrelId_2,&relcat2);
    

    // iterate through all the attributes in both the source relations and check if
    // there are any other pair of attributes other than join attributes
    // (i.e. attribute1 and attribute2) with duplicate names in srcRelation1 and
    // srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    // If yes, return E_DUPLICATEATTR
    
    for(int i=0;i<relcat1.numAttrs;i++)
    {
		if(attrCatEntry1.offset==i)
		continue;
		
		AttrCatEntry temp1;
		AttrCacheTable::getAttrCatEntry(srcrelId_1,i,&temp1);
		
		for(int j=0;j<relcat2.numAttrs;j++)
		{
			if(attrCatEntry2.offset==j)
			continue;
			else
			{
				AttrCatEntry temp2;
				AttrCacheTable::getAttrCatEntry(srcrelId_2,j,&temp2);
				
				if(strcmp(temp1.attrName,temp2.attrName)==0)
				return E_DUPLICATEATTR;
			}
		}
	}

    // get the relation catalog entries for the relations from the relation cache
    // (use RelCacheTable::getRelCatEntry() function)

    int numOfAttributes1 = relcat1.numAttrs;/* number of attributes in srcRelation1 */
    int numOfAttributes2 = relcat2.numAttrs;/* number of attributes in srcRelation2 */

    // if rel2 does not have an index on attr2
    //     create it using BPlusTree:bPlusCreate()
    //     if call fails, return the appropriate error code
    //     (if your implementation is correct, the only error code that will
    //      be returned here is E_DISKFULL)
    
    if(attrCatEntry2.rootBlock==-1)
    {
		ret=BPlusTree::bPlusCreate(srcrelId_2,attribute2);
		
		if(ret!=SUCCESS)
		return ret;
	}

    int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;
    // Note: The target relation has number of attributes one less than
    // nAttrs1+nAttrs2 (Why?)

    // declare the following arrays to store the details of the target relation
    char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numOfAttributesInTarget];

    // iterate through all the attributes in both the source relations and
    // update targetRelAttrNames[],targetRelAttrTypes[] arrays excluding attribute2
    // in srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    
    
    
    for(int i=0;i<numOfAttributes1;i++)
    {
		AttrCatEntry temp1;
		AttrCacheTable::getAttrCatEntry(srcrelId_1,i,&temp1);
		
		strcpy(targetRelAttrNames[i],temp1.attrName);
		targetRelAttrTypes[i]=temp1.attrType;
	}
	
	int idx=numOfAttributes1;
	
	for(int i=0;i<numOfAttributes2;i++)
	{
		if(i==attrCatEntry2.offset)
		continue;
		
		AttrCatEntry temp2;
		AttrCacheTable::getAttrCatEntry(srcrelId_2,i,&temp2);
		
		strcpy(targetRelAttrNames[idx],temp2.attrName);
		targetRelAttrTypes[idx]=temp2.attrType;
		idx++;
	}
		
		

    // create the target relation using the Schema::createRel() function
    
    ret=Schema::createRel(targetRelation,numOfAttributesInTarget,targetRelAttrNames,targetRelAttrTypes);

    // if createRel() returns an error, return that error
    
    if(ret!=SUCCESS)
    return ret;

    // Open the targetRelation using OpenRelTable::openRel()
    int targetRelId=OpenRelTable::openRel(targetRelation);

    if(targetRelId<0)// if openRel() fails (No free entries left in the Open Relation Table)
    {
		Schema::deleteRel(targetRelation);
		return targetRelId;
        // delete target relation by calling Schema::deleteRel()
        // return the error code
    }

    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[numOfAttributesInTarget];
    
    RelCacheTable::resetSearchIndex(srcrelId_1);

    // this loop is to get every record of the srcRelation1 one by one
    while (BlockAccess::project(srcrelId_1, record1) == SUCCESS) 
    {

        // reset the search index of `srcRelation2` in the relation cache
        // using RelCacheTable::resetSearchIndex()
        
        RelCacheTable::resetSearchIndex(srcrelId_2);

        // reset the search index of `attribute2` in the attribute cache
        // using AttrCacheTable::resetSearchIndex()
        
        AttrCacheTable::resetSearchIndex(srcrelId_2,attribute2);

        // this loop is to get every record of the srcRelation2 which satisfies
        //the following condition:
        // record1.attribute1 = record2.attribute2 (i.e. Equi-Join condition)
        while (BlockAccess::search(srcrelId_2, record2, attribute2, record1[attrCatEntry1.offset], EQ) == SUCCESS) 
        {

            // copy srcRelation1's and srcRelation2's attribute values(except
            // for attribute2 in rel2) from record1 and record2 to targetRecord
            
            for(int i=0;i<numOfAttributes1;i++)
			targetRecord[i]=record1[i];
			
			int idx=numOfAttributes1;
			
			for(int i=0;i<numOfAttributes2;i++)
			{
				if(i==attrCatEntry2.offset)
				continue;
				
				targetRecord[idx]=record2[i];
				
				idx++;
			}

            // insert the current record into the target relation by calling
            // BlockAccess::insert()
            
            ret=BlockAccess::insert(targetRelId,targetRecord);

            if(ret<0) /* insert fails (insert should fail only due to DISK being FULL) */
            {

                // close the target relation by calling OpenRelTable::closeRel()
                // delete targetRelation (by calling Schema::deleteRel())
                
                OpenRelTable::closeRel(targetRelId);
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
    }

    // close the target relation by calling OpenRelTable::closeRel()
    OpenRelTable::closeRel(targetRelId);
    return SUCCESS;
}
