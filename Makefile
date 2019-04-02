calc:linux_down.c Socket.h
	gcc -fopenmp -o linux_down linux_down.c Socket.h
clean:
	rm linux_down
