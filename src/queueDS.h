

struct Qnode;
struct Queue;

struct Queue *createQueue();
int enqueueV1_1(int nodeArgCount, int nodeDataType, struct Queue *queue, const char label[], unsigned long authKey, ...);
int freeQueue(struct Queue *queue, int nodeDataType);

struct Qnode *getCurrentNode(struct Queue *queue);
void addToBinaryCode(struct Qnode *node, int value);
void updateTailBinaryCode(struct Queue *queue, int value);

unsigned int getNodeStatement(const struct Qnode *node);

const struct Qnode *getHeadNode(const struct Queue *queue);

unsigned int hashFunction(const struct Qnode *node);
const struct Qnode *getNextNode(const struct Qnode *node);

int isEmpty(const struct Queue *queue);
const char *getNodeLabel(const struct Qnode *node);
unsigned int isComplete(const struct Qnode *node);
void completeNode(struct Qnode *node, unsigned int statementVal, int binaryVal, int overRide);
unsigned int getDecimalAddress(const struct Qnode *node);
void addToStatement(struct Qnode *node, int statementVal);

unsigned int isLabelStatement(const struct Qnode *node);
unsigned int isValueStatement(const struct Qnode *node);
unsigned int isExternStatement(const struct Qnode *node);
unsigned int isEntryStatement(const struct Qnode *node);
unsigned int isInstructionStatement(const struct Qnode *node);
unsigned int isDirectiveStatement(const struct Qnode *node);
unsigned int isUnknownStatement(const struct Qnode *node);

int getBinaryCode(const struct Qnode *node);
int isIdenticalAuthKey(const struct Qnode *nodeA, const struct Qnode *nodeB);
void addToDecimalAddress(struct Qnode *node, unsigned int val);
void setDecimalAddress(struct Qnode *node, unsigned int decimalAddress);

unsigned long getAuthKey(const struct Qnode *node);
int getOpCodeV1(const struct Qnode *node);

int getMacroStartingPos(const struct Qnode *node);
int getMacroEndingPos(const struct Qnode *node);
