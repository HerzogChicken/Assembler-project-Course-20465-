

struct HashTable;
struct HashTable *createHashTable();
unsigned int hashFunctionV2(const char label[]);
int hashInsert(int nodeArgCount, int nodeDataType, long currLineNumber, struct HashTable *hashTable, const char *label, ...);
unsigned int hashGetElementCount(const struct HashTable *hashTable);
int hashIsEmptyCell(const struct HashTable *hashTable, unsigned int index);
int hashIsEmptyTable(const struct HashTable *hashTable);

const struct Qnode *hashSearchElement(const struct HashTable *hashTable, unsigned long authKey, unsigned int index);
const struct Qnode *hashSearch(const struct HashTable *hashTable, const struct Qnode *node);

const struct Qnode *hashGetNextNonEmptyCell(const struct HashTable *hashTable, unsigned int *index);

void freeHashTable(struct HashTable *hashTable, int nodeDataType);

int hashIsReservedKeyWord(const struct HashTable *reservedKeywords, char string[]);
int hashGetOpCode(const struct HashTable *reservedKeywords, const char directive[]);
int instantiateReservedKeywordsTable(int argCount, struct HashTable *hashTable, ...);
struct HashTable *createReservedKeywordsTable();

