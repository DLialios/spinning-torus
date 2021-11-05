IDIR=../include
CC=nvcc
CFLAGS=-I$(IDIR) -O3
ODIR=obj
LDIR=../lib
LIBS=-lm -lpthread
_DEPS=matrix.h params.h render.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))
_OBJ=main.o matrix.o render.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))
EXENAME=proj1

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXENAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(EXENAME)
