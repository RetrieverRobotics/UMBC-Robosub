# https://hiltmon.com/blog/2013/07/03/a-simple-c-plus-plus-project-structure/

UNAME_S := $(shell uname -s)

CC := g++
SRCDIR := src
BUILDDIR := build

MAIN := brain
TARGET := bin/$(MAIN)

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT)) # generates a list of files in src/ ending with .cpp
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o)) # patsubst src/_each_, build/_each_, (a substitution reference that grabs all .cpp files in /src (from SOURCES) and changes ending to .o)
CFLAGS := --std=c++14 $(shell pkg-config --cflags opencv libserialport)
LIB := $(shell pkg-config --libs opencv libserialport) # also known as LDFLAGS
INC := -I include
VARS :=

ifeq ($(UNAME_S), Linux)
	VARS += -DKERNEL_LINUX
	LIB += -pthread
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "LINK $@"
	$(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo "CC $<"
	$(CC) $(CFLAGS) $(INC) $(VARS) -c -o $@ $<

clean:
	@echo "Cleaning..."
	$(RM) -r $(BUILDDIR) $(TARGET)

.PHONY: clean

$(V).SILENT:

.DELETE_ON_ERROR:
