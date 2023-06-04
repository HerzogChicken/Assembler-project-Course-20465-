#include <ctype.h>
#include "queueDS.h"
#include "hashTableDS.h"
#include "sharedEnums.h"


void skipWhiteSpace(const char buffer[], int *currLinePos) {

    while(buffer[*currLinePos] != '\n' && isspace(buffer[*currLinePos])) {
        *currLinePos += 1;
    }
}

unsigned int getVal(char c) {

    if(isdigit(c))
        return (c - '0'); /* '0' = 48 , digits 0- 9*/
    else if(isupper(c))
        return (c - '7'); /* '7' = 55 , upper letters 10 - 35*/
    else /*lower case*/
        return (c - '='); /* '=' = 61 , lower letters 36 - 61*/
}

unsigned long getKey(const char labelCpy[]) {

    int i;
    unsigned long key;

    key = 0;

    for( i = 0 ; labelCpy[i] != '\0' && labelCpy[i] != ':' && i < MAX_LABEL_LEN - 1 ; ++i) {/*added labelCpy[i] != ':' since when sending a label to addToLabelQ it has ':' before ending and that's causing a bug with the authKey*/

        key = key * 2 + getVal(labelCpy[i]);

    }

    return key;
}
