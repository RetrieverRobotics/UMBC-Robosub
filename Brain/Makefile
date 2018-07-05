# https://hiltmon.com/blog/2013/07/03/a-simple-c-plus-plus-project-structure/

CC := g++
SRCDIR := src
BUILDDIR := build

MAIN := brain
TARGET := bin/$(MAIN)

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS := --std=c++11 $(shell pkg-config --cflags opencv)
LIB := $(shell pkg-config --libs opencv)
INC := -I include

all: $(TARGET) # so that "make force_nc all" can be used to, however "make" works just as well for normal cases

$(TARGET): $(OBJECTS)
	@echo "LINK $@"
	$(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo "CC $<"
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning..."
	$(RM) -r $(BUILDDIR) $(TARGET)

.PHONY: clean

force_nc:
	@touch $(SRCDIR)/$(MAIN).$(SRCEXT) # brain.cpp includes code from actions.cpp_nc and tasks.cpp_nc which are not watched for compile, so this refreshes the timestamp on brain

$(V).SILENT: