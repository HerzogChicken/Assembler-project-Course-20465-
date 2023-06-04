#ifndef MAMAN14_MAINASMPASSUTIL_H
#define MAMAN14_MAINASMPASSUTIL_H

#include <stdio.h>

#endif /*MAMAN14_MAINASMPASSUTIL_H*/


/*buffer functions*/
int flushInputLine(FILE *fpExpanded);
int fillLineBufferMainPass(int *index, long currLineNumber, int *fileChar, char lineBuffer[], FILE *fpExpanded, int *createFiles);
int getNewLineMainAsmPass(long *currLineNumber, char lineBuffer[], FILE *fpExpanded, int *createFiles);

/*parsing utils & error functions*/
int isEmptyLine(char buffer[], int *currLinePos);
int parseAndValidateSubStr(char buffer[], char subString[], int currLinePos, long currLineNumber, const struct HashTable *reservedKeywords);

/*string function utils*/
int getSubStr(char buffer[], char subString[], int *currLinePos, long currLineNumber);
int validateEndOfLine(const char buffer[], int *currLinePos, long currLineNumber);
int getToSecondOperand(const char buffer[], int *currLinePos, long currLineNumber);

/*directive string util functions */
int inRange(int val);
int getImmediateOperand(const char buffer[], int *currLinePos, long currLineNumber);
int copyLabelStr(const char buffer[], int *currLinePos, char label[], long currLineNumber);
int getValidJumpDestLabel(const char buffer[], int *currLinePos, long currLineNumber, char labelCopyStr[], const struct HashTable *reservedKeywords);
int determineOperandAddressing(const char buffer[], int *currLinePos, int operandState, long currLineNumber, const struct HashTable *reservedKeywords);
int determineJumpAddressingType(const char buffer[], int currLinePos, long currLineNumber);

/*data instruction util functions*/
int isValidTextBeforeNum(const char buffer[], int currLinePos, long currLineNumber);
void getToFirstNumber(char buffer[], int *currLinePos);
int getToNextNumber(char buffer[], int *currLinePos, long currLineNumber);
int getNumberSign(char sign, int *currLinePos);
int dataInstructionNumValidation(const char buffer[], const int *currLinePos, int number, long currLineNumber);

/*string instruction util functions*/
int isValidStringInstruction(char buffer[], int *currLinePos, long currLineNumber, int length[]);