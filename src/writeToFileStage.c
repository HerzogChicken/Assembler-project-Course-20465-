#include <stdio.h>
#include "queueDS.h"
#include "sharedFunctions.h"
#include "hashTableDS.h"
#include "sharedEnums.h"

void stringChangeFileType(char *file , int fileNameLength, const char *fileType);

void writeDecimalAddressToFile(unsigned int decimalAddress, int isObjFile, FILE *fp) {

    char digit;

    if(isObjFile)
        putc('0', fp);

    digit = (char)( ((decimalAddress / 100) % 10) + '0');
    putc(digit, fp);
    digit = (char)( ((decimalAddress / 10) % 10) + '0');
    putc(digit, fp);
    digit = (char)( (decimalAddress % 10) + '0');
    putc(digit, fp);

    return;
}

void writeBinaryCodeToFile(const struct Qnode *node, FILE *fp) {

    int i;
    int binaryCode;

    binaryCode = getBinaryCode(node);

    for(i = 13 ; i >= 0 ; --i) {
        if(binaryCode & (1 << i))
            putc('/', fp);
        else
            putc('.', fp);
    }
    putc('\n', fp);

    return;
}

void writeNodeLabelToFile(const struct Qnode *directiveNode, FILE *fp) {

    const char *nodeLabelCpy;

    nodeLabelCpy = getNodeLabel(directiveNode);
    for( ; *nodeLabelCpy != '\0' ; ++nodeLabelCpy)
        putc(*nodeLabelCpy, fp);

    return;
}

void writeLabelAndDeciAddressToFile(const struct Qnode *directiveNode, FILE *fp) {

    writeNodeLabelToFile(directiveNode, fp);

    putc(' ', fp);
    putc(' ', fp);

    writeDecimalAddressToFile(getDecimalAddress(directiveNode), FALSE, fp);
    putc('\n', fp);

    return;
}

void writeToFilesEntryTable(const struct HashTable *entryTable, FILE *entryFP) {

    unsigned int index;
    const struct Qnode *entryTableNode;

    index = 0;
    entryTableNode = hashGetNextNonEmptyCell(entryTable, &index);

    while(entryTableNode != NULL) {

        while(entryTableNode != NULL) {

            writeLabelAndDeciAddressToFile(entryTableNode, entryFP);

            entryTableNode = getNextNode(entryTableNode);

        }
        entryTableNode = hashGetNextNonEmptyCell(entryTable, &index);
    }

    return;
}

void writeToObjectFile(const struct Qnode *directiveNode, FILE *objectFP) {

    writeDecimalAddressToFile(getDecimalAddress(directiveNode), TRUE, objectFP);
    putc(' ', objectFP);
    putc(' ', objectFP);
    writeBinaryCodeToFile(directiveNode, objectFP);

    return;
}

void writeToFilesInstructionQ(int directiveCounter, const struct Qnode *directiveNode, FILE *objectFP) {

    while(directiveNode != NULL) {

        addToDecimalAddress((struct Qnode *)directiveNode, directiveCounter); /*bad practice but saves us from adding more lines of (imo) redundant code since the intention can be well understood */
        writeToObjectFile(directiveNode, objectFP);

        directiveNode = getNextNode(directiveNode);
    }

    return;
}

void writeToFilesDirectiveQ(const struct Qnode *directiveNode, FILE *objectFP, FILE *externFP) {
    /*since we only write to file if the file is valid , we don't have to worry about the layer of the while loop which can print extraneous lines in case the file is invalid*/
    while(directiveNode != NULL) {

        writeToObjectFile(directiveNode, objectFP);

        if(isExternStatement(directiveNode)) {
            if(externFP == NULL)
                printf("\nALERT - at function 'writeToFilesDirectiveQ' , externFP is NULL , can't write to file - ALERT\n");
            else
                writeLabelAndDeciAddressToFile(directiveNode, externFP);
        }

        directiveNode = getNextNode(directiveNode);
    }

    return;
}

void startWritingToFile(int directiveCounter, FILE *objectFP, FILE *entryFP, FILE *externFP, const struct Queue *directiveQ, const struct Queue *instructionQ, const struct HashTable *entryTable) {

    putchar('\n');

    if(objectFP != NULL && !isEmpty(directiveQ))
        writeToFilesDirectiveQ(getHeadNode(directiveQ), objectFP, externFP);

    if(objectFP != NULL && !isEmpty(instructionQ))
        writeToFilesInstructionQ(directiveCounter, getHeadNode(instructionQ), objectFP);

    if(entryFP != NULL)
        writeToFilesEntryTable(entryTable, entryFP);

    return;
}

FILE *getFilePointer(char file[], int fileNameLength, const char fileType[]) {

    FILE *tempFP;

    stringChangeFileType(file, fileNameLength, fileType);

    tempFP = fopen(file, "w");/*can easily add an override check as done in main.c , not doing so as I imagine that would get annoying very fast*/
    if(tempFP == NULL) {
        printf("\nFATAL - at function 'getFilePointer' , tempFP is NULL - FATAL\n");
        return NULL;
    }else
        return tempFP;
}

FILE *getObjectFP(char *fileName, int fileNameLength, const struct Queue *directiveQ, const struct Queue *instructionQ) {

    if(!isEmpty(directiveQ) || !isEmpty(instructionQ))
        return getFilePointer(fileName, fileNameLength, ".ob");
    else
        return NULL;
}

FILE *getEntryFP(char *fileName, int fileNameLength, const struct HashTable *entryTable) {

    if(!hashIsEmptyTable(entryTable))
        return getFilePointer(fileName, fileNameLength, ".ent");
    else
        return NULL;
}

FILE *getExternFP(char *fileName, int fileNameLength, const struct HashTable *externTable) {

    if(!hashIsEmptyTable(externTable))
        return getFilePointer(fileName, fileNameLength, ".ext");
    else
        return NULL;
}

void closeFiles(FILE *objectFP, FILE *entryFP, FILE *externFP) {

    if(objectFP != NULL)
        fclose(objectFP);
    if(entryFP != NULL)
        fclose(entryFP);
    if(externFP != NULL)
        fclose(externFP);

    return;
}

void writeToFile(int directiveCounter, char fileName[], int fileNameLength, const struct HashTable *entryTable, const struct HashTable *externTable, struct Queue *directiveQ, struct Queue *instructionQ) {

    FILE *objectFP;
    FILE *entryFP;
    FILE *externFP;

    objectFP = getObjectFP(fileName, fileNameLength, directiveQ, instructionQ);
    entryFP = getEntryFP(fileName, fileNameLength, entryTable);
    externFP = getExternFP(fileName, fileNameLength, externTable);

    startWritingToFile(directiveCounter, objectFP, entryFP, externFP, directiveQ, instructionQ, entryTable);
    closeFiles(objectFP, entryFP, externFP);

    return;
}

