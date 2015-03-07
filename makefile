VERSION   = $(shell git rev-parse --verify --short HEAD 2>/dev/null)-git
CC        = gcc
CFLAGS    = -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-empty-body \
	    -Wno-missing-field-initializers -g -O2
ALLCFLAGS = $(CFLAGS) -std=c11 -D NAVI_COMPILE -D NAVI_VERSION="\"$(VERSION)\"" \
	    -D_POSIX_C_SOURCE=199309L -include assert.h
AR        = ar
ARFLAGS   = rcs
LD        = $(CC)
LDFLAGS   = `pkg-config --libs icu-uc icu-i18n`

libobjects = arithmetic.o bytevector.o char.o control_features.o display.o \
	     environment.o eval.o heap.o list.o port.o read.o string.o \
	     system.o vector.o
testobjects = tests/arithmetic.o tests/bytevector.o tests/char.o \
	      tests/lambda.o tests/list.o tests/main.o
objects = $(libobjects) $(testobjects) navii.o
target  = navii
clean   = $(objects) $(target) libnavi.a

all: $(target)

include rules.mk

libnavi.a: $(libobjects)
	$(call cmd,ar)

$(target): navii.o libnavi.a
	$(call cmd,ld)

check: $(testobjects) libnavi.a
	$(call cmd,ld,-lcheck)
