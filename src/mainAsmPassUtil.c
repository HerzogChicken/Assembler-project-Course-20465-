#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sharedFunctions.h"
#include "hashTableDS.h"
#include "sharedEnums.h"


/* even though these functions play a role for the whole main assembler pass , they obscure away the flow of the parsing .
 * therefore I've decided on grouping and labeling them here , the main idea of the inner works of the main assembler pass
 * can be understood solely by the functions in 'mainAssemblerPass.c' thanks to meaningful (yet sometimes a little long)
 * function naming.
 *
 * 1. buffer functions                      [22 , 93]
 * 2. parsing utils & error functions       [95 , 230]
 * 3. string function utils                 [232 , 295]
 * 4. directive string util functions       [297 , 573]
 * 5. data instruction util functions       [575 , 664]
 * 6. string instruction util functions     [666 , 715]
 * */

/****************************************************************************************************************************************************************************************************************/
/******************************************************************************    BUFFER FUNCTIONS    **********************************************************************************************************/

int flushInputLine(FILE *fpExpanded) {

    int fileChar;

    while((fileChar = getc(fpExpanded)) != EOF && fileChar != '\n')
        ;

    return (fileChar == '\n') ? EMPTY_LINE : STOP;
}

int fillLineBufferMainPass(int *index, long currLineNumber, int *fileChar, char lineBuffer[], FILE *fpExpanded, int *createFiles ) {


    for(*index = 0 ; *index < MAX_LINE && (*fileChar = getc(fpExpanded)) != EOF ; ++(*index)) {

        if(*index == MAX_LINE - 1) {
            printf("\nERROR @ line %ld - input text overflow, lines cannot exceed 80 characters\n", currLineNumber);
            *createFiles = FALSE;
            return flushInputLine(fpExpanded);
        }else if(*fileChar > 127 || *fileChar < -127) {
            printf("\nERROR @ line %ld - input contains illegal character, character is not in ASCII code\n", currLineNumber);
            *createFiles = FALSE;
            return flushInputLine(fpExpanded);
        }

        lineBuffer[*index] = (char)*fileChar;
        if(*fileChar == '\n') {
            lineBuffer[*index + 1] = '\0';
            break;
        }

    }

    return SUCCESS;
}

int getNewLineMainAsmPass(long *currLineNumber, char lineBuffer[], FILE *fpExpanded, int *createFiles) {
    /*seems like getc() is usually a macro(both getc() and fgetc() can be functions)and helps with the incurring overhead by reading a chunk of data implicitly ,getc() may evaluate the argument more than once,but it's not a problem for us*/
    int fileChar[1];
    int i[1];
    int j, newLineBufferState;

    *i =  *fileChar = j = 0;                                                                                            /*not checking for ';' because we need to go over the line anyway to get to the next one */
    *currLineNumber += 1;                                                                                               /*since we don't know the length of it in order to skip to the next one*/

    newLineBufferState = fillLineBufferMainPass(i, *currLineNumber, fileChar, lineBuffer, fpExpanded, createFiles);
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
        return EMPTY_LINE;
    }else
        return SUCCESS;
}

/******************************************************************************    BUFFER FUNCTIONS    **********************************************************************************************************/
/****************************************************************************************************************************************************************************************************************/

/****************************************************************************************************************************************************************************************************************/
/******************************************************************************    PARSING UTIL & ERROR FUNCTIONS    ********************************************************************************************/

int isEmptyLine(char buffer[], int *currLinePos) {

    skipWhiteSpace(buffer, currLinePos);

    if(buffer[*currLinePos] == '\n')
        return TRUE;
    else
        return FALSE;
}

int isValidLabelV2(const char subString[], long currLineNumber) {

    int i;

    for(i = 0 ; subString[i] != '\0' ; ++i) {
        if(subString[i] == ':' && subString[i + 1] == '\0')
            return TRUE;
        else if(!isalnum(subString[i])) {
            printf("\nERROR @ line %ld - Invalid label, cannot start with numerical value\n", currLineNumber);
            return FALSE;
        }
    }

    return FALSE;
}

int isInstruction(const char string[], long currLineNumber) {

    if(strcmp(string, ".extern") == 0) {
        return EXTERN;
    }else if(strcmp(string, ".entry") == 0) {
        return ENTRY;
    }else if(strcmp(string, ".data") == 0) {
        return DATA;
    }else if(strcmp(string, ".string") == 0) {
        return STRING;
    }

    printf("\nERROR @ line %ld - unknown instruction\n", currLineNumber);
    return ERROR;
}


int determineSubStrErrorIllegalChar(const char subString[], long currLineNumber) {

    int i;

    for(i = 0 ; subString[i] != '\0' ; ++i) {
        if(ispunct(subString[i])) {
            printf("\nERROR @ line %ld - labels can only contain digits and alphanumerics\n", currLineNumber);
            return TRUE;
        }
    }

    return FALSE;
}

void determineSubStrErrorVariousChecks(const char buffer[], int currLinePos, long currLineNumber) {

    int i, containsAlnum, containsSpace;

    for(i = currLinePos ; isalnum(buffer[i]) || (buffer[i] != '\n' && isspace(buffer[i])) ; ++i) {

        if(isspace(buffer[i]))
            containsSpace = TRUE;
        else /*isalnum(buffer[i]) == TRUE*/
            containsAlnum = TRUE;

        if(buffer[i + 1] == ':') {

            if(containsSpace) {
                if(containsAlnum)
                    printf("\nERROR @ line %ld - label declaration cannot contain any white spaces within the label name or between the label and colon\n", currLineNumber);
                else
                    printf("\nERROR @ line %ld - cannot contain white space between the label and colon (':')\n", currLineNumber);/*changed from label declaration missing, only contains white space*/
            }else if(i >= MAX_LABEL_LEN - 1)
                printf("\nERROR @ line %ld - length of the label declaration exceeds the maximum length allowed\n", currLineNumber);

            return;
        }
    }

    if(buffer[i] == '.') {
        printf("\nERROR @ line %ld - label is not declared properly, missing a colon\n", currLineNumber);
    }else
        printf("\nERROR @ line %ld - label declaration is missing a colon (':')\n", currLineNumber);

    return;
}

void determineSubStrError(const char buffer[], const char subString[], int currLinePos, long currLineNumber) { /*a very simple detection (detects the first error that occurs without checking the rest), myopic*/

    if(strcmp(subString, "data") == 0 || strcmp(subString, "string") == 0 || strcmp(subString, "extern") == 0 || strcmp(subString, "entry") == 0) {
        printf("\nERROR @ line %ld - missing a dot ('.') before the instruction\n", currLineNumber);
        return;
    }

    if(determineSubStrErrorIllegalChar(subString, currLineNumber) == TRUE)
        return;

    determineSubStrErrorVariousChecks(buffer, currLinePos, currLineNumber);

    return;
}

int parseAndValidateSubStr(char buffer[], char subString[], int currLinePos, long currLineNumber, const struct HashTable *reservedKeywords) {

    if(subString[0] == '.')
        return isInstruction(subString, currLineNumber);
    else if(isdigit(subString[0])) {
        printf("\nERROR @ line %ld - Instruction \\ directive cannot start with a numerical value\n", currLineNumber);
        return ERROR;
    }else if(buffer[currLinePos - 1] == ':') {
        return (isValidLabelV2(subString, currLineNumber)) ? LABEL : ERROR;
    }else if(ispunct(buffer[currLinePos - 1])) {
        if(buffer[currLinePos - 1] == ',')
            printf("\nERROR @ line %ld - Extraneous comma right after instruction \\ directive name\n", currLineNumber); /*if we want to add more info about the instruction or directive we need to search in res table and check if it is instruction by simply comparing  the sub string with the instruction names*/
        else
            printf("\nERROR @ line %ld - Invalid symbol at the end of instruction \\ directive\n", currLineNumber);
        return ERROR;
    }else {

        if(hashIsReservedKeyWord(reservedKeywords, subString))
            return DIRECTIVE;
        else {
            determineSubStrError(buffer, subString, currLinePos, currLineNumber);
            return ERROR;
        }
    }
}

/******************************************************************************    PARSING UTIL & ERROR FUNCTIONS    ********************************************************************************************/
/****************************************************************************************************************************************************************************************************************/

/****************************************************************************************************************************************************************************************************************/
/******************************************************************************    STRING FUNCTION UTILS   ******************************************************************************************************/

/*not defined, do we need space between the colon marking the end of a label and the rest of the text or not ? (assuming we don't by adding a break if ':' in the loop)*/
/*if we need space then I need to add more detections for errors and more precise errors for label declarations */
int getSubStr(char buffer[], char subString[], int *currLinePos, long currLineNumber) {

    int i;

    skipWhiteSpace(buffer,currLinePos);

    for( i = 0 ; !isspace(buffer[*currLinePos]) ; i++ , (*currLinePos)++) {
        if(i >= MAX_LABEL_LEN - 1) {
            printf("\nERROR @ line %ld - label length is over the maximum allowed\n", currLineNumber);
            return ERROR;
        }
        subString[i] = buffer[*currLinePos];
        if(buffer[*currLinePos] == ':') {
            (*currLinePos)++;
            i += 1;
            break;
        }
    }

    subString[i] = '\0';
    return SUCCESS;
}


int validateEndOfLine(const char buffer[], int *currLinePos, long currLineNumber) {

    while(TRUE) { /*We know for sure this won't end up as an infinite loop since 'buffer[]' is of limited size*/

        if(buffer[*currLinePos] == '\n' || buffer[*currLinePos] == '\0') {
            return SUCCESS;
        }else if(!isspace(buffer[*currLinePos])) {
            printf("\nERROR @ line %ld - extraneous text\n", currLineNumber);
            return ERROR;
        }else
            *currLinePos += 1;

    }

    return STOP;
}

int getToSecondOperand(const char buffer[], int *currLinePos, long currLineNumber) {

    skipWhiteSpace(buffer, currLinePos);

    if(buffer[*currLinePos] == '\n' ) {
        printf("\nERROR @ line %ld - missing second operand\n", currLineNumber);
        return ERROR;
    }else if(buffer[*currLinePos] != ',') {
        printf("\nERROR @ line %ld - missing comma between operands\n", currLineNumber);
        return ERROR;
    }else {
        *currLinePos += 1;
        return SUCCESS;
    }
}

/******************************************************************************    STRING FUNCTION UTILS   ******************************************************************************************************/
/****************************************************************************************************************************************************************************************************************/

/****************************************************************************************************************************************************************************************************************/
/******************************************************************************    DIRECTIVE STRING UTIL FUNCTIONS    *******************************************************************************************/

int inRange(int val) {
    return ( (MINIMUM <= val) && (val <= MAXIMUM) );
}

int aToI(const char operandStr[], int sign, long currLineNumber) {

    int i;
    int result;

    result = 0;

    for(i = 0 ; operandStr[i] != '\0' ; ++i)
        result = 10 * result + (operandStr[i] - '0');

    if(inRange(result * sign))
        return (result * sign);
    else {
        printf("\nERROR @ line %ld - parameter value out of range\n", currLineNumber);
        return ERROR;
    }
}

int getImmediateOperandVal(const char buffer[], int sign, int *currLinePos, long currLineNumber) {

    int i;
    char operandStr[5];

    i = 0;
    operandStr[4] = '\0';

    for( ; buffer[*currLinePos] != '\n' ; i++, (*currLinePos)++) {

        if(buffer[*currLinePos] == ',' || isspace(buffer[*currLinePos]) || buffer[*currLinePos] == ')')
            break;
        else if(isdigit(buffer[*currLinePos]))
            if(i >= 4) {
                printf("\nERROR @ line %ld - Invalid operand , value out of range\n", currLineNumber);
                return ERROR;
            }else
                operandStr[i] = buffer[*currLinePos];
        else {
            printf("\nERROR @ line %ld - Invalid operand\n", currLineNumber);
            return ERROR;
        }

    }

    operandStr[i] = '\0';
    return aToI(operandStr, sign, currLineNumber);
}

int getImmediateOperand(const char buffer[], int *currLinePos, long currLineNumber) {

    int sign;
    sign = 1;

    if(buffer[*currLinePos] == '-') {
        *currLinePos += 1;
        sign = -1;
    }else if(buffer[*currLinePos] == '+')
        *currLinePos += 1;

    return getImmediateOperandVal(buffer, sign, currLinePos, currLineNumber);
}

int copyLabelStr(const char buffer[], int *currLinePos, char label[], long currLineNumber) { /*don't need to check if label is missing and worry about returning an empty string since every function that calls this one verifies that it's not the case before calling*/

    int i;
    i = 0;

    while(buffer[*currLinePos] != '\n' && isalnum(buffer[*currLinePos])) {
        if(i >= MAX_LABEL_LEN - 1) {
            printf("\nERROR @ line %ld - label length is over the maximum allowed\n", currLineNumber);
            return ERROR;
        }

        label[i++] = buffer[(*currLinePos)++];
    }

    label[i] = '\0';
    return SUCCESS;
}

int validateJumpDestLabel(const char buffer[], int currLinePos, long currLineNumber) {

    if(isdigit(buffer[currLinePos])) {
        printf("\nERROR @ line %ld  - Invalid label, label cannot start with a numerical value\n", currLineNumber);
        return ERROR;
    }else if(buffer[currLinePos] == '\n') {
        printf("\nERROR @ line %ld - missing jump destination label\n", currLineNumber);
        return ERROR;
    }else if(!isalpha(buffer[currLinePos])) {
        printf("\nERROR @ line %ld - invalid jump destination label\n", currLineNumber);
        return ERROR;
    }

    return SUCCESS;
}

int getValidJumpDestLabel(const char buffer[], int *currLinePos, long currLineNumber, char labelCopyStr[], const struct HashTable *reservedKeywords) {

    skipWhiteSpace(buffer, currLinePos);

    if(validateJumpDestLabel(buffer, *currLinePos, currLineNumber) == ERROR)
        return ERROR;

    if(copyLabelStr(buffer, currLinePos, labelCopyStr, currLineNumber) == ERROR)
        return ERROR;

    if(!hashIsReservedKeyWord(reservedKeywords, labelCopyStr))
        return SUCCESS;
    else
        return ERROR;
}

int getTempOperandStr(const char buffer[], int currLinePos, long currLineNumber, char tempStr[]) {

    int subStrLength;

    if(isdigit(buffer[currLinePos])) {
        printf("\nERROR @ line %ld - Invalid label\\directive\n", currLineNumber);
        return ERROR;
    }

    for(subStrLength = 0 ; buffer[currLinePos] != '\n' ; currLinePos++ , subStrLength++) {
        if(isalnum(buffer[currLinePos])) {

            if(subStrLength >= MAX_LABEL_LEN - 1) {
                printf("\nERROR @ line %ld - operand length is over the maximum valid length\n", currLineNumber);
                return ERROR;
            }else
                tempStr[subStrLength] = buffer[currLinePos];

        }else if(isspace(buffer[currLinePos]) || buffer[currLinePos] == ',' || buffer[currLinePos] == ')')
            break;
        else {
            printf("\nERROR @ line %ld - invalid operand\n", currLineNumber);
            return ERROR;
        }
    }
    tempStr[subStrLength] = '\0';

    return subStrLength;
}

int isDirectOrRegister(const char buffer[], int currLinePos, long currLineNumber, const struct HashTable *reservedKeywords) {

    char tempStr[MAX_LABEL_LEN];
    int subStrLength;

    if((subStrLength = getTempOperandStr(buffer, currLinePos, currLineNumber, tempStr)) == ERROR)
        return ERROR;

    if(subStrLength == 2) {
        if(buffer[currLinePos] == 'r' && buffer[currLinePos + 1] != '8' && buffer[currLinePos + 1] != '9' && isdigit(buffer[currLinePos + 1]))
            return DIRECT_REGISTER;
    }

    if(hashIsReservedKeyWord(reservedKeywords, tempStr)) {
        printf("\nERROR @ line %ld - a directive cannot be an operand\n", currLineNumber);
        return ERROR;
    }

    return DIRECT;
}

int determineOperandAddressing(const char buffer[], int *currLinePos, int operandState, long currLineNumber, const struct HashTable *reservedKeywords) {

    skipWhiteSpace(buffer, currLinePos);

    if(buffer[*currLinePos] == '#') {
        *currLinePos += 1;
        return IMMEDIATE;
    }else if(isalpha(buffer[*currLinePos])) {
        return isDirectOrRegister(buffer, *currLinePos, currLineNumber, reservedKeywords);
    }else {
        if(buffer[*currLinePos] == '\n')
            printf("\nERROR @ line %ld - missing operand\n", currLineNumber);
        else if(buffer[*currLinePos] == ',') /*this can trigger as well in cases where we have a comma before the operand inside the parenthesis , "Extraneous comma" is not exactly right ,but it will do */
            printf("\nERROR @ line %ld - Extraneous comma\n", currLineNumber);
        else
            printf("\nERROR @ line %ld - Invalid %s operand\n", currLineNumber, (operandState == SOURCE) ? "source" : "destination");

        return ERROR;
    }
}

int isValidJumpNoOperands(const char buffer[], int currLinePos, long currLineNumber) {

    while(buffer[currLinePos] != '\n') {

        if(!isspace(buffer[currLinePos])) {
            if(buffer[currLinePos] == '(')
                printf("\nERROR @ line %ld - extraneous whitespace between jump destination label and opening parenthesis\n", currLineNumber);
            else
                printf("\nERROR @ line %ld - extraneous text after jump destination label\n", currLineNumber);

            return FALSE;
        }

        currLinePos += 1;
    }

    return TRUE;
}

int isValidateEndOfJumpLine(const char buffer[], int currLinePos, long currLineNumber) {

    while(buffer[currLinePos] != '\n' ) {

        if(!isspace(buffer[currLinePos])) {
            printf("\nERROR @ line %ld - extraneous text after closing parenthesis\n", currLineNumber);
            return FALSE;
        }else
            currLinePos += 1;
    }

    return TRUE;
}

int isValidJumpWithOperands(const char buffer[], int currLinePos, long currLineNumber) {

    int commaCount;

    commaCount = 0;

    while(buffer[currLinePos] != ')' && buffer[currLinePos] != '\n') {

        if(isspace(buffer[currLinePos])) {
            printf("\nERROR @ line %ld - cannot contain whitespace within parenthesis\n", currLineNumber);
            return FALSE;
        }else if(buffer[currLinePos] == ',') {
            commaCount += 1;
        }

        currLinePos += 1;
    }

    if(buffer[currLinePos] == '\n') {
        printf("\nERROR @ line %ld - missing closing parenthesis\n", currLineNumber);
        if(commaCount > 1)
            printf("\nERROR @ line %ld - extraneous comma\n", currLineNumber);

        return FALSE;
    }else if(commaCount > 1) {
        printf("\nERROR @ line %ld - extraneous comma\n", currLineNumber);
        return FALSE;
    }

    return isValidateEndOfJumpLine(buffer, currLinePos + 1, currLineNumber); /* +1 since the current char is ')' */
}

int determineJumpAddressingType(const char buffer[], int currLinePos, long currLineNumber) {

    if (buffer[currLinePos] == '\n')
        return JUMP_NO_OPER;
    else if (isspace(buffer[currLinePos])) {
        if (isValidJumpNoOperands(buffer, currLinePos, currLineNumber))
            return JUMP_NO_OPER;
        else
            return ERROR;
    }else if(buffer[currLinePos] == '(') {
        if (isValidJumpWithOperands(buffer, currLinePos, currLineNumber))
            return JUMP_WITH_OPER;
        else
            return ERROR;
    }else {
        printf("\nERROR @ line %ld - missing opening parenthesis\n", currLineNumber);
        return ERROR;
    }
}

/******************************************************************************    DIRECTIVE STRING UTIL FUNCTIONS    *******************************************************************************************/
/****************************************************************************************************************************************************************************************************************/

/****************************************************************************************************************************************************************************************************************/
/******************************************************************************    DATA INSTRUCTION  UTIL FUNCTIONS    ******************************************************************************************/

int isValidTextBeforeNum(const char buffer[], int currLinePos, long currLineNumber) {

    if(buffer[currLinePos] == '-' || buffer[currLinePos] == '+' || isdigit(buffer[currLinePos]))
        return TRUE;
    else {
        if(buffer[currLinePos] == ',')
            printf("\nERROR @ line %ld - extraneous comma before first value\n", currLineNumber);
        else if(isalpha(buffer[currLinePos]))
            printf("\nERROR @ line %ld - invalid char, data instruction operands cannot contain letters\n", currLineNumber);
        else if(buffer[currLinePos] == '\n')
            printf("\nERROR @ line %ld - missing data\n", currLineNumber);
        else
            printf("\nERROR @ line %ld - invalid symbol\n", currLineNumber);


        return FALSE;
    }
}

void getToFirstNumber(char buffer[], int *currLinePos) {
    skipWhiteSpace(buffer, currLinePos);
}

int isEndOfLine(const char buffer[], const int *currLinePos) {
    if(buffer[*currLinePos] == '\n')
        return TRUE;
    else
        return FALSE;
}

int getToNextNumber(char buffer[], int *currLinePos, long currLineNumber) {
    skipWhiteSpace(buffer, currLinePos);

    if(isEndOfLine(buffer, currLinePos))
        return STOP;
    else if(buffer[*currLinePos] != ',') {
        printf("\nERROR @ line %ld - missing comma\n", currLineNumber);
        return ERROR;
    }else
        *currLinePos += 1;

    skipWhiteSpace(buffer, currLinePos);

    if(isEndOfLine(buffer, currLinePos)) {
        printf("\nERROR @ line %ld - extraneous comma after the last number\n", currLineNumber);
        return ERROR;
    }else if(buffer[*currLinePos] == ',') {
        printf("\nERROR @ line %ld - missing value between commas\n", currLineNumber);
        return ERROR;
    }else
        return SUCCESS;
}

int getNumberSign(char sign, int *currLinePos) {
    if(sign == '-') {
        *currLinePos += 1;
        return -1;
    }else if(sign == '+')
        *currLinePos += 1;

    return 1;
}

int dataInstructionNumValidation(const char buffer[], const int *currLinePos, int number, long currLineNumber) {

    if(buffer[*currLinePos] == '.') {
        printf("\nERROR @ line %ld - invalid value , value is not an integers\n", currLineNumber);/*1.0 is considered as a float*/
        return ERROR;
    }else if(isEndOfLine(buffer, currLinePos)) {
        if(inRange(number))
            return STOP;
        else
            return ERROR;
    }else if(buffer[*currLinePos] == ',' || isspace(buffer[*currLinePos]))
        return SUCCESS;
    else {
        if(buffer[*currLinePos] == '+' || buffer[*currLinePos] == '-')
            printf("\nERROR @ line %ld - extraneous %c\n", currLineNumber, buffer[*currLinePos]);
        else
            printf("\nERROR @ line %ld - invalid value\n", currLineNumber);

        return ERROR;
    }
}

/******************************************************************************    DATA INSTRUCTION  UTIL FUNCTIONS    ******************************************************************************************/
/****************************************************************************************************************************************************************************************************************/

/****************************************************************************************************************************************************************************************************************/
/******************************************************************************    STRING INSTRUCTION  UTIL FUNCTIONS    ****************************************************************************************/

/*go to the end of the array , then go backwards , so we start from the end of the array and move backwards until we hit a non whitespace character, calculate the difference*/
int isValidEndOfString(const char buffer[], int currLinePos, int length[], long currLineNumber) {

    int i;                                                                                                              /*since we don't know the intentions of the user we cannot predict in situations like ".string "abc"[text]  " */
    /* if it's extraneous text or missing closing quotation marks*/
    for(i = currLinePos ; buffer[i] != '\n'; ++i)                                                                       /* (can't say the quotation marks are not enveloping the string */
        ;                                                                                                               /* as we can have way more complex text that we won't be able to determine that for sure) */

    for(i = i - 1 ; i - 1 >= currLinePos && isspace(buffer[i]) ; --i)
        ;

    if(i == currLinePos || buffer[i] != '"') {
        printf("\nERROR @ line %ld - missing ending double quotation marks\n", currLineNumber);
        return FALSE;
    }else {
        length[0] = i - currLinePos;
        return TRUE;
    }
}

int isValidStartOfString(const char buffer[], int currLinePos, long currLineNumber) {

    if(buffer[currLinePos] == '"')
        return TRUE;
    else {
        if(buffer[currLinePos] == '\n' )
            printf("\nERROR @ line %ld - empty string instruction\n", currLineNumber);
        else
            printf("\nERROR @ line %ld - missing starting double quotation marks\n", currLineNumber);

        return FALSE;
    }
}

int isValidStringInstruction(char buffer[], int *currLinePos, long currLineNumber, int length[]) {

    skipWhiteSpace(buffer, currLinePos);
    if(!isValidStartOfString(buffer, *currLinePos, currLineNumber) || !isValidEndOfString(buffer, *currLinePos, length, currLineNumber))
        return FALSE;
    else {
        *currLinePos += 1;
        return TRUE;
    }
}

/******************************************************************************    STRING INSTRUCTION  UTIL FUNCTIONS    ****************************************************************************************/
/****************************************************************************************************************************************************************************************************************/