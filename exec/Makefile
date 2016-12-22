CC=cc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=test

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) -L../lib -I../lib $(OBJECTS) -o $@ -lmaddb

.c.o:
	$(CC) $(CFLAGS) -I../lib $< -o $@ -lmaddb
