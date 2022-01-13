EXE=spin
LIB=m pthread
IDIR=../include
ODIR=obj
SRC=main.c matrix.c
DEP=params.h matrix.h
CU_SRC=render.cu
CU_DEP=render.h

$(EXE): $(patsubst %,$(ODIR)/%.opt.o,$(basename $(SRC)))
	gcc -s -static $^ $(patsubst %,-l%,$(LIB)) -o $@ 

$(ODIR)/%.opt.o: %.c $(patsubst %,$(IDIR)/%,$(DEP))
	gcc -Wall -O3 -march=native -I$(IDIR) -c $< -o $@

$(patsubst %,%debug,$(EXE)): $(patsubst %,$(ODIR)/%.o,$(basename $(SRC)))
	gcc -static $^ $(patsubst %,-l%,$(LIB)) -o $@ 

$(ODIR)/%.o: %.c $(patsubst %,$(IDIR)/%,$(DEP))
	gcc -Wall -Og -g -I$(IDIR) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(EXE) $(patsubst %,%debug,$(EXE))

#$(EXE): $(OBJ)
#	nvcc -o $@ $^
#
#
#$(ODIR)/%.o: %.cu $(DEPS)
#	nvcc -arch compute_61 -code sm_61 -g -G -c -o $@ $< -I$(IDIR)
#
#$(EXEOPT): $(OBJOPT)
#	nvcc -o $@ $^ $(LIBS)
#
#$(ODIR)/%.opt.o: %.c $(DEPS)
#	gcc -O3 -c -o $@ $< -I$(IDIR)
#
#$(ODIR)/%.opt.o: %.cu $(DEPS)
#	nvcc -O3 -arch compute_61 -code sm_61 -c -o $@ $< -I$(IDIR)	
