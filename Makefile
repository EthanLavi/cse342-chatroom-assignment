out: ./client/client.o ./server/server.o debugging.o
	g++ ./client/client.o debugging.o -o ./client/c -lpthread
	g++ ./server/server.o debugging.o -o ./server/s -lpthread
	
./client/client.o: ./client/client.cpp ./protocol_header.h
	g++ -c ./client/client.cpp -o ./client/client.o

./server/server.o: ./server/server.cpp ./protocol_header.h
	g++ -c ./server/server.cpp -o ./server/server.o

./debugging.o: ./debugging.cpp ./debugging.h
	g++ -c ./debugging.cpp

clean_o:
	rm *.o ./client/*.o ./server/*.o

clean:
	rm *.o ./client/*.o ./client/c ./server/*.o ./server/s 
