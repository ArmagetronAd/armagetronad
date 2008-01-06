
CC=g++
CFLAGS=-ggdb -Wall -I.. -I../.. -I../../network  -I../../../bin 
LDFLAGS= -L/usr/lib/ -lcppunit
EXTRA_SOURCES=../tArray.cpp ../tError.cpp
SOURCES=tPolynomialTest.cpp  tToolUnitTest.cpp nMessageMock.cpp $(EXTRA_SOURCES)
HEADERS=tPolynomial.h tPolynomialWithBase.hOB
EXPERIMENTATION=myTArray
EXPERIMENTATION_SRC=myTArray.cpp
EXPERIMENTATION_OBJECTS=$(EXPERIMENTATION_SRC:.cpp=.o)
OBJECTS=$(SOURCES:.cpp=.o)
EXTRA_OBJECTS=$(EXTRA_SOURCES:.cpp=.o)
EXECUTABLE=tToolUnitTest

all: $(SOURCES) $(EXECUTABLE) $(EXPERIMENTATION)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(EXPERIMENTATION): $(EXPERIMENTATION_SRC) $(EXTRA_SOURCES)
	$(CC) $(CFLAGS) -c $(EXPERIMENTATION_SRC) 
	$(CC) $(LDFLAGS) $(EXPERIMENTATION_OBJECTS) $(EXTRA_OBJECTS) -o $@
	
.cpp.o: $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ 
