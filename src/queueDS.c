#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sharedEnums.h"


struct Qnode_SymbolData {

    unsigned int deciAddress; /*decimal address*/
    int binaryCode; /*binary machine code*/
    unsigned int statement;
};

struct Qnode_ReservedKeywordData {

    unsigned int operands;
    int opCode;
};

struct Qnode_MacroData {

    int startingPos;
    int endingPos;
};

struct Qnode {

    struct Qnode *next;
    char label[MAX_LABEL_LEN];
    unsigned long authKey;
    void *data;
};

struct Queue {

    struct Qnode *head;
    struct Qnode *tail;
};

void freeMacroData(struct Qnode_MacroData *nodeData) {

    if(nodeData == NULL) {
        printf("\nERROR @ freeMacroData - nodeData is NULL\n");
        return;
    }
    free(nodeData);
}

void freeSymbolData(struct Qnode_SymbolData *nodeData) {

    if(nodeData == NULL) {
        printf("\nERROR @ freeSymbolData - nodeData is NULL\n");
        return;
    }
    free(nodeData);
}

void freeKeywordData(struct Qnode_ReservedKeywordData *nodeData) {

    if(nodeData == NULL) {
        printf("\nERROR @ freeKeywordData - nodeData is NULL\n");
        return;
    }
    free(nodeData);
}

void freeQNodeData(void *data, int nodeDataType) {

    switch(nodeDataType) {
        case KEYWORD_DATATYPE:
            freeKeywordData((struct Qnode_ReservedKeywordData *)data);
            break;
        case SYMBOL_DATATYPE:
            freeSymbolData((struct Qnode_SymbolData *)data);
            break;
        case MACRO_DATATYPE:
            freeMacroData((struct Qnode_MacroData *)data);
            break;
        default:
            printf("\nERROR @ freeQNodeData - reached default\n");
            break;
    }
}

int freeQueue(struct Queue *queue, int nodeDataType) {

    struct Qnode *currNode;
    struct Qnode *tempNode;

    currNode = queue->head;

    while(currNode != NULL) {

        tempNode = currNode;
        currNode = currNode->next;
        freeQNodeData(tempNode->data, nodeDataType);
        free(tempNode);
    }

    free(queue);

    return 0;
}

int isEmpty(const struct Queue *queue) {
    return (queue->tail == NULL && queue->head == NULL);
}

struct Qnode_MacroData *allocMacroData() {
    return (struct Qnode_MacroData *)malloc(sizeof(struct Qnode_MacroData));
}

struct Qnode_SymbolData *allocSymbolData() {
    return (struct Qnode_SymbolData*) malloc(sizeof(struct Qnode_SymbolData));
}

struct Qnode_ReservedKeywordData *allocResKeywordData() {
    return (struct Qnode_ReservedKeywordData*) malloc(sizeof(struct Qnode_ReservedKeywordData));
}

struct Qnode *allocQNode() {
    return (struct Qnode *) malloc(sizeof(struct Qnode));
}

/*int startingPos, int endingPos*/
void fillMacroData(struct Qnode *newNode, va_list args) {

    ((struct Qnode_MacroData *)(newNode->data))->startingPos = va_arg(args, int);
    ((struct Qnode_MacroData *)(newNode->data))->endingPos = va_arg(args, int);
}

/*unsigned int deciAddress, int binaryCode, unsigned int statement*/
void fillSymbolData(struct Qnode *newNode, va_list args) {

    ((struct Qnode_SymbolData *)(newNode->data))->deciAddress = va_arg(args,unsigned int);
    ((struct Qnode_SymbolData *)(newNode->data))->binaryCode = va_arg(args, int);
    ((struct Qnode_SymbolData *)(newNode->data))->statement =va_arg(args,unsigned int);
}
/*unsigned int operands, int opCode*/
void fillResKeywordData(struct Qnode *newNode, va_list args) {

    ((struct Qnode_ReservedKeywordData *)(newNode->data))->operands = va_arg(args,unsigned int);
    ((struct Qnode_ReservedKeywordData *)(newNode->data))->opCode = va_arg(args, int);
}

int fillDataFieldSymbolData(struct Qnode **newNode, va_list args) {

    (*newNode)->data = allocSymbolData();
    if((struct Qnode_SymbolData *)((*newNode)->data) == NULL) {
        printf("\nERROR - enqueue - cannot allocate memory for Qnode_SymbolData\n");
        return ERROR;
    }else {
        fillSymbolData(*newNode, args);
        return SUCCESS;
    }
}

int fillDataFieldKeywordData(struct Qnode **newNode, va_list args) {

    (*newNode)->data = allocResKeywordData();
    if((struct Qnode_ReservedKeywordData *)((*newNode)->data) == NULL) {
        printf("\nERROR - enqueue - cannot allocate memory for Qnode_ReservedKeywordData\n");
        return ERROR;
    }else {
        fillResKeywordData(*newNode, args);
        return SUCCESS;
    }
}

int fillDataFieldMacroData(struct Qnode **newNode, va_list args) {

    (*newNode)->data = allocMacroData();
    if((struct Qnode_MacroData *)((*newNode)->data) == NULL) {
        printf("\nERROR - enqueue - cannot allocate memory for Qnode_MacroData\n");
        return ERROR;
    }else {
        fillMacroData(*newNode, args);
        return SUCCESS;
    }
}

int fillDataField(struct Qnode *newNode,  int nodeDataType, va_list args) {

    switch(nodeDataType) {
        case SYMBOL_DATATYPE:
            return fillDataFieldSymbolData(&newNode, args);
        case KEYWORD_DATATYPE:
            return fillDataFieldKeywordData(&newNode, args);
        case MACRO_DATATYPE:
            return fillDataFieldMacroData(&newNode, args);
        default:
            printf("\nFATAL @ fillDataField reached default\n");
            return ERROR;
    }
}

int enqueueV1_1(int nodeArgCount, int nodeDataType, struct Queue *queue, const char label[], unsigned long authKey, ...) {

    struct Qnode *newNode;
    int i;
    va_list args;
    
    newNode = allocQNode();
    if(newNode == NULL) {
        printf("\nERROR - enqueue - cannot allocate memory for Qnode\n");
        return ERROR;
    }

    va_start(args, authKey);
    fillDataField(newNode, nodeDataType, args);
    va_end(args);

    newNode->next = NULL;
    for( i = 0 ; label[i] != '\0' && label[i] != ':'; ++i) {
        newNode->label[i] = label[i];
    }
    newNode->label[i] = '\0';
    newNode->authKey = authKey;
    if(isEmpty(queue)) {
        queue->head = queue->tail = newNode;
    }else {
        queue->tail->next = newNode;
        queue->tail = newNode;
    }

    return SUCCESS;
}

struct Queue *allocQueue() {
    return (struct Queue *)malloc(sizeof(struct Queue));
}

struct Queue *createQueue() {

    struct Queue *newQueue;
    newQueue = allocQueue();
    if(newQueue == NULL) {
        printf("\nERROR - createQueue - cannot allocate memory \n");
        return NULL;
    }

    newQueue->head = NULL;
    newQueue->tail = NULL;

    return newQueue;
}

struct Qnode *getCurrentNode(struct Queue *queue) {

    if(isEmpty(queue)) {
        printf("\nWARNING - tailPointer - queue is empty \n");
    }
    return queue->tail;
}
/*caller must make sure the passing node is of matching type*/
void addToBinaryCode(struct Qnode *node, int value) {
    ((struct Qnode_SymbolData *)(node->data))->binaryCode |= value;
    return;
}

void updateTailBinaryCode(struct Queue *queue, int value) {
    addToBinaryCode(getCurrentNode(queue), value);
    return;
}

unsigned int isComplete(const struct Qnode *node) {
    if(node == NULL)
        return 0;

    return (((struct Qnode_SymbolData *)(node->data))->statement & COMPLETED_STATE);
}

/*caller must make sure the passing node is of matching type*/
unsigned int getNodeStatement(const struct Qnode *node) {
    if(node == NULL) {
        printf("\nERROR at function nodeGetStatement - node is NULL\n");
        return 0;
    }else
        return ((struct Qnode_SymbolData *)(node->data))->statement;
}

unsigned int hashFunction(const struct Qnode *node) {

    /*FNV a1 hash function, might implement halfsiphash later on */
    int i;
    unsigned long FNV_offset_basis;
    unsigned long FNV_prime;
    unsigned long hash;

    FNV_offset_basis = 2166136261u;
    FNV_prime = 16777619u;
    hash = FNV_offset_basis;

    for(i = 0; node->label[i] != '\0' && node->label[i] != ':' ; i++)
    {
        hash ^= node->label[i];
        hash *= FNV_prime;
    }
    return hash % HASH_TABLE_SIZE;
}

const struct Qnode *getHeadNode(const struct Queue *queue) {
    return queue->head;
}

const struct Qnode *getNextNode(const struct Qnode *node) {
    return node->next;
}

const char *getNodeLabel(const struct Qnode *node) { /*calling function must make sure the node isn't null*/
    return node->label;
}

/*caller must make sure the passing node is of matching type*/
void addToStatement(struct Qnode *node, unsigned int statementVal) {
    if(node != NULL)
        ((struct Qnode_SymbolData *)(node->data))->statement |= statementVal;

    return;
}
/*caller must make sure the passing node is of matching type*/
void completeNode(struct Qnode *node, unsigned int statementVal, int binaryVal, int overRide) {
    if(node == NULL)
        return;

    addToStatement(node, COMPLETED_STATE);
    ((struct Qnode_SymbolData *)(node->data))->statement ^= NOT_COMPLETE_STATE;
    addToStatement(node, statementVal);
    if(overRide)
        ((struct Qnode_SymbolData *)(node->data))->binaryCode = binaryVal;
    else
        addToBinaryCode(node, binaryVal);

    return;
}
/*caller must make sure the passing node is of matching type*/
unsigned int getDecimalAddress(const struct Qnode *node) { /*calling function has to make sure node isn't NULL*/
    if(node == NULL)
        return 0;

    return ((struct Qnode_SymbolData *)(node->data))->deciAddress;
}
/*caller must make sure the passing node is of matching type*/
int getBinaryCode(const struct Qnode *node) { /*calling function has to make sure node isn't NULL*/
    if(node == NULL)
        return 0;

    return ((struct Qnode_SymbolData *)(node->data))->binaryCode;
}
/*caller must make sure the passing node is of matching type*/
unsigned int isLabelStatement(const struct Qnode *node) {
    return (getNodeStatement(node) & LABEL_STATE);
}

unsigned int isValueStatement(const struct Qnode *node) {
    return (getNodeStatement(node) & VALUE_STATE);
}

unsigned int isExternStatement(const struct Qnode *node) {
    return (getNodeStatement(node) & EXTERN_STATE);
}

unsigned int isEntryStatement(const struct Qnode *node) {
    return (getNodeStatement(node) & ENTRY_STATE);
}

unsigned int isInstructionStatement(const struct Qnode *node) {
    return (getNodeStatement(node) & INSTRUCTION_STATE);
}

unsigned int isDirectiveStatement(const struct Qnode *node) {
    return (getNodeStatement(node) & DIRECTIVE_STATE);
}

unsigned int isUnknownStatement(const struct Qnode *node) {
    return (getNodeStatement(node) & 0);
}

int isIdenticalAuthKey(const struct Qnode *nodeA, const struct Qnode *nodeB) {
    return (nodeA->authKey == nodeB->authKey);
}

void addToDecimalAddress(struct Qnode *node, unsigned int val) {
    ((struct Qnode_SymbolData *)(node->data))->deciAddress += val;

    return;
}

void setDecimalAddress(struct Qnode *node, unsigned int decimalAddress) {
    ((struct Qnode_SymbolData *)(node->data))->deciAddress = decimalAddress;

    return;
}

unsigned long getAuthKey(const struct Qnode *node) {
    if(node == NULL)
        return 0;

    return node->authKey;
}

int getOpCodeV1(const struct Qnode *node) {
    if(node == NULL)
        return 0;
    else
        return ((struct Qnode_ReservedKeywordData *)(node->data))->opCode;
}

int getMacroStartingPos(const struct Qnode *node) {
    if(node == NULL)
        return 0;
    else
        return ((struct Qnode_MacroData *)(node->data))->startingPos;
}

int getMacroEndingPos(const struct Qnode *node) {
    if(node == NULL)
        return 0;
    else
        return ((struct Qnode_MacroData *)(node->data))->endingPos;
}
