#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "queueDS.h"
#include "sharedFunctions.h"
#include "sharedEnums.h"


enum NumOfOPCodes {NUM_OF_OPCODES = 16};


struct HashTable {

    struct Queue *table[HASH_TABLE_SIZE];
    unsigned int elementCount;
};

struct HashTable *hashTableAlloc() {
    return (struct HashTable *)malloc(sizeof(struct HashTable));
}

void initDefaultTable(struct HashTable *newHashTable) {

    int i;

    for(i = 0 ; i < HASH_TABLE_SIZE ; ++i)
        newHashTable->table[i] = NULL;

    return;
}

struct HashTable *createHashTable() {

    struct HashTable *newHashTable;

    newHashTable = hashTableAlloc();
    if(newHashTable == NULL) {
        printf("\nERROR @ createHashTable - cannot allocate memory for a new hash table\n");
        return NULL;
    }else {
        initDefaultTable(newHashTable);
        newHashTable->elementCount = 0;
        return newHashTable;
    }
}

unsigned int hashFunctionV2(const char label[]) { /*based on FNV hash, almost the same as the one on queueDS.c file , the parameter is a general string and instead of a queue node*/
                                                  /*https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function*/
    int i;
    unsigned long FNV_offset_basis;
    unsigned long FNV_prime;
    unsigned long hash;

    FNV_offset_basis = 2166136261u;
    FNV_prime = 16777619u;
    hash = FNV_offset_basis;

    for(i = 0; label[i] != '\0' && label[i] != ':' ; i++)
    {
        hash ^= label[i];
        hash *= FNV_prime;
    }

    return hash % HASH_TABLE_SIZE;
}

/*we'll use this program just to print an error , we will still add it to the table if unique*/
int isUniqueLabel(struct Queue *hashTableCell, unsigned long authKey, long currLineNumber) {

    const struct Qnode *probe;

    probe = getHeadNode(hashTableCell);

    while(probe != NULL) {

        if(getAuthKey(probe) == authKey) {
            printf("\nERROR @ line %ld - multiple definitions in the same file with identical label : %s  \n", currLineNumber, getNodeLabel(probe));
            return FALSE;
        }else
            probe = getNextNode(probe);
    }

    return TRUE;
}

int hashAddElementResKeyword(unsigned int index, struct HashTable *hashTable, const char label[], unsigned long authKey, va_list args) {

    unsigned int operands;
    int opCode;

    operands = va_arg(args,unsigned int);
    opCode = va_arg(args,int);

    return enqueueV1_1(RESERVED_KEYWORD_DATA_ARGC, KEYWORD_DATATYPE, hashTable->table[index], label, authKey, operands, opCode);
}

int hashAddElementSymbol(unsigned int index, struct HashTable *hashTable, const char label[], unsigned long authKey, va_list args) {

    unsigned int deciAddress; /*decimal address*/
    int binaryCode; /*binary machine code*/
    unsigned int statement;

    deciAddress = va_arg(args,unsigned int);
    binaryCode = va_arg(args,int);
    statement = va_arg(args,unsigned int);

    return enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, hashTable->table[index], label, authKey, deciAddress, binaryCode, statement);
}

int hashAddElementMacro(unsigned int index, struct HashTable *hashTable, const char label[], unsigned long authKey, va_list args) {

    int startingPos;
    int endingPos;

    startingPos = va_arg(args,unsigned int);
    endingPos = va_arg(args,int);

    return enqueueV1_1(RESERVED_KEYWORD_DATA_ARGC, KEYWORD_DATATYPE, hashTable->table[index], label, authKey, startingPos, endingPos);
}

int hashAddElement(int nodeDataType, unsigned int index, struct HashTable *hashTable, const char label[], unsigned long authKey, va_list args) {

    int result;

    switch(nodeDataType) {
        case KEYWORD_DATATYPE:
            result = hashAddElementResKeyword(index, hashTable, label, authKey, args);
            break;
        case SYMBOL_DATATYPE:
            result = hashAddElementSymbol(index, hashTable, label, authKey, args);
            break;
        case MACRO_DATATYPE:
            result = hashAddElementMacro(index, hashTable, label, authKey, args);
            break;
        default:
            printf("\nFATAL ERROR @ hashAddElement - reached default\n");
            return ERROR;
    }

    if(result == ERROR)
        return ERROR;
    else {
        hashTable->elementCount += 1;
        return SUCCESS;
    }
}

int hashInsertNodeNewQueue(int nodeDataType, unsigned int index, struct HashTable *hashTable, const char label[], unsigned long authKey, va_list args) {

    hashTable->table[index] = createQueue();
    if(hashTable->table[index] == NULL) {
        printf("\nFATAL ERROR @ hashInsertNodeNewQueue - cannot allocate memory\n");
        return ERROR;
    }

    return hashAddElement(nodeDataType, index, hashTable, label, authKey, args);
}

int hashInsertNode(int nodeDataType, unsigned int index, long currLineNumber, struct HashTable *hashTable, const char label[], unsigned long authKey, va_list args) {

    if(isUniqueLabel(hashTable->table[index], authKey, currLineNumber))
        return hashAddElement(nodeDataType, index, hashTable, label, authKey, args);
    else {
        hashTable->elementCount += 1; /*adding 1 just in case it is the symbol table , even though we don't add the duplicate , we still want to account for it for when we check if we have more than 256 symbols defined , AKA overflow*/
        return SUCCESS;
    }
}

int hashInsert(int nodeArgCount, int nodeDataType, long currLineNumber, struct HashTable *hashTable, const char *label, ...) {

    unsigned int index;
    unsigned long authKey;
    int result;
    va_list args;
    
    index = hashFunctionV2(label);
    authKey = getKey(label);

    va_start(args, label);

    if(hashTable->table[index] == NULL) {
        result = hashInsertNodeNewQueue(nodeDataType, index, hashTable, label, authKey, args);
    }else
        result = hashInsertNode(nodeDataType, index, currLineNumber, hashTable, label, authKey, args);

    va_end(args);
    return result;
}

int instantiateReservedKeywordsTable(int argCount, struct HashTable *hashTable, ...) {

    int i;
    int result;
    const char *label;
    unsigned int operands;
    int opCode;
	va_list args;
	
    if(argCount % 3 != 0) {
        printf("\nFATAL ERROR @ instantiateReservedKeywordsTable - incorrect argument count\n");
        return ERROR;
    }else
        i = argCount / 3;

    va_start(args, hashTable);

    for( ; i > 0 ; --i) {
        label = va_arg(args, const char *);
        operands = va_arg(args,unsigned int);
        opCode = va_arg(args,int);                                                                      /*0 as a default val for now , can change it later on if needed*/
        result = hashInsert(RESERVED_KEYWORD_DATA_ARGC, KEYWORD_DATATYPE, 0,hashTable, label, operands, opCode);
        if(result == ERROR ) {

            printf("\nERROR @ instantiateReservedKeywordsTable - multiple declarations with identical name\n");

            va_end(args);
            return ERROR;
        }

    }
    va_end(args);
    return SUCCESS;
}

struct HashTable *createReservedKeywordsTable() {
    /*if we need to add mcr , endmcr, data, string, extern, entry to the init list ,then we also need to change HASH_TABLE_SIZE from 307 to 311 as to avoid collisions after adding these new strings (at 307 we'll have collisions)*/
    struct HashTable *hashTable;
    int result, hashTableFields;

    hashTableFields = 3;
    hashTable = createHashTable();
    if(hashTable == NULL)
        return NULL;

    result = instantiateReservedKeywordsTable((NUM_OF_OPCODES * hashTableFields), hashTable,"mov", 2, 0,"cmp", 2, 1,"add", 2, 2,"sub", 2, 3,"not", 1, 4,"clr", 1, 5,"lea", 2, 6,"inc", 1, 7,"dec", 1, 8,"jmp", 1, 9,"bne", 1, 10,"red", 1, 11,"prn", 1, 12,"jsr", 1, 13,"rts", 0, 14,"stop", 0,15);
    if(result == SUCCESS)
        return hashTable;
    else
        return NULL;
}

unsigned int hashGetElementCount(const struct HashTable *hashTable) {
    return hashTable->elementCount;
}

int hashIsEmptyCell(const struct HashTable *hashTable, unsigned int index) {

    return (hashTable == NULL || hashTable->table[index] == NULL || isEmpty(hashTable->table[index]));
}

int hashIsEmptyTable(const struct HashTable *hashTable) {
    return (hashTable == NULL || hashTable->elementCount == 0);
}

/*calling function needs to make sure the hashTable sent isn't NULL*/
/*CALLING FUNCTION NEEDS TO CALL hashIsEmptyCell BEFOREHAND*/
const struct Qnode *hashSearchElement(const struct HashTable *hashTable, unsigned long authKey, unsigned int index) {

    const struct Qnode *probe;

    if(!hashIsEmptyCell(hashTable,  index))
        probe = getHeadNode(hashTable->table[index]);
    else
        probe = NULL;

    while(probe != NULL) {

        if(getAuthKey(probe) == authKey)
            return probe;
        else
            probe = getNextNode(probe);
    }

    return NULL;
}

const struct Qnode *hashSearch(const struct HashTable *hashTable, const struct Qnode *node) {

    if(node == NULL || hashIsEmptyTable(hashTable))
        return NULL;
    else
        return hashSearchElement(hashTable, getAuthKey(node), hashFunction(node));
}

const struct Qnode *hashGetNextNonEmptyCell(const struct HashTable *hashTable, unsigned int *index) {

    while( *index < HASH_TABLE_SIZE && hashIsEmptyCell(hashTable, *index)) {
        *index += 1;
    }

    return (*index < HASH_TABLE_SIZE) ? getHeadNode(hashTable->table[(*index)++]) : NULL;
}

void freeHashTable(struct HashTable *hashTable, int nodeDataType) {

    unsigned int i;

    if(hashTable == NULL)
        return;

    for(i = 0 ; i < HASH_TABLE_SIZE ; ++i) {

        if(!hashIsEmptyCell(hashTable, i))
            freeQueue(hashTable->table[i], nodeDataType);
    }

    free(hashTable);
}

int hashIsReservedKeyWord(const struct HashTable *reservedKeywords, char string[]) {
    if(hashSearchElement(reservedKeywords, getKey(string), hashFunctionV2(string)) == NULL)
        return FALSE;
    else
        return TRUE;
}

int hashGetOpCode(const struct HashTable *reservedKeywords, const char directive[]) {

    const struct Qnode *directiveNode;

    directiveNode = hashSearchElement(reservedKeywords, getKey(directive), hashFunctionV2(directive));
    if(directiveNode == NULL) {
        printf("\nERROR @ hashGetOpCode - string : %s was not found \n", directive);
        return 0;
    }else
        return getOpCodeV1(directiveNode);

}


