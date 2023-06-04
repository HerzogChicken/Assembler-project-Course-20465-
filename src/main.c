#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queueDS.h"
#include "hashTableDS.h"
#include "sharedEnums.h"

enum FileExtension {FILE_EXTENSION_LEN = 5};

int parseMacroV2(int *createFiles ,FILE *fpSource, FILE *fpDest, const struct HashTable *reservedKeywords);
int firstStage(int *createFiles, char *file, int *directiveCounter, int *instructionCounter, struct HashTable *entryTable, struct HashTable *externTable, struct Queue *instructionQ, struct Queue *directiveQ, struct HashTable *symbolTable, const struct HashTable *reservedKeywords);
int validationPass(int isValidFirstPass ,int directiveCounter, const struct Queue *directiveQ, const struct HashTable *symbolTable, const struct HashTable *entryTable, const struct HashTable *externTable);
void writeToFile(int directiveCounter, char fileName[], int fileNameLength, const struct HashTable *entryTable, const struct HashTable *externTable, struct Queue *directiveQ, struct Queue *instructionQ);

void strCopy(char fileName[], const char argument[]) {

    int i;

    for(i = 0 ; argument[i] != '\0' ; ++i)
        fileName[i] = argument[i];

    fileName[i] = '\0';
    return;
}

int stringLength(const char *fileName) { /*not using strlen() as to avoid from implementation defined variables (e.g size_t) as much as possible*/

    int strLen;

    for(strLen = 0 ; fileName[strLen] != '\0' ; ++strLen)
        ;

    return strLen;
}

void stringChangeFileType(char *file , int fileNameLength, const char *fileType) {

    int i;
    int j;

    i = fileNameLength;
    for(j = 0 ; fileType[j] != '\0' ; ++j , ++i)
        file[i] = fileType[j];

    file[i] = '\0';
    return;
}

int copyNewFileName(int *newFileNameLen , int *currFileNameLen , char **fileName, char *newFileName) {

    *newFileNameLen = stringLength(newFileName);

    if(*currFileNameLen < *newFileNameLen) {
        *fileName = (char *)malloc(sizeof(char) * ((*newFileNameLen) + FILE_EXTENSION_LEN));
        if(fileName == NULL) {
            printf("\nWARNING - cannot allocate enough memory for current file name : %s\n", newFileName);
            return ERROR;
        }else
            *currFileNameLen = *newFileNameLen;

    }

    strCopy(*fileName, newFileName);
    return SUCCESS;
}

void freeMainHashTables(struct HashTable **symbolTable, struct HashTable **entryTable, struct HashTable **externTable) {

    if(*symbolTable != NULL)
        freeHashTable(*symbolTable, SYMBOL_DATATYPE);

    if(*entryTable != NULL)
        freeHashTable(*entryTable, SYMBOL_DATATYPE);

    if(*externTable != NULL)
        freeHashTable(*externTable, SYMBOL_DATATYPE);


    return;
}

void freeMainDataStructures(struct HashTable **symbolTable, struct HashTable **entryTable, struct HashTable **externTable, struct Queue **instructionQ, struct Queue **directiveQ) {

    freeMainHashTables(symbolTable, entryTable, externTable);
    freeQueue(*instructionQ, SYMBOL_DATATYPE);
    freeQueue(*directiveQ, SYMBOL_DATATYPE);

    return;
}

int createNewQueues(struct Queue **instructionQ, struct Queue **directiveQ) {

    *instructionQ = createQueue();
    if(*instructionQ == NULL) {
        return ERROR;
    }

    *directiveQ = createQueue();
    if(*directiveQ == NULL) {
        freeQueue(*instructionQ, SYMBOL_DATATYPE);
        return ERROR;
    }

    return SUCCESS;
}

int createNewHashTables(struct HashTable **symbolTable, struct HashTable **entryTable, struct HashTable **externTable) {

    *symbolTable = createHashTable();
    if(*symbolTable == NULL)
        return ERROR;

    *entryTable = createHashTable();
    if(*entryTable == NULL) {
        freeHashTable(*symbolTable, SYMBOL_DATATYPE);
        return ERROR;
    }

    *externTable = createHashTable();
    if(*externTable == NULL) {
        freeHashTable(*symbolTable, SYMBOL_DATATYPE);
        freeHashTable(*entryTable, SYMBOL_DATATYPE);
        return ERROR;
    }

    return SUCCESS;
}

int createMainDataStructures(struct HashTable **symbolTable, struct HashTable **entryTable, struct HashTable **externTable, struct Queue **instructionQ, struct Queue **directiveQ) {

    if(createNewHashTables(symbolTable, entryTable, externTable) == ERROR)
        return ERROR;

    if(createNewQueues(instructionQ, directiveQ) == ERROR) {
        freeMainHashTables(symbolTable, entryTable, externTable);
        return ERROR;
    }

    return SUCCESS;
}

int getInputDataFileFP(char *file, int fileNameLength, FILE **fpSource) {

    stringChangeFileType(file, fileNameLength, ".as");
    *fpSource = fopen(file, "r");

    if(*fpSource == NULL) { /*this is not a specific error , we can use errno.h but how can we ensure that the value won't change in between the instructions (need to lock the thread?)*/
        printf("\nWARNING - file : %s was not found\n", file);
        return ERROR;
    }

    return SUCCESS;
}

int fileOverrideWarning(char *file) {

    int i;
    int inputChar;

    printf("\nWARNING - file %s already exists, any information on it will be discarded\n", file);

    printf("\nEnter 'y' to proceed or 'n' to abort\n");
    for(i = 0 ; (inputChar = getchar()) != EOF && inputChar != '\0' ; ++i) {

        if(inputChar != 'y' && inputChar != 'Y' && inputChar != 'n' && inputChar != 'N') {
            continue;
        }else if(inputChar == 'y' || inputChar == 'Y') {
            printf("\nOverriding file\n");
            return TRUE;
        }else {
            printf("\nAborting work on current file\n");
            return FALSE;
        }

    }

    return FALSE;
}

int getMainOutputDataFileFP(char *file, int fileNameLength, FILE **fpExpanded) {

    FILE *fpCheckFileStatus;

    stringChangeFileType(file, fileNameLength, ".am");

    fpCheckFileStatus = fopen(file, "r");
    if(fpCheckFileStatus != NULL && fileOverrideWarning(file) == FALSE) {
        fclose(fpCheckFileStatus);
        return ERROR;
    }

    *fpExpanded = fopen(file, "w");

    if(fpExpanded == NULL) {
        printf("\nERROR - cannot create\\open file %s\n", file);
        return ERROR;
    }else
        return SUCCESS;
}

int startMacroExpansion(char *file, int fileNameLength, int *createFiles, const struct HashTable *reservedKeywords) {

    FILE *fpExpanded;
    FILE *fpSource;

    if(getInputDataFileFP(file, fileNameLength, &fpSource) == ERROR)
        return ERROR;

    if(getMainOutputDataFileFP(file, fileNameLength, &fpExpanded) == ERROR)
        return ERROR;

    parseMacroV2(createFiles, fpSource, fpExpanded, reservedKeywords);
    fclose(fpSource);
    fclose(fpExpanded);
    if(!createFiles[0]) {
        printf("\nNOTICE - at least one error was found during the macro expansion phase and therefore the program won't proceed working on the current file (and move onto the next available file , if provided).\n");
        return ERROR;
    }else
        return SUCCESS;
}

void startAssembler(char *file, int fileNameLength, const struct HashTable *reservedKeywords) {

    struct HashTable *symbolTable, *entryTable, *externTable;

    struct Queue *instructionQ, *directiveQ;

    int createFiles[1], directiveCounter[1], instructionCounter[1];

    createFiles[0] = TRUE;
    directiveCounter[0] = 100;
    instructionCounter[0] = 0;

    if(startMacroExpansion(file, fileNameLength, createFiles, reservedKeywords) == ERROR || createFiles[0] ==  FALSE)
        return;

    if(createMainDataStructures(&symbolTable, &entryTable, &externTable, &instructionQ, &directiveQ) == ERROR)
        return;

    firstStage(createFiles, file, directiveCounter, instructionCounter, entryTable, externTable, instructionQ, directiveQ, symbolTable, reservedKeywords);

    createFiles[0] = validationPass(createFiles[0], *directiveCounter, directiveQ, symbolTable, entryTable, externTable);

    if(createFiles[0])
        writeToFile(*directiveCounter, file, fileNameLength, entryTable, externTable, directiveQ, instructionQ);

    freeMainDataStructures(&symbolTable, &entryTable, &externTable, &instructionQ, &directiveQ);

    return;
}

void mainProgramLoop(int argc, char *argv[]) {
    /*using dynamic allocation , we can also accept file names including a path , user must make sure the program has the correct permissions to create/modify/read the needed files*/
    int i;
    int newFileNameLen[1], currFileNameLen[1];
    char *file;
    struct HashTable *reservedKeywords;

    *newFileNameLen = *currFileNameLen = 0;

    if(argc <= 1) {
        printf("\nWARNING - no file names were passed to the program\n");
        return;
    }else {
        reservedKeywords = createReservedKeywordsTable();
        if(reservedKeywords == NULL)
            return;
    }

    for(i = 1 ; i < argc ; ++i) {
        /*in order to avoid calling 'realloc' , we keep the size of the 'file' in case it is bigger than the new file name , we call malloc only if the new file name is longer than the current allocated space*/
        if(copyNewFileName(newFileNameLen, currFileNameLen, &file, argv[i]) == ERROR)
            continue;

        startAssembler(file, *newFileNameLen, reservedKeywords);
    }

    free(file);
    freeHashTable(reservedKeywords, KEYWORD_DATATYPE);

    return;
}

int main(int argc, char *argv[]) {

    mainProgramLoop(argc, argv);

    return 0;
}

