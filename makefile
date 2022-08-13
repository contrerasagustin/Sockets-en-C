all:
	g++ -o cliente client.cpp -pthread
	g++ -o servidor server.cpp -pthread
cliente: 
	g++ -o cliente client.cpp -pthread
servidor:
	g++ -o servidor server.cpp -pthread