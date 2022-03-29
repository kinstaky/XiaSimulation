GXX = g++

ROBJS = res.o Resolution.o
OBJS = Adapt.o Adapter.o sim.o Simulator.o Picker.o FilterAlgorithm.o TraceReader.o SeperateTrace.o Single.o TimeRes.o
DEFINES =

ROOTCFLAGS = $(shell root-config --cflags)
CFLAGS = -Wall -O3 $(ROOTCFLAGS) $(INCLUDE) -pthread

ROOTLIBS = $(shell root-config --libs) -lSpectrum
LIBS = $(ROOTLIBS)
LDFLAGS = $(LIBS)

all:
	make adapt;
	make sim;
	make seperate;
	make single;
	make tres;
adapt: Adapt.o Adapter.o
	$(GXX) -o $@ $^ $(LDFLAGS)
sim: sim.o Simulator.o Picker.o FilterAlgorithm.o TraceReader.o
	$(GXX) -o $@ $^ $(LDFLAGS)
seperate: SeperateTrace.o
	$(GXX) -o $@ $^ $(LDFLAGS)
single: Single.o
	$(GXX) -o $@ $^ $(LDFLAGS)
tres: TimeRes.o
	$(GXX) -o $@ $^ $(LDFLAGS)
res: $(ROBJS)
	$(GXX) -o $@ $^ $(LDFLAGS)
$(OBJS):%.o:%.cpp
	$(GXX) $(CFLAGS) $(DEFINES) -c $<

clean:
	rm *.o adapt sim seperate single tres res || true
