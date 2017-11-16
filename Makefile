CC = gcc
CFLAGS = -Wall -pedantic -Wextra -pthread
BIN = agents
OBJ = agent.o 

agents: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)
agent.o: agent.c agent.h
	$(CC) $(CFLAGS) -c agent.c
clean:
	rm -f $(BIN) *.o core account *~
