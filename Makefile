# Makefile for hev-dns-forwarder
 
PP=cpp
CC=cc
CCFLAGS=-O3 -Werror -Wall
LDFLAGS=
 
SRCDIR=src
BINDIR=src
BUILDDIR=src
 
TARGET=$(BINDIR)/hev-dns-forwarder
CCOBJSFILE=$(BUILDDIR)/ccobjs
-include $(CCOBJSFILE)
LDOBJS=$(patsubst $(SRCDIR)%.c,$(BUILDDIR)%.o,$(CCOBJS))
 
DEPEND=$(LDOBJS:.o=.dep)
 
all : $(CCOBJSFILE) $(TARGET)
	@$(RM) $(CCOBJSFILE)
 
clean : 
	@echo -n "Clean ... " && $(RM) $(TARGET) $(CCOBJSFILE) $(BUILDDIR)/*.dep  $(BUILDDIR)/*.o && echo "OK"

run :
	@$(TARGET)
 
$(CCOBJSFILE) : 
	@mkdir -p $(BINDIR) $(BUILDDIR)
	@echo CCOBJS=`ls $(SRCDIR)/*.c` > $(CCOBJSFILE)
 
$(TARGET) : $(LDOBJS)
	@echo -n "Linking $^ to $@ ... " && $(CC) -o $@ $^ $(LDFLAGS) && echo "OK"
 
$(BUILDDIR)/%.dep : $(SRCDIR)/%.c
	@$(PP) $(CCFLAGS) -MM -MT $(@:.dep=.o) -o $@ $<
 
$(BUILDDIR)/%.o : $(SRCDIR)/%.c
	@echo -n "Building $< ... " && $(CC) $(CCFLAGS) -c -o $@ $< && echo "OK"
 
-include $(DEPEND)
