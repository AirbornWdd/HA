TARGET = $(addsuffix .a,$(notdir $(shell pwd))) 
SRC := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRC))
CC = gcc
AR = ar 
CFLAGS = -I. -I.. -DHAVE_CONFIG_H

$(TARGET): $(OBJS)
	$(AR) -rv $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
