CFLAGS = -Wall 

server: ftserver.c 
	gcc -o ftserver -g ftserver.c $(CFLAGS)

clean:
	rm -rf *.dSYM *.o *.gcov *.gcda *.gcno *.so *.out 
