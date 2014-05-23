CC        = gcc
CFLAGS    = -Wall -Wextra -Wno-unused-parameter -Wno-empty-body -g -O2
ALLCFLAGS = $(CFLAGS) -std=gnu11
LD        = $(CC)
LDFLAGS   =

objects = arithmetic.o bytevector.o char.o control_features.o display.o \
	  environment.o eval.o list.o port.o read.o repl.o sexp.o string.o \
	  vector.o uchar.o
target  = tlisp
clean   = $(objects)

all: $(target)

include rules.mk

$(target): $(objects)
	$(call cmd,ld)
