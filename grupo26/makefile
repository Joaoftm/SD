

CC = gcc
INCLUDE = include/
OBJ = object/
SRC = source/
BIN = binary/
FLAG = gcc -g -Wall -I include/ -c
LIB = lib/
PRJ = proj_1/
CLT = client/
SVR = server/
MSG = message/
PROTOC=/usr/local/
CFLAGS = $(CC) -g -Wall -I $(INCLUDE)
LDFLAGS = ${PROTOC}lib/libprotobuf-c.a
pthread = -lpthread 
zooflag = -lzookeeper_mt
all : data.o entry.o serialization.o tree.o

# FASE 1 DATA / ENTRY / TREE / SERIALIZATION
data.o: $(INCLUDE)data.h
	$(FLAG) $(SRC)$(PRJ)data.c -o $(OBJ)data.o

entry.o: $(INCLUDE)entry.h $(INCLUDE)data.h
	$(FLAG) $(SRC)$(PRJ)entry.c -o $(OBJ)entry.o

serialization.o: $(INCLUDE)serialization.h $(INCLUDE)data.h $(INCLUDE)entry.h $(INCLUDE)tree.h $(INCLUDE)tree-private.h 
	$(FLAG) $(SRC)$(PRJ)serialization.c -o $(OBJ)serialization.o

tree.o: $(INCLUDE)tree-private.h $(INCLUDE)tree.h $(INCLUDE)entry.h $(INCLUDE)data.h
	$(FLAG) $(SRC)$(PRJ)tree.c -o $(OBJ)tree.o

# FASE 2 CLIENT / SERVER / MESSAGE

network_client.o = inet.h client_stub-private.h network_client.h message-private.h
client_stub.o = client_stub-private.h message-private.h network_client.h inet.h
tree_client.o = inet.h client_stub-private.h client_stub.h

network_server.o = network_server.h tree_skel.h message-private.h inet.h
tree_skel.o = inet.h message-private.h tree_skel.h
tree_server.o = inet.h network_server.h message-private.h

message-private.o = message-private.h
sdmessage.pb-c.o = sdmessage.pb-c.h

all : proto client-lib.o tree-client tree-server

client-lib = $(PRJ)data.o $(PRJ)entry.o $(CLT)client_stub.o $(CLT)network_client.o
client-lib.o: proto $(client-lib)
	ld -r $(addprefix $(OBJ),$(notdir $(client-lib))) $(LDFLAGS) -o $(OBJ)$@

tree-c = $(PRJ)data.o $(PRJ)entry.o $(CLT)client_stub.o $(CLT)network_client.o \
			 $(CLT)tree_client.o $(MSG)sdmessage.pb-c.o $(MSG)message-private.o
tree-client: proto $(tree-c)
	$(CC) $(addprefix $(OBJ),$(notdir $(tree-c))) $(LDFLAGS) -o $(BIN)$@ $(zooflag) -DTHREADED


tree-s =  $(PRJ)data.o $(PRJ)entry.o $(PRJ)tree.o  \
			$(SVR)network_server.o $(SVR)tree_skel.o $(SVR)tree_server.o \
			$(MSG)sdmessage.pb-c.o $(MSG)message-private.o $(CLT)client_stub.o $(CLT)network_client.o

tree-server: proto $(tree-s)
	$(CC) $(addprefix $(OBJ),$(notdir $(tree-s))) $(LDFLAGS) -o $(BIN)$@  $(pthread) $(zooflag) -DTHREADED

proj_1/%.o: $(SRC)$(PRJ)%.c $(%.o)
	$(CFLAGS) -c $< -o $(OBJ)$(notdir $@)
client/%.o: $(SRC)$(CLT)%.c $(%.o)
	$(CFLAGS) -c $< -o $(OBJ)$(notdir $@)
server/%.o: $(SRC)$(SVR)%.c $(%.o)
	$(CFLAGS) -c $< -o $(OBJ)$(notdir $@)
message/%.o: $(SRC)$(MSG)%.c $(%.o)
	$(CFLAGS) -c $< -o $(OBJ)$(notdir $@)

proto: sdmessage.pb-c.h
%.pb-c.h: %.proto
	${PROTOC}bin/protoc-c sdmessage.proto --c_out=./
	mv $@ $(INCLUDE)
	mv $(@:.h=.c) $(SRC)$(MSG) 
	$(CFLAGS) -c $(SRC)$(MSG)$(@:.h=.c) -o $(OBJ)$(@:.h=.o)

all : server-lib.o client-lib.o

client-lib.o:
	ld -r $(OBJ)client_stub.o $(OBJ)network_client.o $(OBJ)data.o $(OBJ)entry.o\
		  -o $(LIB)client-lib.o 
server-lib.o:
	ld -r $(OBJ)data.o $(OBJ)entry.o $(OBJ)tree.o $(OBJ)tree_skel.o\
		  $(OBJ)network_server.o -o $(LIB)server-lib.o

# Test

runServerA: tree-server
	./binary/tree-server 5000 127.0.0.1 2181
runServerB:
	./binary/tree-server 5001 127.0.0.1 2181
runClient: tree-client
#zookeeper ip
	./binary/tree-client 127.0.1.1:2181
	
#Zookeeper:
#	cd /home/dakid/Desktop/sd/apache-zookeeper-3.5.8-bin/bin
#	./zkServer.sh start

#	CLEAN
clean:
	rm -f $(OBJ)*.o
	rm -f $(BIN)t*
	rm -f $(SRC)$(MSG)*pb-c.* $(INCLUDE)*pb-c.*
