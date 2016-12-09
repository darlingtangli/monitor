TARGET_LIB=./lib/libmonitor.a
TEST=./bin/test
MTOOL=./bin/mtool
MREPORT=./bin/mreport
CC=g++ 
CFLAGS=-Wall -Wno-deprecated -O2

INCLUDE = -I ./include/
LIBS=-pthread

objects = ./src/report_impl.o ./src/initiator.o ./src/hash_map.o ./src/hash.o

testexe = ./src/test.o 
mtoolexe = ./src/mtool.o
mreportexe = ./src/mreport.o

all : $(TARGET_LIB) $(TEST) $(MTOOL) $(MREPORT)

$(TARGET_LIB) : $(objects)
	ar cr $@ $^

$(TEST) : $(testexe) $(TARGET_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

$(MTOOL) : $(mtoolexe)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^

$(MREPORT) : $(mreportexe) $(TARGET_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^

$(objects) : %.o : %.cc
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(testexe) : %.o : %.cc
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(mtoolexe) : %.o : %.cc
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

install :
	cp ./include/* /usr/local/include/
	cp ./lib/* /usr/local/lib/
	cp ./bin/mtool ./bin/mreport /usr/local/bin/

clean :
	rm -rf ./src/*.o ./lib/* ./bin/* 
