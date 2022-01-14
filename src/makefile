EXE=spin
LIB=pthread m
IDIR=../include
ODIR=obj
SRC=main.c render.c matrix.c
DEP=params.h render.h matrix.h
CU_SRC=render.cu
CU_DEP=rendercu.h
LIBCUDART=/opt/cuda/targets/x86_64-linux/lib
COMPUTE_VERSION=61

# make spin
$(EXE): $(patsubst %,$(ODIR)/%.opt.o,$(basename $(SRC)))
	gcc -s -static $^ $(patsubst %,-l%,$(LIB)) -o $@

$(ODIR)/%.opt.o: %.c $(patsubst %,$(IDIR)/%,$(DEP))
	gcc -Wall -O3 -march=native -I$(IDIR) -c $< -o $@

# make spindebug
$(patsubst %,%debug,$(EXE)): $(patsubst %,$(ODIR)/%.o,$(basename $(SRC)))
	gcc -static $^ $(patsubst %,-l%,$(LIB)) -o $@

$(ODIR)/%.o: %.c $(patsubst %,$(IDIR)/%,$(DEP))
	gcc -Wall -Og -g -I$(IDIR) -c $< -o $@

# make spincuda
$(patsubst %,%cuda,$(EXE)): $(patsubst %,$(ODIR)/%.opt.CUDA_R.o,$(basename $(SRC))) \
	$(patsubst %,$(ODIR)/%.opt.cu.o,$(basename $(CU_SRC)))
	gcc -s -L$(LIBCUDART) $^ -lcudart $(patsubst %,-l%,$(LIB)) -o $@

$(ODIR)/%.opt.CUDA_R.o: %.c $(patsubst %,$(IDIR)/%,$(DEP) $(CU_DEP))
	gcc -Wall -O3 -march=native -I$(IDIR) -DCUDA_R -c $< -o $@

$(ODIR)/%.opt.cu.o: %.cu $(patsubst %,$(IDIR)/%,$(DEP) $(CU_DEP))
	nvcc -O3 -arch compute_$(COMPUTE_VERSION) -code sm_$(COMPUTE_VERSION) -I$(IDIR) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(EXE) $(patsubst %,%debug,$(EXE)) $(patsubst %,%cuda,$(EXE))
