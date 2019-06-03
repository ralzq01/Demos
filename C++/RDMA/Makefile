all:	client server
client: client.c
		gcc client.c -g -o client -libverbs -lmlx4 -pthread
		gcc client.c -g -DDEBUG -o client_d -libverbs -lmlx4 -pthread
server: server.c
		gcc server.c -g -o server -libverbs -lmlx4 -pthread
		gcc server.c -g -DDEBUG -o server_d -libverbs -lmlx4 -pthread
