CC=gcc
CFLAGS=-Wall -ansi -pedantic

.PHONY: all
all: assembler

assembler: main.o macroExpansion.o mainAssemblerPass.o fulfillAndValidatePass.o writeToFileStage.o sharedFunctions.o queueDS.o hashTableDS.o mainAsmPassUtil.o sharedEnums.h
	$(CC) $(CFLAGS) $^ -o $@

mainAsmPassUtil.o: mainAsmPassUtil.c sharedFunctions.h hashTableDS.h sharedEnums.h
	$(CC) -c $(CFLAGS) $< -o $@

hashTableDS.o: hashTableDS.c queueDS.h sharedFunctions.h sharedEnums.h
	$(CC) -c $(CFLAGS) $< -o $@

queueDS.o: queueDS.c sharedEnums.h
	$(CC) -c $(CFLAGS) $< -o $@

sharedFunctions.o: sharedFunctions.c queueDS.h hashTableDS.h sharedEnums.h
	$(CC) -c $(CFLAGS) $< -o $@

main.o: main.c queueDS.h hashTableDS.h sharedEnums.h
	$(CC) -c $(CFLAGS) $< -o $@

macroExpansion.o: macroExpansion.c queueDS.h sharedFunctions.h hashTableDS.h sharedEnums.h
	$(CC) -c $(CFLAGS) $< -o $@

mainAssemblerPass.o: mainAssemblerPass.c queueDS.h sharedFunctions.h hashTableDS.h sharedEnums.h mainAsmPassUtil.h
	$(CC) -c $(CFLAGS) $< -o $@

fulfillAndValidatePass.o: fulfillAndValidatePass.c queueDS.h sharedFunctions.h hashTableDS.h sharedEnums.h
	$(CC) -c $(CFLAGS) $< -o $@

writeToFileStage.o: writeToFileStage.c queueDS.h sharedFunctions.h hashTableDS.h sharedEnums.h
	$(CC) -c $(CFLAGS) $< -o $@
	
.PHONY: clean
clean:
	rm -f *.o assembler
