notes: notes.o config.o cache.o utilities.o
	   gcc -Wall -Wextra -Wpedantic out/notes.o out/config.o out/cache.o out/utilities.o -o out/notes

notes.o: src/notes.c src/config.h src/utilities.h
		 gcc -Wall -Wextra -Wpedantic -c src/notes.c -o out/notes.o

config.o: src/config.c src/config.h src/utilities.h src/cache.h
		  gcc -Wall -Wextra -Wpedantic -c src/config.c -o out/config.o

cache.o: src/utilities.c src/config.h src/utilities.h src/cache.h
			 gcc -Wall -Wextra -Wpedantic -c src/cache.c -o out/cache.o

utilities.o: src/utilities.c src/utilities.h
			 gcc -Wall -Wextra -Wpedantic -c src/utilities.c -o out/utilities.o
