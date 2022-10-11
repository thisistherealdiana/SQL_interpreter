Table.o: Table.h Table.cxx
	g++ Table.cxx -c -o Table.o
client: Table.o sql_client.cxx
	g++ sql_client.cxx Table.o -o client
server: Table.o sql_server.cxx
	g++ sql_server.cxx Table.o -o server
