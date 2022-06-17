# 2021-03-11
# Make test executables for the arm64 simulator
#----------------------------------------
# change the stupid tab-prefix syntax to a dash:
.RECIPEPREFIX = -
#----------------------------------------
S=as
SFLAGS=-als -g
LINK=ld
LFLAGS=

DEST=../Test-exes/

%.o: %.s
-       $(S) -o $@ $(SFLAGS) $<  > $*.lst

%: %.o
-       $(LINK) $(LFLAGS) -o $@ $<

#----------------------------------------
help:
-       @echo "Targets:"
-       @echo "    help"
-       @echo "    clean"
-       @echo "    veryclean"
-       @echo ""
-       @echo "  Executables:"
-       @echo "    nop"
-       @echo "    simplestring"
-       @echo "    demostr0"
-       @echo "    hexsmall"
-       @echo "    hexbig"
-       @echo "    dialog"
-       @echo "    writeint"
-       @echo "    factorial"
-       @echo "    fibonacci"
-       @echo "    averageloop"
-       @echo "    all"
-       @echo ""
-       @echo "  Assembly listings:"
-       @echo "    nop.o"
-       @echo "    demostr0.o"
-       @echo "    hexsmall.o"
-       @echo "    hexbig.o"
-       @echo "    simplestring.o"
-       @echo "    dialog.o"
-       @echo "    writeint.o"
-       @echo "    factorial.o"
-       @echo "    fibonacci.o"
-       @echo "    averageloop.o"
-       @echo ""
-       @echo "  Linked helper functions:"
-       @echo "    Utility/int2hex.o"
-       @echo "    Utility/int2str.o"
-       @echo "    Utility/strlen.o"
-       @echo "    Utility/write.o"
-       @echo "    Utility/constwrite.o"
-       @echo "    Utility/hexwrite.o"
-       @echo "    Utility/intwrite.o"
-       @echo "    Utility/strwrite.o"

#----------------------------------------
clean:
-       -rm -f *.o *~ *.lst

veryclean: clean
-       -rm -f nop demostr0 hexsmall hexbig simplestring dialog writeint factorial fibonacci averageloop

#----------------------------------------

int2hex.o: Utility/int2hex.s
-	$(S) $(SFLAGS) -o $@ $^ > $*.lst

int2str.o: Utility/int2str.s
-	$(S) $(SFLAGS) -o $@ $^ > $*.lst

strlen.o: Utility/strlen.s
-	$(S) $(SFLAGS) -o $@ $^ > $*.lst

write.o: Utility/write.s
-	$(S) $(SFLAGS) -o $@ $^ > $*.lst

constwrite.o: Utility/constwrite.s
-	$(S) $(SFLAGS) -o $@ $^ > $*.lst

hexwrite.o: Utility/hexwrite.s
-	$(S) $(SFLAGS) -o $@ $^ > $*.lst

intwrite.o: Utility/intwrite.s
-	$(S) $(SFLAGS) -o $@ $^ > $*.lst

strwrite.o: Utility/strwrite.s
-	$(S) $(SFLAGS) -o $@ $^ > $*.lst

#----------------------------------------
nop.o: nop.s

nop: nop.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


demostr0.o: demostr0.s

demostr0: demostr0.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


hexsmall.o: hexsmall.s

hexsmall: hexsmall.o  constwrite.o hexwrite.o  int2hex.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


hexbig.o: hexbig.s

hexbig: hexbig.o  constwrite.o hexwrite.o  int2hex.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


simplestring.o: simplestring.s

simplestring: simplestring.o write.o strlen.o intwrite.o int2str.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


dialog.o: dialog.s

dialog: dialog.o write.o strlen.o intwrite.o int2str.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


writeint.o: writeint.s

writeint: writeint.o strwrite.o strlen.o int2str.o hexwrite.o int2hex.o intwrite.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


factorial.o: factorial.s

factorial: factorial.o constwrite.o intwrite.o int2str.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


fibonacci.o: fibonacci.s

fibonacci: fibonacci.o constwrite.o intwrite.o int2str.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


averageloop.o: averageloop.s

averageloop: averageloop.o constwrite.o intwrite.o int2str.o strwrite.o strlen.o
-	$(LINK) $(LFLAGS) -o $@ $^
-	./$@
-	@mkdir -p $(DEST)
-	@mv -f $@ $(DEST)/$@
-	@echo '#--'


all: nop demostr0 hexsmall hexbig simplestring dialog writeint factorial fibonacci averageloop
-	ls -l $(DEST)

#----------------------------------------
