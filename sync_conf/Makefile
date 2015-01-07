TARGET = $(addsuffix .a,$(notdir $(shell pwd))) 
SRC := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRC))
CC = gcc
AR = ar 

$(TARGET): $(OBJS)
	$(AR) -rv $@ $^

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
