EXE=spin
EXEOPT=$(patsubst %,%_opt,$(EXE))

LIBS=

IDIR=../include
ODIR=obj

_DEPS=params.h matrix.h render.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ=main.o matrix.o render.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))
OBJOPT=$(patsubst %.o,%.opt.o,$(OBJ))

all: $(EXE) $(EXEOPT)

$(EXE): $(OBJ)
	nvcc -o $@ $^ $(LIBS)

$(ODIR)/%.o: %.c $(DEPS)
	gcc -Wall -g -c -o $@ $< -I$(IDIR)

$(ODIR)/%.o: %.cu $(DEPS)
	nvcc -arch compute_61 -code sm_61 -g -G -c -o $@ $< -I$(IDIR)

$(EXEOPT): $(OBJOPT)
	nvcc -o $@ $^ $(LIBS)

$(ODIR)/%.opt.o: %.c $(DEPS)
	gcc -O3 -c -o $@ $< -I$(IDIR)

$(ODIR)/%.opt.o: %.cu $(DEPS)
	nvcc -O3 -arch compute_61 -code sm_61 -c -o $@ $< -I$(IDIR)	

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(EXE) $(EXEOPT)