# 2022-05-20
#----------------------------------------
# change the stupid tab-prefix syntax to a dash:
.RECIPEPREFIX = -
#----------------------------------------
CC=gcc
CFLAGS=-Wall
LFLAGS=

#----------------------------------------
help:
-       @echo "Targets:"
-       @echo "    help"
-       @echo "    memsim-stub"
-       @echo "    memsim-full"
-       @echo "    memsim-all"
-       @echo "    clean"
-       @echo "    veryclean"

#----------------------------------------
memsim-stub: memsimulate.c memory.c fde-stub.c
-       $(CC) $(CFLAGS) -o $@  $^

#----------------------------------------
memsim-full: memsimulate.c memory.c fde-full.c  decode.c execute.c
-       $(CC) $(CFLAGS) -o $@  $^ $(LFLAGS)

#----------------------------------------
# 2022-05-22
memsim-all: memsimulate.c memory.c fde-full.c  decode.c exec.movk-madd-sub-sys_read.c
-       $(CC) $(CFLAGS) -o $@  $^ $(LFLAGS)

#----------------------------------------
clean:
-       rm -f *.o *~ .*.un~

veryclean: clean
-       rm -f  memsim-stub  memsim-full  memsim-all

#----------------------------------------
