Model SQL Interpreter
============
**The task of implementing a model SQL interpreter is divided into the following subtasks:**
1. **Implementation of the "Client-Server" architecture.**
2. **Implementation of the SQL interpreter.**
3. **Implementation of a model SQL server based on the developed interpreter and provided class library for working with data files.**
4. **Implementation of the user interface with a model SQL server.**
5. **Implementation of a model database that demonstrates the performance of programs.**
**The implementation language is C++.**


One client. Client and server on a computer network
The client receives a SQL request from the user, analyzes it, and in case of an error informs the user about it, otherwise it sends the request to the server in some internal representation. The server accesses the database, determines the response to the request and sends it to the client. The client gives the server a response to the user.

Compilation:
make server
makeclient

Next, start the server, and then the client.
I tested on one computer, so I used localhost:
./client localhost 1234 (opens connection to localhost address and port 1234)
./server 1234 (passing the port)

The client receives the user's request and checks its correctness, if it turned out to be correct, it sends the request already in the internal representation to the server.
The server receives the correct request, analyzes it, if it encounters a where-clause, expression or logical expression, calls the appropriate functions. Next, the server sends a response to the client, in the case of SELECT, the client displays a temporary table, and in the case of UPDATE, the client receives a temporary table compiled by the server, turns this table into our original table and deletes the temporary one.

Internal representation:
the first digit from 1 to 6 is responsible for the type of offer
"1" - SELECT
"2" - INSERT
"3" - UPDATE
"4" - DELETE
"5" - CREATE
"6" - DROP
Next comes the name of the table.
Next comes either "*"(!), or a number from 1 to 4 (WHERE clause type)
(!) - only in the case of SELECT * , followed by a number from 1 to 4 (WHERE clause type)

Where clause:
"1" - IN/NOTIN
"2" - LIKE/NOTLIKE
"3" - ALL
"4" - boolean expression

All expressions and logical expressions are presented as Reverse Polish notation
The LIKE/NOTLIKE clause is implemented using regular expressions.
