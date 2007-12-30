
CC=g++
CFLAGS=-c -ggdb -Wall -I.. -I../.. -I../../network  -I../../../bin 
LDFLAGS= -L/usr/lib/ -lcppunit
SOURCES=tPolynomialTest.cpp  tToolUnitTest.cpp nMessageMock.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=tToolUnitTest

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@ 
