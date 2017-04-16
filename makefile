
TARGET=libtcommmq.a
CXX=g++
CFLAGS=-g -O2 -Wall

SRC=src
EXAMPLE=example

INC=-Iinclude
OBJS = $(addsuffix .o, $(basename $(wildcard $(SRC)/*.cpp)))

$(TARGET): $(OBJS)
	echo $(OBJS)
	ar cqs $@ $^

-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)
	@$(CXX) -MM $*.cpp $(INC) > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	rm -f $*.d.tmp

.PHONY: clean

clean:
	-rm -f src/*.o $(TARGET)
