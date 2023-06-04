#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "queueDS.h"
#include "sharedFunctions.h"
#include "hashTableDS.h"
#include "sharedEnums.h"
#include "mainAsmPassUtil.h"


/***************************************************************************************************** DIRECTIVES ******************************************************************/
int getArity(int opCode) {

    if(opCode <= SUB_P || opCode == LEA_P)
        return BINARY;
    else if(opCode == JMP_P || opCode == BNE_P || opCode == JSR_P)
        return JMP_CAPABLE;
    else if(opCode < RTS_P )
        return UNARY;
    else /*then it is either rts or stop*/
        return NULLARY;
}

void updateJumpWithOperBinCode(int addressing, int state, struct Qnode *directiveNode) {

    if(addressing == DIRECT)
        (state == SOURCE) ? addToBinaryCode(directiveNode, (1 << 12)) : addToBinaryCode(directiveNode, (1 << 10));
    else if(addressing == DIRECT_REGISTER)
        (state == SOURCE) ? addToBinaryCode(directiveNode, (3 << 12)) : addToBinaryCode(directiveNode, (3 << 10));
    /*else , addressing == IMMEDIATE which is 00 , therefore we do not include it*/
}

int registerToBinaryCode(int registerNum, int state) {

    if(registerNum == 1 || registerNum == 2 || registerNum == 4)
        return (state == SOURCE) ? (1 << ((registerNum/2) + 8)) : (1 << ((registerNum/2) + 2));
    else if(registerNum == 3)
        return (state == SOURCE) ? (1 << 9 | 1 << 8) : (1 << 3 | 1 << 2);
    else if(registerNum == 5)
        return (state == SOURCE) ? (1 << 10 | 1 << 8) : (1 << 4 | 1 << 2);
    else if(registerNum == 6)
        return (state == SOURCE) ? (1 << 10 | 1 << 9) : (1 << 4 | 1 << 3);
    else /*registerNum == 7*/
        return (state == SOURCE) ? (1 << 10 | 1 << 9 | 1 << 8) : (1 << 4 | 1 << 3 | 1 << 2);
}

int getRegisterNum(const char buffer[], int *currLinePos) { /*not doing any validation since it was already done in the determineAddressing function*/

    int number;

    *currLinePos += 1;
    number = (buffer[*currLinePos] - '0');
    *currLinePos += 1;                                                                      /*INCREASING THE currLinePos by 2 ! so we go to the next value in the buffer*/

    return number;
}

void updateQnodeBinCode(struct Qnode *node, int addressing, int state) {

    if(addressing == DIRECT)
        (state == SOURCE) ? addToBinaryCode(node, 1 << 4) : addToBinaryCode(node, 1 << 2);
    else if(addressing == JUMP)
        (state == SOURCE) ? addToBinaryCode(node, 1 << 5) : addToBinaryCode(node, 1 << 3);
    else
        (state == SOURCE) ? addToBinaryCode(node, (1 << 5 | 1 << 4)) : addToBinaryCode(node, (1 << 3 | 1 << 2));
}

int isCompatibleAddressSource(int opCode, int addressing) {

    if(opCode == MOV_P || opCode == CMP_P || opCode == ADD_P || opCode == SUB_P)
        return TRUE;
    else if(opCode == LEA_P && addressing == DIRECT) /*again , the table on page 32 is incorrect , from the description , the first operand "source" can only be a symbol*/
        return TRUE;
    else
        return FALSE;
}

int isCompatibleAddressDest(int opCode, int addressing) {

    if(opCode == CMP_P || opCode == PRN_P)
        return TRUE;
    else if(addressing != IMMEDIATE) /*'rts' and 'stop' won't reach this function so , we're only worried about the other directives left which are not compatible with immediate addressing*/
        return TRUE;
    else
        return FALSE;
}

int isCompatibleAddressing(int operandState, int opCode, int addressing) {

    if(opCode == JMP_P || opCode == BNE_P || opCode == JSR_P) /*from the examples in page 19 we can conclude that the table on page 32 is incorrect and these 3 directives can work with any addressing 0-3*/
        return TRUE;
    else if(operandState == SOURCE)
        return isCompatibleAddressSource(opCode, addressing);
    else
        return isCompatibleAddressDest(opCode, addressing);
}

int addImmediateOperand(char buffer[], int *currLinePos, long currLineNumber, int *directiveCounter, struct Queue *directiveQ) {

    int operandValue;

    operandValue = getImmediateOperand(buffer, currLinePos, currLineNumber);

    if(operandValue == ERROR)
        return ERROR;
    else {                                                                              /*shifting the operandValue by 2 to the left, from the examples it seems like*/
        enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, directiveQ, "Immediate", 0, *directiveCounter, (operandValue << 2), (COMPLETED_STATE | VALUE_STATE));
        *directiveCounter += 1;                                                                                                                                         /*we leave the A,R,E bits open , when it is a data instruction then we leave the numbers as they are*/
        return SUCCESS;
    }
}

int addDirectOperand(char buffer[], int *currLinePos, long currLineNumber, int *directiveCounter, int operandState, int jumpWithOperand, struct Queue *directiveQ, struct Qnode *directiveNode, const struct HashTable *reservedKeywords) {

    char labelCopy[MAX_LABEL_LEN];

    if(copyLabelStr(buffer, currLinePos, labelCopy, currLineNumber) == ERROR)
        return ERROR;

    if(hashIsReservedKeyWord(reservedKeywords, labelCopy))
        return ERROR;

    enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, directiveQ, labelCopy, getKey(labelCopy), *directiveCounter, 0, NOT_COMPLETE_STATE);
    *directiveCounter += 1;

    if(jumpWithOperand == TRUE)
        updateJumpWithOperBinCode(DIRECT, operandState, directiveNode);
    else/*after going over the example again , seems like in case of a jump directive we only update the last 10-13 bits and set the source destination bits 3-4 as 2 and  4-5 bits are zero since it's a unary directive */
        updateQnodeBinCode(directiveNode, DIRECT, operandState);

    return SUCCESS;
}

int multipleRegisterOperand(int operandState, int registerNum, int jumpWithOperand, struct Queue *directiveQ, struct Qnode *directiveNode) {

    if(registerNum != 0)
        updateTailBinaryCode(directiveQ, registerToBinaryCode(registerNum, operandState));

    if(jumpWithOperand == TRUE)
        updateJumpWithOperBinCode(DIRECT_REGISTER, operandState, directiveNode);

    return SUCCESS;
}

int singleRegisterOperand(int *directiveCounter, int operandState, int registerNum, int opRegisterCount[], int jumpWithOperand, struct Queue *directiveQ, struct Qnode *directiveNode) {

    if(registerNum == 0)
        enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, directiveQ, "register", 0, *directiveCounter, 0, (COMPLETED_STATE | VALUE_STATE));
    else
        enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, directiveQ, "register", 0, *directiveCounter, registerToBinaryCode(registerNum, operandState), (COMPLETED_STATE | VALUE_STATE));


    if(jumpWithOperand == TRUE)
        updateJumpWithOperBinCode(DIRECT_REGISTER, operandState, directiveNode);

    opRegisterCount[0] = 1;
    *directiveCounter += 1;

    return SUCCESS;
}

int addDirectRegisterOperand(const char buffer[], int *currLinePos, int *directiveCounter, int operandState, int opRegisterCount[], int jumpWithOperand, struct Queue *directiveQ, struct Qnode *directiveNode) {

    int registerNum;

    registerNum = getRegisterNum(buffer, currLinePos);
    if(!jumpWithOperand) /*we only want to update the BinCode in bits 3-5 only if it is not a jump directive*/
        updateQnodeBinCode(directiveNode, DIRECT_REGISTER, operandState);

    if(opRegisterCount[0] == 0)
        return singleRegisterOperand(directiveCounter, operandState, registerNum, opRegisterCount, jumpWithOperand, directiveQ, directiveNode);
    else
        return multipleRegisterOperand(operandState, registerNum, jumpWithOperand, directiveQ, directiveNode);
}

int addOperand(char buffer[], int *currLinePos, long currLineNumber, int opCode, int operandState, int opRegisterCount[], int *directiveCounter, int jumpWithOperand, struct Queue *directiveQ, struct Qnode *directiveNode, const struct HashTable *reservedKeywords) {

    int opAddressingType;

    opAddressingType = determineOperandAddressing(buffer, currLinePos, operandState, currLineNumber, reservedKeywords);
    if(opAddressingType == ERROR)
        return ERROR;

    if(!isCompatibleAddressing(operandState, opCode, opAddressingType)) {
        printf("\nERROR @ line %ld - %s operand with incompatible addressing type\n", currLineNumber , (operandState == SOURCE) ? "source" : "destination");
        return ERROR;
    }

    switch(opAddressingType) {
        case IMMEDIATE:
            return addImmediateOperand(buffer, currLinePos, currLineNumber, directiveCounter, directiveQ);
        case DIRECT:
            return addDirectOperand(buffer, currLinePos, currLineNumber, directiveCounter, operandState, jumpWithOperand, directiveQ, directiveNode, reservedKeywords);
        case DIRECT_REGISTER:
            return addDirectRegisterOperand(buffer, currLinePos, directiveCounter, operandState, opRegisterCount, jumpWithOperand, directiveQ, directiveNode);
        default:
            printf("\nFATAL @ line %ld - function addOperand - reached default\n", currLineNumber);
            return ERROR;
    }
}

int binaryDirective(char buffer[], int *currLinePos, long currLineNumber, int opCode, int opRegisterCount[], int *directiveCounter, int jumpWithOperand, struct Queue *directiveQ, struct Qnode *directiveNode, const struct HashTable *reservedKeywords) {

    int result;

    result = addOperand(buffer, currLinePos, currLineNumber, opCode, SOURCE, opRegisterCount, directiveCounter, jumpWithOperand, directiveQ, directiveNode, reservedKeywords);
    if(result == ERROR)
        return ERROR;

    if(getToSecondOperand(buffer, currLinePos, currLineNumber) == ERROR)
        return ERROR;

    result = addOperand(buffer, currLinePos, currLineNumber, opCode, DESTINATION, opRegisterCount, directiveCounter, jumpWithOperand, directiveQ, directiveNode, reservedKeywords);
    if(result == ERROR)
        return ERROR;

    return SUCCESS;
}

int processBinaryDirective(char buffer[], int *currLinePos, long currLineNumber, int opCode, int opRegisterCount[], int *directiveCounter, int jumpWithOperand, struct Queue *directiveQ, struct Qnode *directiveNode, const struct HashTable *reservedKeywords) {

    int result;

    result = binaryDirective(buffer, currLinePos, currLineNumber, opCode, opRegisterCount, directiveCounter, jumpWithOperand, directiveQ, directiveNode, reservedKeywords);
    if(result == SUCCESS)
        return validateEndOfLine(buffer, currLinePos, currLineNumber);
    else
        return ERROR;
}

int processJumpNoOperands(char labelCopyStr[], int *directiveCounter, struct Queue *directiveQ, struct Qnode *directiveNode) {

    addToBinaryCode(directiveNode, (1 << 2)); /*set as 1 in bits 2-3 for direct destination*/
    enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, directiveQ, labelCopyStr, getKey(labelCopyStr), *directiveCounter, 0, NOT_COMPLETE_STATE);
    *directiveCounter += 1;

    return SUCCESS;
}

int processJumpWithOperands(char buffer[], int *currLinePos, long currLineNumber, int opCode, int opRegisterCount[], char labelCopyStr[], int *directiveCounter, struct Queue *directiveQ, struct Qnode *directiveNode, const struct HashTable *reservedKeywords) {

    int result;

    enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, directiveQ, labelCopyStr, getKey(labelCopyStr), *directiveCounter, 0, NOT_COMPLETE_STATE);
    *directiveCounter += 1;
    *currLinePos += 1; /*we move to the character that is right after the last character of the label/symbol , we validated the template in function determineJumpAddressingType ,so now we increment by 1 to skip the opening parenthesis */

    addToBinaryCode(directiveNode, (1 << 3)); /*since a jump directive has bits 2-3 set as 2  and bits 4-5 set as zero, if we have operands , then it is set ast 2 in bits 2-3 , otherwise, we have only a label so bits 2-3 are set as 1 as that's the number of direct destination*/
    result = binaryDirective(buffer, currLinePos, currLineNumber, opCode, opRegisterCount, directiveCounter, TRUE, directiveQ, directiveNode, reservedKeywords);
    if(result == ERROR)
        return ERROR;
    else {
        *currLinePos += 1; /*we increment again in order to skip the closing parenthesis, we know the template is valid thanks to determineJumpAddressingType*/
        return SUCCESS;
    }
}

int processSpecialJumpDirective(char buffer[], int *currLinePos, long currLineNumber, int opCode, int opRegisterCount[], int *directiveCounter, struct Queue *directiveQ, struct Qnode *directiveNode, const struct HashTable *reservedKeywords) {

    int result;
    char labelCopyStr[MAX_LABEL_LEN];

    if(getValidJumpDestLabel(buffer, currLinePos, currLineNumber, labelCopyStr, reservedKeywords) == ERROR)
        return ERROR;

    result = determineJumpAddressingType(buffer, *currLinePos, currLineNumber);
    if(result == JUMP_NO_OPER) {
        return processJumpNoOperands(labelCopyStr, directiveCounter, directiveQ, directiveNode);
    }else if(result == JUMP_WITH_OPER) {
        return processJumpWithOperands(buffer, currLinePos, currLineNumber, opCode, opRegisterCount, labelCopyStr, directiveCounter, directiveQ, directiveNode, reservedKeywords);
    }else
        return ERROR;
}

int processUnaryDirective(char buffer[], int *currLinePos, long currLineNumber, int opCode, int opRegisterCount[], int *directiveCounter, int jumpWithOperand, struct Queue *directiveQ, struct Qnode *directiveNode, const struct HashTable *reservedKeywords) {

    int result;

    result = addOperand(buffer, currLinePos, currLineNumber, opCode, DESTINATION, opRegisterCount, directiveCounter, jumpWithOperand, directiveQ, directiveNode, reservedKeywords);
    if(result == ERROR)
        return ERROR;
    else
        return validateEndOfLine(buffer, currLinePos, currLineNumber);
}

int processNullaryDirective(char buffer[], int *currLinePos, long currLineNumber) {
    return validateEndOfLine(buffer, currLinePos, currLineNumber);
}

int callCorrectDirective(char buffer[], int *currLinePos, long currLineNumber, int opCode, int opRegisterCount[], int *directiveCounter, struct Queue *directiveQ, struct Qnode *directiveNode, const struct HashTable *reservedKeywords) {

    switch(getArity(opCode)) {
        case JMP_CAPABLE:
            return processSpecialJumpDirective(buffer, currLinePos, currLineNumber, opCode, opRegisterCount, directiveCounter, directiveQ, directiveNode, reservedKeywords);
        case BINARY:
            return processBinaryDirective(buffer, currLinePos, currLineNumber, opCode, opRegisterCount, directiveCounter, FALSE, directiveQ, directiveNode, reservedKeywords);
        case UNARY:
            return processUnaryDirective(buffer, currLinePos, currLineNumber, opCode, opRegisterCount, directiveCounter, FALSE, directiveQ, directiveNode, reservedKeywords);
        case NULLARY:
            return processNullaryDirective(buffer, currLinePos, currLineNumber);
        default:
            printf("\nFATAL @ line %ld - function processDirective - reached default\n", currLineNumber);
            return ERROR;
    }
}

int processDirective(char buffer[], int *currLinePos, long currLineNumber, const char directive[], int *directiveCounter, struct Queue *directiveQ, const struct HashTable *reservedKeywords) {

    int opCode;
    int opRegisterCount[1];
    struct Qnode *directiveNode;

    opCode = hashGetOpCode(reservedKeywords, directive);
    opRegisterCount[0] = 0;
    enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, directiveQ, " starting directive node ", 0, *directiveCounter, (opCode << 6), (DIRECTIVE_STATE | COMPLETED_STATE));
    *directiveCounter += 1;/*accounting for the directive itself and increasing for the next enqueue*/
    directiveNode = getCurrentNode(directiveQ);

    return callCorrectDirective(buffer, currLinePos, currLineNumber, opCode, opRegisterCount, directiveCounter, directiveQ, directiveNode, reservedKeywords);
}

/***************************************************************************************************** DIRECTIVES ******************************************************************/
/***************************************************************************************************** DATA INSTRUCTIONS ***********************************************************/
int getDataInstructionNumber(char buffer[], int *currLinePos, int number[], long currLineNumber) {

    int i;
    int sign;

    sign = getNumberSign(buffer[*currLinePos], currLinePos);

    for(i = 0 ; isdigit(buffer[*currLinePos]) ; i++ , (*currLinePos)++) {

        if(i >= 4) {
            printf("\nERROR @ line %ld - value out of range\n", currLineNumber);
            return ERROR;
        }else
            *number = (*number) * 10 + (buffer[*currLinePos] - '0');
    }

    *number *= sign;
    return dataInstructionNumValidation(buffer, currLinePos, *number, currLineNumber);
}

void addDataNumberToQueue(int number, int *instructionCounter, struct Queue *instructionQ) {

    char tempItoAStr[6]; /*for visual debugging and checking*/
    int cpyNum;
    int i;
    i = 0;
    cpyNum = (number < 0) ? (-number) : number;

    while(cpyNum != 0) {
        tempItoAStr[i++] = (char)((cpyNum % 10) + '0');
        cpyNum /= 10;
    }
    if(number < 0)
        tempItoAStr[i++] = '-';

    tempItoAStr[i] = '\0';

    enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, instructionQ, tempItoAStr, 0, *instructionCounter, number, (VALUE_STATE | COMPLETED_STATE));
    *instructionCounter += 1;

    return;
}

int addDataInstructionNumbers(char buffer[], int *currLinePos, long currLineNumber,  int *instructionCounter, struct Queue *instructionQ) {

    int number[1];
    int result;

    number[0] = 0;

    while((result = getDataInstructionNumber(buffer, currLinePos, number, currLineNumber)) == SUCCESS) { /* 3 possible return values , STOP/ERROR/SUCCESS */

        addDataNumberToQueue(number[0], instructionCounter, instructionQ);

        result = getToNextNumber(buffer, currLinePos, currLineNumber);

        if(result != SUCCESS || !isValidTextBeforeNum(buffer, *currLinePos, currLineNumber))
            return result;
        else
            number[0] = 0;
    }

    if(result == ERROR)
        return ERROR;
    else {
        addDataNumberToQueue(number[0], instructionCounter, instructionQ);
        return STOP;
    }
}

int processDataInstruction(char buffer[], int *currLinePos, long currLineNumber, int *instructionCounter, struct Queue *instructionQ) {

    int result;

    getToFirstNumber(buffer, currLinePos);
    if(!isValidTextBeforeNum(buffer, *currLinePos, currLineNumber))/* if data has a comma but no values e.g ".data , "  right now we will return that there's an extra comma which is right , it is possible to also check if it doesn't contain anything */
        return ERROR;

    result = addDataInstructionNumbers(buffer, currLinePos, currLineNumber, instructionCounter, instructionQ);
    return (result == ERROR) ? ERROR : SUCCESS;
}

/***************************************************************************************************** DATA INSTRUCTIONS ***********************************************************/
/***************************************************************************************************** STRING INSTRUCTIONS *********************************************************/
int addStringToQueue(char buffer[], int *currLinePos, int length, int *instructionCounter, struct Queue *instructionQ) {

    int i;
    char tempLetterCpy[2];/*for visual debugging and checking*/
    tempLetterCpy[1] = '\0';

    for(i = 1 ; i < length ; i++ , (*currLinePos)++) {
        tempLetterCpy[0] = buffer[*currLinePos];
        enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, instructionQ, tempLetterCpy, 0, *instructionCounter, buffer[*currLinePos], (VALUE_STATE | COMPLETED_STATE));
        *instructionCounter += 1;
    }
    enqueueV1_1(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, instructionQ, " \\0", 0, *instructionCounter, 0, (VALUE_STATE | COMPLETED_STATE));
    *instructionCounter += 1;

    return SUCCESS;
}

/*ignoring control characters, will act as if they were arbitrary printable characters*/
int processStringInstruction(char buffer[], int *currLinePos, long currLineNumber, int *instructionCounter, struct Queue *instructionQ) {

    int length[1];
    length[0] = 0;

    if(!isValidStringInstruction(buffer, currLinePos, currLineNumber, length))
        return ERROR;
    else
        return addStringToQueue(buffer, currLinePos, *length, instructionCounter, instructionQ);
}
/***************************************************************************************************** STRING INSTRUCTIONS *********************************************************/
/***************************************************************************************************** EXTERN AND ENTRY INSTRUCTIONS ***********************************************/
int getEntryOrExternOperand(char buffer[], int *currLinePos, long currLineNumber, int typeOfInstruction,char labelStr[], const struct HashTable *reservedKeywords) {

    if(isEmptyLine(buffer, currLinePos)) {
        printf("\nERROR @ line %ld - missing %s operand (label)\n", currLineNumber, (typeOfInstruction == ENTRY) ? "entry" : "extern");
        return ERROR;
    }

    if(copyLabelStr(buffer, currLinePos, labelStr, currLineNumber) == ERROR)
        return ERROR;

    if(hashIsReservedKeyWord(reservedKeywords, labelStr)) {
        printf("\nERROR @ line %ld - invalid label , label cannot be the same as protected keywords\n", currLineNumber);
        return ERROR;
    }

    return validateEndOfLine(buffer, currLinePos, currLineNumber);
}

int getExternInstructionOperand(char buffer[], int *currLinePos, long currLineNumber, char externLabelStr[], const struct HashTable *reservedKeywords) {
    return getEntryOrExternOperand(buffer, currLinePos, currLineNumber, EXTERN, externLabelStr, reservedKeywords);
}

int processExternInstruction(char buffer[], int *currLinePos, long currLineNumber, struct HashTable *externTable, const struct HashTable *reservedKeywords) {

    char externLabelStr[MAX_LABEL_LEN];

    if(getExternInstructionOperand(buffer, currLinePos, currLineNumber, externLabelStr, reservedKeywords) == ERROR)
        return ERROR;

    hashInsert(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, currLineNumber, externTable, externLabelStr, 0, EXTERNAL, (EXTERN_STATE | COMPLETED_STATE));

    return SUCCESS;
}

int getEntryInstructionOperand(char buffer[], int *currLinePos, long currLineNumber, char entryLabelStr[], const struct HashTable *reservedKeywords) {
    return getEntryOrExternOperand(buffer, currLinePos, currLineNumber, ENTRY, entryLabelStr, reservedKeywords);
}

int processEntryInstruction(char buffer[], int *currLinePos, long currLineNumber, struct HashTable *entryTable, const struct HashTable *reservedKeywords) {

    char entryLabelStr[MAX_LABEL_LEN];

    if(getEntryInstructionOperand(buffer, currLinePos, currLineNumber, entryLabelStr, reservedKeywords) == ERROR)
        return ERROR;

    hashInsert(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, currLineNumber, entryTable, entryLabelStr, 0, (ENTRY_STATE | COMPLETED_STATE));

    return SUCCESS;
}
/*pages 25-26 we're told entry and extern instructions each have 1 operand only and that's how it is implemented here. there's a mistake(I assume) on pages 33-34 ,where we're implicitly told we can have more than 1 operand*/
/***************************************************************************************************** EXTERN AND ENTRY INSTRUCTIONS ***********************************************/

void addToSymbolTable(char label[], int counter, long currLineNumber, unsigned int statement, struct HashTable *symbolTable) {

    int hashInsertStatus;
    hashInsertStatus = hashInsert(SYMBOL_DATA_ARGC, SYMBOL_DATATYPE, currLineNumber, symbolTable, label, counter, 0, (statement | COMPLETED_STATE));
    if(hashInsertStatus != SUCCESS)
        printf("\nDEBUGGING @ addToSymbolTable - label : %s statement : %d\n", label, statement);

    return;
}

int lineWithLabelDeclaration(char buffer[], int *currLinePos, long currLineNumber, char label[], char secondSubString[], struct HashTable *entryTable, struct HashTable *externTable, struct Queue *instructionQ, struct Queue *directiveQ, struct HashTable *symbolTable, const struct HashTable *reservedKeywords, int *directiveCounter, int *instructionCounter) {

    if(getSubStr(buffer, secondSubString, currLinePos, currLineNumber) == ERROR)
        return ERROR;

    switch(parseAndValidateSubStr(buffer, secondSubString, *currLinePos, currLineNumber, reservedKeywords)) {
        case DATA:
            addToSymbolTable(label, *instructionCounter, currLineNumber, INSTRUCTION_STATE, symbolTable);
            return processDataInstruction(buffer, currLinePos, currLineNumber, instructionCounter, instructionQ);
        case STRING:
            addToSymbolTable(label, *instructionCounter, currLineNumber, INSTRUCTION_STATE, symbolTable);
            return processStringInstruction(buffer, currLinePos, currLineNumber, instructionCounter, instructionQ);
        case ENTRY:
            printf("\nWarning @ line %ld - label declaration before an '.entry' instruction, label declaration will be discarded\n", currLineNumber);
            return processEntryInstruction(buffer, currLinePos, currLineNumber, entryTable, reservedKeywords);
        case EXTERN:
            printf("\nWarning @ line %ld - label declaration before an '.extern' instruction, label declaration will be discarded\n", currLineNumber);
            return processExternInstruction(buffer, currLinePos, currLineNumber, externTable, reservedKeywords);
        case LABEL:
            printf("\nERROR @ line %ld - multiple label declarations - %s\n", currLineNumber, label);
            return ERROR;
        case DIRECTIVE:
            addToSymbolTable(label, *directiveCounter, currLineNumber, LABEL_STATE, symbolTable);
            return processDirective(buffer, currLinePos, currLineNumber, secondSubString, directiveCounter, directiveQ, reservedKeywords);
        case ERROR:
            return ERROR;
        default:
            printf("\nFATAL @ line %ld - function lineWithLabelDeclaration - reached default - %s    -   %s\n", currLineNumber, label, secondSubString);
            return ERROR;
    }
}

int processLine(char buffer[], int *currLinePos, long currLineNumber, char *firstSubString, char *secondSubString, int *directiveCounter, int *instructionCounter, struct HashTable *entryTable, struct HashTable *externTable, struct Queue *instructionQ, struct Queue *directiveQ, struct HashTable *symbolTable, const struct HashTable *reservedKeywords) {

    if(getSubStr(buffer, firstSubString, currLinePos, currLineNumber) == ERROR)
        return ERROR;

    switch(parseAndValidateSubStr(buffer, firstSubString, *currLinePos, currLineNumber, reservedKeywords)) {
        case DATA:
            return processDataInstruction(buffer, currLinePos, currLineNumber, instructionCounter, instructionQ);
        case STRING:
            return processStringInstruction(buffer, currLinePos, currLineNumber, instructionCounter, instructionQ);
        case ENTRY:
            return processEntryInstruction(buffer, currLinePos, currLineNumber, entryTable, reservedKeywords);
        case EXTERN:
            return processExternInstruction(buffer, currLinePos, currLineNumber, externTable, reservedKeywords);
        case LABEL:
            return lineWithLabelDeclaration(buffer, currLinePos, currLineNumber, firstSubString, secondSubString, entryTable, externTable, instructionQ, directiveQ, symbolTable, reservedKeywords, directiveCounter, instructionCounter);
        case DIRECTIVE:
            return processDirective(buffer, currLinePos, currLineNumber, firstSubString, directiveCounter, directiveQ, reservedKeywords);
        case ERROR:
            printf("\nERROR @ line %ld - function processLine - reached ERROR\n", currLineNumber);
            return ERROR;
        default:
            printf("\nFATAL @ line %ld - function processLine - reached default\n", currLineNumber);
            return ERROR;
    }
}

int runFirstStage(char *buffer, int *currLinePos, long *currLineNumber, char *firstSubString, char *secondSubString, int *directiveCounter, int *instructionCounter, int *createFiles, FILE *fpExpanded, struct HashTable *entryTable, struct HashTable *externTable, struct Queue *instructionQ, struct Queue *directiveQ, struct HashTable *symbolTable, const struct HashTable *reservedKeywords) {

    int newLineState;
    int precessLineState;

    while((newLineState = getNewLineMainAsmPass(currLineNumber, buffer, fpExpanded, createFiles)) != STOP) {

        currLinePos[0] = 0;
        switch(newLineState) {
            case SUCCESS:
                precessLineState = processLine(buffer, currLinePos, *currLineNumber, firstSubString, secondSubString, directiveCounter, instructionCounter, entryTable, externTable, instructionQ, directiveQ, symbolTable, reservedKeywords);
                if(precessLineState == ERROR)
                    *createFiles = FALSE;
                break;
            case EMPTY_LINE:
                break;
            case LAST_LINE:
                precessLineState = processLine(buffer, currLinePos, *currLineNumber, firstSubString, secondSubString, directiveCounter, instructionCounter, entryTable, externTable, instructionQ, directiveQ, symbolTable, reservedKeywords);
                if(precessLineState == ERROR)
                    *createFiles = FALSE;
                return SUCCESS;
            default:
                printf("\nFATAL - function 'runFirstStage' - reached default\n");
                *createFiles = FALSE;
                return ERROR;
        }
    }

    return SUCCESS;
}
/*add protection in case we have over 256 lines in total from instructions and directives , since we only have a space of 256 "words" in memory, */
int firstStage(int *createFiles, char *file, int *directiveCounter, int *instructionCounter, struct HashTable *entryTable, struct HashTable *externTable, struct Queue *instructionQ, struct Queue *directiveQ, struct HashTable *symbolTable, const struct HashTable *reservedKeywords) {

    char buffer[MAX_LINE + 1]; /*adding 1 ,so we can add '\n' and '\0' to the end of the line , might be redundant now but can be a good measure in case things change later on*/
    char firstSubString[MAX_LABEL_LEN];
    char secondSubString[MAX_LABEL_LEN];
    int currLinePos[1];
    long currLineNumber[1];
    
    FILE *fpExpanded;

    currLineNumber[0] = 0;

    fpExpanded = fopen(file, "r");
    if(fpExpanded == NULL) {
        printf("\nERROR - cannot open file %s for reading\n", file); /*need to add a more precise error , check if ferror can help with that*/
        return ERROR;
    }

    runFirstStage(buffer, currLinePos, currLineNumber, firstSubString, secondSubString, directiveCounter, instructionCounter, createFiles, fpExpanded, entryTable, externTable, instructionQ, directiveQ, symbolTable, reservedKeywords);

    fclose(fpExpanded);

    return createFiles[0];
}


