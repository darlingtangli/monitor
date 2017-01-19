TARGET_LIB=./lib/libmonitor.a
MTOOL=./bin/mtool
MREPORT=./bin/mreport
TEST1=./bin/test1
TEST2=./bin/test2

CC=gcc
CFLAGS=-Wall -O2 -DNDEBUG
CXX=g++
CXXFLAGS=-Wall -O2
LIBS=-pthread

INCLUDE = -I ./include/

COBJS = ./src/report_impl.o ./src/loader.o ./src/hash_map.o ./src/spin_lock.o
MTOOLOBJS = ./src/mtool.o
MREPORTOBJS = ./src/mreport.o
TEST1OBJS = ./src/test1.o
TEST2OBJS = ./src/test2.o

all : $(TARGET_LIB) $(MTOOL) $(MREPORT) $(TEST1) $(TEST2)

$(TARGET_LIB) : $(COBJS)
	ar cr $@ $^

$(COBJS) : %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(MTOOL) : $(MTOOLOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE)

$(MREPORT) : $(MREPORTOBJS) $(TARGET_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE)

$(TEST1) : $(TEST1OBJS) $(TARGET_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE) $(LIBS)

$(TEST2) : $(TEST2OBJS) $(TARGET_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE) $(LIBS)

$(MTOOLOBJS) : %.o : %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

$(MREPORTOBJS) : %.o : %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

$(TEST1OBJS) : %.o : %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

$(TEST2OBJS) : %.o : %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

install :
	cp ./include/report.h /usr/local/include/
	cp ./lib/libmonitor.a /usr/local/lib/
	cp ./bin/mtool ./bin/mreport /usr/local/bin/

uninstall :
	rm /usr/local/include/report.h
	rm /usr/local/lib/libmonitor.a
	rm /usr/local/bin/mtool
	rm /usr/local/bin/mreport

clean :
	rm -rf ./src/*.o $(TARGET_LIB) $(MTOOL) $(MREPORT) $(TEST1) $(TEST2)
