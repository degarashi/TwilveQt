CXX		= g++
LD		= g++
CXXFLAGS	= -O2
LFLAGS		= 
PROGRAM		= enc
SRC		= $(wildcard *.cpp)
OBJ		= $(patsubst %.cpp,%.o, $(SRC))
DEPEND		= $(patsubst %.cpp,%.depend, $(SRC))

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $<
$(PROGRAM):	$(OBJ)
		$(LD) $(LFLAGS) $(OBJ) -o $@
		
%.depend:	%.cpp
		@set -e; $(CXX) -MM $(CXXFLAGS) $< \
                | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
                [ -s $@ ] || rm -f $@
-include $(DEPEND)

.PHONY: clean depend
clean:
	rm -f *.o *~ *.depend $(PROGRAM)
