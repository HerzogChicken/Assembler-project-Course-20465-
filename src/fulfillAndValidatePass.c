#include <stdio.h>
#include "queueDS.h"
#include "sharedFunctions.h"
#include "hashTableDS.h"
#include "sharedEnums.h"

int completeRelocatableNode(int directiveCounter, struct Qnode *directiveNode, const struct HashTable *symbolTable) {

    const struct Qnode *symbolNode;
    unsigned int decimalAddress;

    symbolNode = hashSearch(symbolTable, directiveNode);
    if(symbolNode != NULL) {

        decimalAddress = getDecimalAddress(symbolNode);
        if(isInstructionStatement(symbolNode))
            completeNode(directiveNode, LABEL_STATE, (RELOCATABLE + (((int)decimalAddress + directiveCounter) << 2)), FALSE);
        else if(isDirectiveStatement(symbolNode) || isLabelStatement(symbolNode))
            completeNode(directiveNode, LABEL_STATE, (RELOCATABLE + ((int)decimalAddress << 2)), FALSE);
        else
            return ERROR;


        return SUCCESS;

    }else
        return ERROR;
}

int completeExternalNode(struct Qnode *directiveNode, const struct HashTable *externTable) {

    const struct Qnode *symbolNode;

    symbolNode = hashSearch(externTable, directiveNode);
    if(symbolNode != NULL) {
        completeNode(directiveNode, EXTERN_STATE, EXTERNAL, TRUE);
        return SUCCESS;
    }else
        return ERROR;
}

int completeDirectiveNode(int directiveCounter, struct Qnode *directiveNode, const struct HashTable *symbolTable, const struct HashTable *externTable) {

    if(completeRelocatableNode(directiveCounter, directiveNode, symbolTable) == SUCCESS || completeExternalNode(directiveNode, externTable) == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}

int completeDirectiveQ(int directiveCounter, struct Qnode *directiveNode, const struct HashTable *symbolTable, const struct HashTable *externTable) {

    int completedSuccessfully;

    completedSuccessfully = SUCCESS;

    while(directiveNode != NULL) {

        if(!isComplete(directiveNode)) {

            if(completeDirectiveNode(directiveCounter, directiveNode, symbolTable, externTable) == ERROR)
                completedSuccessfully = ERROR;
        }
        directiveNode = (struct Qnode *)getNextNode(directiveNode);
    }

    return completedSuccessfully;
}

int isValidEntryTableElements(int directiveCounter, struct Qnode *entryTableNode, const struct HashTable *symbolTable) {

    int isValid;
    const struct Qnode *symbolTableNode;

    isValid = TRUE;
    symbolTableNode = NULL;

    while(entryTableNode != NULL) {

        if(!hashIsEmptyTable(symbolTable))
            symbolTableNode = hashSearch(symbolTable, entryTableNode);

        if(symbolTableNode == NULL) {
            printf("\nERROR - label %s in current file is set as entry but was not defined\n", getNodeLabel(entryTableNode));
            isValid = FALSE;
        }else {
            if(isInstructionStatement(symbolTableNode))
                setDecimalAddress(entryTableNode, (getDecimalAddress(symbolTableNode) + directiveCounter)); 
            else
                setDecimalAddress(entryTableNode, getDecimalAddress(symbolTableNode));
        }
        entryTableNode = (struct Qnode *)getNextNode(entryTableNode); /*casting as (struct Qnode *) since the function 'getNextNode' returns 'const struct Qnode *' , this is not the best practice but it avoids the need to add additional lines */
    }/*inner loop*/

    return isValid;
}

int validateEntryTable(int directiveCounter, const struct HashTable *entryTable, const struct HashTable *symbolTable) {

    unsigned int index;
    int isValid;
    struct Qnode *entryTableNode;

    index = 0;
    isValid = TRUE;
    /*the time complexity of this nested while loop is O(HASH_TABLE_SIZE) */
    while((entryTableNode = (struct Qnode *)hashGetNextNonEmptyCell(entryTable, &index)) != NULL) {

        if(isValidEntryTableElements(directiveCounter, entryTableNode, symbolTable) == FALSE)
            isValid = FALSE;
    }

    return isValid;
}

void printAllTableElements(const struct HashTable *table, char *errorString) {

    unsigned int index;
    const struct Qnode *tableElement[1];

    index = 0;
    while((*tableElement = hashGetNextNonEmptyCell(table, &index)) != NULL) {

        while(*tableElement != NULL) {

            printf("\nERROR - label %s in current file %s\n", getNodeLabel(*tableElement), errorString);

            *tableElement = getNextNode(*tableElement);
        }
    }
}

int isDisjointTables(const struct HashTable *mainTable, const struct HashTable *comparisonTable, char *errorString) {

    unsigned int index;
    int isValid;
    const struct Qnode *mainTableProbe;
    const struct Qnode *comparisonTableNode;

    index = 0;
    isValid = TRUE;

    while((mainTableProbe = hashGetNextNonEmptyCell(mainTable, &index)) != NULL) {

        while(mainTableProbe != NULL) {

            comparisonTableNode = hashSearch(comparisonTable, mainTableProbe);
            if(comparisonTableNode != NULL) {
                printf("\nERROR - label %s in current file %s\n", getNodeLabel(mainTableProbe), errorString);
                isValid = FALSE;
            }
            mainTableProbe = getNextNode(mainTableProbe);
        }/*inner loop*/
    }/*outer loop*/

    return isValid;
}

int validateTables(int directiveCounter, const struct HashTable *symbolTable, const struct HashTable *entryTable, const struct HashTable *externTable) {

    int isValid;

    isValid = TRUE;

    if(!hashIsEmptyTable(entryTable) && !hashIsEmptyTable(symbolTable)) {
        if(validateEntryTable(directiveCounter, entryTable, symbolTable) == FALSE)
            isValid = FALSE;
    }

    if(!hashIsEmptyTable(externTable) && !hashIsEmptyTable(symbolTable)) {
        if(!isDisjointTables(externTable, symbolTable, "is defined but also set as extern"))
            isValid = FALSE;
    }

    if(!hashIsEmptyTable(entryTable) && !hashIsEmptyTable(externTable)) {
        if(!isDisjointTables(entryTable, externTable, "is set as both , extern and entry"))
            isValid = FALSE;
    }

    return isValid;
}

int validationPass(int isValidFirstPass ,int directiveCounter, const struct Queue *directiveQ, const struct HashTable *symbolTable, const struct HashTable *entryTable, const struct HashTable *externTable) {

    int isValidValidationPass;

    isValidValidationPass = TRUE;

    if(hashGetElementCount(symbolTable) > MAX_SYMBOLS_IN_FILE) {
        printf("\nERROR - number of symbols in the current file (%u) is over the maximum allowed (%d) \n", hashGetElementCount(symbolTable), MAX_SYMBOLS_IN_FILE);
        isValidValidationPass = FALSE;
    }

    if(hashIsEmptyTable(symbolTable) && !hashIsEmptyTable(entryTable)) {
        printAllTableElements(entryTable, "is declared as entry but was not defined");
        isValidValidationPass = FALSE;
    }

    if(validateTables(directiveCounter, symbolTable, entryTable, externTable) == FALSE)
        isValidValidationPass = FALSE;

    if(isValidValidationPass && isValidFirstPass) {
        if(completeDirectiveQ(directiveCounter, (struct Qnode *)getHeadNode(directiveQ), symbolTable, externTable) == SUCCESS)
            return TRUE;
        else
            return FALSE;
    }else
        return FALSE;
    /*that means there's an error , therefore we don't need to complete the symbolTable and we won't write to files so the process for the current file is done ,and we return from here to main , clear the pointers and data structures and start again if there are multiple files to go over*/
    /*decided on using hashTable for entry label and extern (we don't add duplicates , just write a warning), Queue for directive and instruction*/
}

/*1# check elementCount after 1st pass in symbolTable*/
/*2# validate that every node in entryQ is in fact in the symbolTable and update both the nodes in entry and symbolTable, for every instruction node in entry , add the directiveCounter value to their decimalAddress, for every matching node , update node in symbolTable with statement entry*/
/*we check entry nodes by comparing the authKey and checking if the matching node in symbolTable (if exists) doesn't have the entry bit flipped on in the statement field , if it does that means we have multiple declarations of an entry node with the same label*/
/*this way , we can keep using a queue for the entry nodes*/
/*3# merge symbolTable with externQ nodes , send errors accordingly*/
/*4# validate that entry nodes and extern nodes are disjoint*/