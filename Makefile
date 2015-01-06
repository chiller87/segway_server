# the compiler to use.
CC=gcc
# options to pass to the compiler.
CFLAGS=-c -Wall -O3 -funroll-loops -funroll-all-loops -lpthread
# options to pass to the linker
LDFLAGS=`pkg-config --cflags --libs opencv` -lm
# the soure files
SOURCES=server.c
# the object files
OBJECTS=$(SOURCES:.c=.o)
# name of the executable
EXECUTABLE=server

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o $(EXECUTABLE) 

