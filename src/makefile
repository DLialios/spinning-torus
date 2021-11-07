EXENAME=proj1

LIBS=

IDIR=../include
ODIR=obj

_DEPS=matrix.h params.h render.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ=main.o matrix.o render.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(EXENAME): $(OBJ)
	nvcc -o $@ $^ $(LIBS)

$(ODIR)/%.o: %.c $(DEPS)
	gcc -Wall -g -c -o $@ $< -I$(IDIR)

$(ODIR)/%.o: %.cu $(DEPS)
	nvcc -arch compute_61 -code sm_61 -g -G -c -o $@ $< -I$(IDIR)

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(EXENAME)