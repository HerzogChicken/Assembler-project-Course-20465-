#include <stdio.h>
#include <ctype.h>
#include "queueDS.h"
#include "sharedFunctions.h"
#include "hashTableDS.h"
#include "sharedEnums.h"

int flushInputLineMacro(FILE *fpSource, FILE *fpDest, long *endOfLinePos) {

    int fileChar;

    while((fileChar = getc(fpSource)) != EOF && fileChar != '\n') {

        if(fileChar > 127 || fileChar < -127)
            putc(' ', fpDest);
        else
            putc(fileChar, fpDest);

        (*endOfLinePos)++;
    }

    if(fileChar == '\n')
        putc(fileChar, fpDest);

    return (fileChar == EOF) ? STOP : EMPTY_LINE;
}

void copyLineToDestFile(const char buffer[], int lastLine, FILE *fpDest) {

    int i;

    for(i = 0 ; buffer[i] != '\n' ; ++i)
        putc(buffer[i], fpDest);

    if(!lastLine)
        putc(buffer[i], fpDest);
}

int fillLineBufferMacro(int *index, int *fileChar, char lineBuffer[], long *endOfLinePos, long currLineNum, FILE *fpSource, FILE *fpDest) {
    /*not checking for ';' because we need to go over the line anyway to get to the next one since we don't know the length of it in order to skip to the next one*/
    for(*index = 0 ; *index < MAX_LINE && (*fileChar = getc(fpSource)) != EOF ; ++(*index)) {                                    /*search end of macro can break , if we get endmcr but after than 75 white spaces then we have overflow and ignore that line, so do we                          */
        (*endOfLinePos)++;                                                                                                              /*keep going if we have a line with more than 80 chars in the macro expansion stage or do we take the risk and can have multiple errors of overflow in case     */
                                                                                                                                        /*that line is inside a macro definition ,and it's being called multiple times, for the moment I will opt for the latter option but need to ask roi about it    */
        if(*index == MAX_LINE - 1) {
            if(*fileChar > 127) {
                printf("\nERROR @ line %ld - input contains illegal character, character is not in ASCII code\n", currLineNum);
                lineBuffer[*index] = ' ';
            }else
                lineBuffer[*index] = *fileChar;

            printf("\nERROR @ line %ld - input text overflow, lines cannot exceed 80 characters\n", currLineNum);
            lineBuffer[*index + 1] = '\n';
            copyLineToDestFile(lineBuffer, TRUE, fpDest);

            return flushInputLineMacro(fpSource, fpDest, endOfLinePos);
        }else if(*fileChar > 127 || *fileChar < -127) {
            printf("\nERROR @ line %ld - input contains illegal character, character is not in ASCII code\n", currLineNum);
            lineBuffer[*index] = ' ';
            continue;
        }

        lineBuffer[*index] = *fileChar;
        if(*fileChar == '\n') {
            lineBuffer[*index + 1] = '\0';
            break;
        }
    }

    return SUCCESS;
}

/*invalid characters will be ignored , line overflow , only the first 80 characters will be added, the rest will be discarded*/
int getNewLineMacro(char lineBuffer[], long *endOfLinePos, FILE *fpSource, FILE *fpDest, long *currLineNum) {

    int fileChar[1], i[1];
    int j, newLineBufferState;

    *i =  *fileChar = j = 0;
    *currLineNum += 1;

    newLineBufferState = fillLineBufferMacro(i, fileChar, lineBuffer, endOfLinePos, *currLineNum, fpSource, fpDest);
    if(newLineBufferState != SUCCESS)
        return newLineBufferState;
    
    while(lineBuffer[j] != '\n' && isspace(lineBuffer[j])) {
        j += 1;
    }

    if(*fileChar == EOF) {
        if(lineBuffer[j] == ';' || *i == 0 || lineBuffer[j] == '\n')
            return STOP; /*modify to show that we can ignore the line and finish*/
        else {
            lineBuffer[*i] = '\n';
            lineBuffer[*i + 1] = '\0';
            return LAST_LINE;/*modify to show that it's the last line to process and not to call getNewLine again*/
        }
    }else if(lineBuffer[j] == ';' || lineBuffer[j] == '\n') {
        copyLineToDestFile(lineBuffer, FALSE, fpDest);
        return EMPTY_LINE;
    }else
        return SUCCESS;
}

int copyChunkToDestFile(FILE *fpOrigin, FILE *fpDest, long startingPos, long endingPos, long endOfLinePos) {

    long charCount;

    if(fseek(fpOrigin, startingPos, SEEK_SET) != 0) {
        printf("\nERROR - fseek cannot move to startingPos\n");
        return ERROR;
    }

    charCount = endingPos - startingPos;

    while(charCount-- > 0) {
        putc(getc(fpOrigin), fpDest);
    }

    if(fseek(fpOrigin, endOfLinePos, SEEK_SET) != 0) {
        printf("\nERROR - fseek cannot move to endOfLinePos\n");
        return ERROR;
    }

    return SUCCESS;
}

/*assuming the closing macro is valid , otherwise we have to validate the closing macro , and work on edge cases like if a label is set as 'endmcr:' , we need to write an error and also keep searching for the closing macro */
int isClosingMacroDeclaration(const char buffer[], int *currLinePos) {

    skipWhiteSpace(buffer, currLinePos);

    if(buffer[(*currLinePos)] == 'e' && buffer[(*currLinePos) + 1] == 'n' && buffer[(*currLinePos) + 2] == 'd' && buffer[(*currLinePos) + 3] == 'm' && buffer[(*currLinePos) + 4] == 'c' && buffer[(*currLinePos) + 5] == 'r') {

        if(isspace(buffer[(*currLinePos) + 6])) {
            *currLinePos += 6;
            return TRUE;
        }else
            return FALSE;

    }else
        return FALSE;
}
/*add option to go over the lines and print them to the expansion file until we reach a closing macro , in case the declaration is valid, we have opening and closing , but the label is invalid*/
long getEndOfMacroPos(char buffer[], int *currLinePos, long *endOfLinePos, int flushChunkToDestFile, FILE *fpSource, FILE *fpDest, long *currLineNum) { /*function is a little over 20 lines but I really think it shouldn't be broken up into smaller sub functions*/

    int result;
    long lastMacroLinePos;
    long firstMacroLinePos;

    firstMacroLinePos = *endOfLinePos + 1;
    lastMacroLinePos = *endOfLinePos;

    while((result = getNewLineMacro(buffer, endOfLinePos, fpSource, fpDest, currLineNum)) != STOP) {
        *currLinePos = 0;
        switch(result) {
            case SUCCESS:
            case LAST_LINE:

                if(isClosingMacroDeclaration(buffer, currLinePos)) {

                    if(flushChunkToDestFile) {/*in case we have an invalid label , we copy the lines we already passed while searching for a closing macro (which is guaranteed by the given assumption) and discard the opening macro and closing macro lines*/
                        copyChunkToDestFile(fpSource, fpDest, firstMacroLinePos, lastMacroLinePos, *endOfLinePos);
                        return ERROR;
                    }else
                        return lastMacroLinePos;
                }else {
                    lastMacroLinePos = *endOfLinePos;
                    break;
                }
            case EMPTY_LINE:
                lastMacroLinePos = *endOfLinePos;
                break;
            default:
                printf("\nFATAL - function getEndOfMacroPos - reached default\n");
                return ERROR;
        }/*switch*/
    }/*while loop*/

    printf("\nFATAL - function getEndOfMacroPos - macro is not terminated correctly\n");
    return ERROR; /*we're told we can assume there will be a closing macro , in it doesn't happen we return an error*/
}

int getMacroLabel(const char buffer[], int *currLinePos, char *macroLabel, const struct HashTable *reservedKeywords, long currLineNum) {

    int i;

    for(i = 0 ; i < MAX_LABEL_LEN && !isspace(buffer[*currLinePos]) ; ++i, (*currLinePos)++) {

        if(!isalnum(buffer[*currLinePos])) {
            printf("\nERROR @ line %ld - label of a macro definition can only contain digits and letters\n", currLineNum);
            return ERROR;
        }else {
            if(i == MAX_LABEL_LEN - 1) {
                printf("\nERROR @ line %ld - label's length of the macro definition exceeds the maximum length\n", currLineNum);
                return ERROR;
            }else
                macroLabel[i] = buffer[*currLinePos];
        }
    }/*for loop*/

    macroLabel[i] = '\0';
    if(hashSearchElement(reservedKeywords, getKey(macroLabel), hashFunctionV2(macroLabel)) != NULL) {
        printf("\nERROR @ line %ld - macro label cannot not be the same as a reserved keyword\n", currLineNum);
        return ERROR;
    }else
        return SUCCESS;
}
/*add reservedKeyword check , if we need to expand a macro with invalid label , don't do it and just move on and remove the label calling for macro expansion*/
int getValidMacroLabel(char buffer[], int *currLinePos, char *macroLabel, const struct HashTable *reservedKeywords, long currLineNum) {

    skipWhiteSpace(buffer, currLinePos);

    if(buffer[*currLinePos] == '\n') {
        printf("\nERROR @ line %ld - macro definition is missing a label\n", currLineNum);
        return ERROR;
    }else if(isdigit(buffer[*currLinePos])) {
        printf("\nERROR @ line %ld - label of a macro definition cannot start with a digit\n", currLineNum);
        return ERROR;
    }

    if(getMacroLabel(buffer, currLinePos, macroLabel, reservedKeywords, currLineNum) == ERROR)
        return ERROR;

    skipWhiteSpace(buffer, currLinePos);

    if(buffer[*currLinePos] != '\n') {
        printf("\nERROR @ line %ld - extraneous text after macro label\n", currLineNum);
        return ERROR;
    }else
        return SUCCESS;
}

/*since we're not told about the naming convention of macros , we assume it follows the same convention of a label*/
int isStartMacroDeclaration(const char buffer[], int *currLinePos, long currLineNum) {

    if(buffer[(*currLinePos)] == 'm' && buffer[(*currLinePos) + 1] == 'c' && buffer[(*currLinePos) + 2] == 'r') {

        if(buffer[(*currLinePos) + 3] == '\n') {
            printf("\nERROR @ line %ld - macro definition is missing a label\n", currLineNum);
            return ERROR; /*assuming there's a valid macro label since the given example of a working algorithm assumes it implicitly*/
        }else if(isspace(buffer[(*currLinePos) + 3])) {
            *currLinePos += 3;
            return TRUE;
        }else
            return FALSE;

    }else
        return FALSE;
}

int processMacro(char buffer[], int *currLinePos, char *macroLabel, long *endOfLinePos, struct HashTable *macroTable, const struct HashTable *reservedKeywords, FILE *fpSource, FILE *fpDest , long *currLineNum) {

    int startingPos;
    int endingPos;
    int flushChunkToDestFile;

    if(getValidMacroLabel(buffer, currLinePos, macroLabel, reservedKeywords, *currLineNum) == ERROR)
        flushChunkToDestFile = TRUE;
    else {
        flushChunkToDestFile = FALSE;
        startingPos = (*endOfLinePos) ;
    }

    endingPos = getEndOfMacroPos(buffer, currLinePos, endOfLinePos, flushChunkToDestFile, fpSource, fpDest, currLineNum);
    if(endingPos == ERROR)
        return ERROR;
    else {
        hashInsert(MACRO_DATA_ARGC, MACRO_DATATYPE, *currLineNum, macroTable, macroLabel, startingPos, endingPos);
        return SUCCESS;
    }
}

int expandMacro(FILE *fpOrigin, FILE *fpDest, long startingPos, long endingPos, long endOfLinePos) {

    return copyChunkToDestFile(fpOrigin, fpDest, startingPos, endingPos, endOfLinePos);
}

int isValidMacroLabel(const char buffer[], int *currLinePos, char macroCall[], const struct HashTable *reservedKeywords) {

    int i;

    if(isdigit(buffer[*currLinePos]))
        return FALSE;

    for( i = 0 ; !isspace(buffer[*currLinePos]) ; i++ , (*currLinePos)++) {
        if(i >= MAX_LABEL_LEN - 1 || !isalnum(buffer[*currLinePos]))
            return FALSE;
        else
            macroCall[i] = buffer[*currLinePos];
    }

    macroCall[i] = '\0';

    skipWhiteSpace(buffer, currLinePos);

    if(buffer[*currLinePos] == '\n' && hashSearchElement(reservedKeywords, getKey(macroCall), hashFunctionV2(macroCall)) == NULL)
        return TRUE;
    else
        return FALSE;
}

const struct Qnode *isMacroCall(const char buffer[], int *currLinePos, char *macroCall, const struct HashTable *macroTable, const struct HashTable *reservedKeywords) {

    if(!isValidMacroLabel(buffer, currLinePos, macroCall, reservedKeywords))
        return NULL;

    return hashSearchElement(macroTable, getKey(macroCall), hashFunctionV2(macroCall));
}

int searchMacro(char buffer[], int *currLinePos, char *macroLabel, char *macroCall, long *endOfLinePos, int lastLine, FILE *fpOrigin, FILE *fpDest, struct HashTable *macroTable, const struct HashTable *reservedKeywords, long *currLineNum ) {

    int result;
    const struct Qnode *macroNodeCpy;

    skipWhiteSpace(buffer, currLinePos);

    result = isStartMacroDeclaration(buffer, currLinePos, *currLineNum);
    if(result != FALSE) {

        if(result == ERROR)
            return ERROR;

        result = processMacro(buffer, currLinePos, macroLabel, endOfLinePos, macroTable, reservedKeywords, fpOrigin, fpDest, currLineNum);
        if(result == ERROR)
            return ERROR;

    }else if(isClosingMacroDeclaration(buffer, currLinePos)) {
        printf("\nERROR @ line %ld - invalid end of macro declaration , missing starting declaration and a label\n", *currLineNum);
        /*just ignore the line */
        return SUCCESS;
    }else if(  (macroNodeCpy = isMacroCall(buffer, currLinePos, macroCall, macroTable, reservedKeywords)) != NULL) {
        expandMacro(fpOrigin, fpDest, getMacroStartingPos(macroNodeCpy), getMacroEndingPos(macroNodeCpy), *endOfLinePos);
    }else
        copyLineToDestFile(buffer, lastLine, fpDest);


    return SUCCESS;
}

int runParseMacro(int *createFiles, char macroLabel[], char macroCall[], long *endOfLinePos, long *currLineNum, int *currLinePos, char buffer[], FILE *fpSource, FILE *fpDest, struct HashTable *macroTable, const struct HashTable *reservedKeywords) {

    int result;

    while((result = getNewLineMacro(buffer, endOfLinePos, fpSource, fpDest, currLineNum)) != STOP) {

        *currLinePos = 0;
        macroCall[0] = '\0';
        macroLabel[0] = '\0';

        switch(result) {
            case SUCCESS:
                if(searchMacro(buffer, currLinePos, macroLabel, macroCall, endOfLinePos, FALSE, fpSource, fpDest, macroTable, reservedKeywords, currLineNum) == ERROR)
                    *createFiles = FALSE;
                break;
            case EMPTY_LINE:
                break;
            case LAST_LINE:
                if(searchMacro(buffer, currLinePos, macroLabel, macroCall, endOfLinePos, TRUE, fpSource, fpDest, macroTable, reservedKeywords, currLineNum) == ERROR)
                    *createFiles = FALSE;

                return *createFiles;
            default:
                printf("\nFATAL - function runParseMacro - reached default\n");
                return ERROR;
        }
    }/*end of while loop*/

    return ERROR;
}

/* if valid open and close but invalid label , ignore lines within declaration
 * if valid open and label or missing label but no closing , ignore opening and label
 * if closing but no opening and label , ignore line
 * */

/*assuming just like we're told , every starting valid macro will have a closing macro(makes writing this code much easier), not checking inline comments, so there can be a starting macro and a label there and later on we'll find a closing macro without the starting declaration
 * we'll assume that's an error ,and we're missing a starting declaration since we cannot tell if the macro declaration in the inline comment was meant for that closing macro or not*/
/*assuming there won't be a macro label that matches a label for an instruction or directive later on*/
/*assuming no nested macro declarations , given*/
/*assuming there are no macro label duplicates , otherwise we can't expand the macros and have to stop immediately when we encounter a macro expansion call with a duplicate*/
/*PAGE 28 -WE CAN PROCEED TO SECOND AND LATER STAGES ONLY IF THE MACRO EXPANSION STAGE WENT SUCCESSFULLY , IF NOT THEN WE CAN STOP AFTER IT (FROM WHAT I UNDERSTAND IMPLICITLY)*/
/*there's conflicting info about the need to add to 'struct HashTable *reservedKeywords' endmcr and mcr , labels cannot be set as those names , assuming that won't happen*/
/*should we add data, string , extern and entry to the reserved table as well ? , I think we should, labels cannot contain '.' anyway so adding just the letters without the dot at the start will work*/
int parseMacroV2(int *createFiles ,FILE *fpSource, FILE *fpDest, const struct HashTable *reservedKeywords) {

    int result;
    long endOfLinePos[1];
    long currLineNum[1];

    char buffer[MAX_LINE + 1]; /* + 1 for null character */
    int currLinePos[1];
    struct HashTable *macroTable;

    /*creating char arrays here ,so we won't have to recreate arrays with every call and incur extra overhead for no reason*/
    char macroLabel[MAX_LABEL_LEN];
    char macroCall[MAX_LABEL_LEN];

    if((macroTable = createHashTable()) == NULL) {
        printf("\nERROR - cannot allocate memory for macroTable\n");
        return FALSE;
    }

    endOfLinePos[0] = 0;
    currLineNum[0] = 0;

    result = runParseMacro(createFiles, macroLabel, macroCall, endOfLinePos, currLineNum, currLinePos, buffer, fpSource, fpDest, macroTable, reservedKeywords);

    freeHashTable(macroTable, MACRO_DATATYPE);

    return result;
}
