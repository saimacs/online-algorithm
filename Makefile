onlinenew: onlinenew.o my402list.o
	mpicc -o onlinenew -g onlinenew.o my402list.o -lm

serial: serial.o my402list.o
	gcc -o serial -g serial.o my402list.o -lm

online: online.o my402list.o
	mpicc -o online -g online.o my402list.o -lm

serial.o: serial.c my402list.h
	gcc -g -c serial.c

onlinenew.o: onlinenew.c my402list.h
	mpicc -g -c onlinenew.c

online.o: online.c my402list.h
	mpicc -g -c online.c

my402list.o: my402list.c my402list.h
	gcc -g -c my402list.c

clean:
	rm -f *.o online
	rm -f *.o onlinenew
	rm -f *.o serial
