CC = g++
CFLAGS  = -g
SERVER = server
CLIENT = client

all: 
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp && $(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp

client:	all
	./$(CLIENT) $(ARGS)

server: all 
	./${SERVER} $(ARGS)

clean:
	$(RM) $(SERVER) $(CLIENT)




# all: 
# 	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp
# #	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp
	
# server:	${SERVER}
# 	./${SERVER} ${ARGS}

# client: ${CLIENT}
# 	./${CLIENT} ${ARGS}

# clean:
# 	$(RM) $(CLIENT) && $(RM) $(SERVER)
