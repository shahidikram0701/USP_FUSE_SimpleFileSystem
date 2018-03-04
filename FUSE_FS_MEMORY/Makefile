run: fs_functions.o fs.o
	gcc -Wall fs_functions.o fs.o `pkg-config fuse3 --cflags --libs` -o run

fs_functions.o: fs_functions.c fs.h
	gcc -Wall -c fs_functions.c `pkg-config fuse3 --cflags --libs`

fs.o: fs.c fs.h
	gcc -Wall -c fs.c `pkg-config fuse3 --cflags --libs`

clean:
	rm *.o
