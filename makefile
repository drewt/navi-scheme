CC        = gcc
CFLAGS    = -Wall -Wextra -Wno-unused-parameter -Wno-empty-body -g -O2
ALLCFLAGS = $(CFLAGS) -std=gnu11
AR        = ar
ARFLAGS   = rcs
LD        = $(CC)
LDFLAGS   =

libobjects = arithmetic.o bytevector.o char.o control_features.o display.o \
	     environment.o eval.o list.o port.o read.o sexp.o string.o \
	     vector.o uchar.o
objects = $(libobjects) repl.o
target  = navi
clean   = $(objects) $(target) libnavi.a

all: $(target)

include rules.mk

libnavi.a: $(libobjects)
	$(call cmd,ar)

$(target): repl.o libnavi.a
	$(call cmd,ld)
