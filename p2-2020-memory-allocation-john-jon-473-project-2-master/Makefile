CFLAGS = -std=gnu99
SOURCES = project2.c my_memory.c
OUT = out
default:
	gcc $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT) -lm
debug:
	gcc -g $(CFLAGS) $(SOURCES)  -o $(OUT) -lm
all:
	gcc $(SOURCES) $(LIBS) -o $(OUT) -lm
clean:
	rm $(OUT)
