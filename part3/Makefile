
all : bank
	
	
bank: bank.o string_parser.o
	gcc -o bank -pthread -lpthread bank.o string_parser.o
	
	
bank.o: bank.c
	gcc -g -c -pthread bank.c
	
string_parser.o: string_parser.c string_parser.h
	gcc -c string_parser.c

clean:
	rm -f core *.o bank