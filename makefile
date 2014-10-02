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
testobjects = tests/arithmetic.o tests/bytevector.o tests/char.o tests/main.o
objects = $(libobjects) $(testobjects) repl.o
target  = navi
clean   = $(objects) $(target) libnavi.a

all: $(target)

include rules.mk

libnavi.a: $(libobjects)
	$(call cmd,ar)

$(target): repl.o libnavi.a
	$(call cmd,ld)

check: $(testobjects) libnavi.a
	$(call cmd,ld,-lcheck)
