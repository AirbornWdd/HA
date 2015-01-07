SUBDIRS = detect heartbeat lib sync_conf
LIBS := $(foreach dir, $(SUBDIRS),$(dir)/$(dir).a)
SRC := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRC))
CFLAGS = -g -Wall -I./lib/ -I./detect -I./heartbeat \
	-I./sync_conf -I. -DHAVE_CONFIG_H
LDFLAGS = -lcrypt -lrt -lcap
CC = gcc
BIN := ha_deamon

$(BIN):	$(OBJS) $(LIBS) 
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIBS):
	@for dir in $(SUBDIRS) ; \
	do \
		echo "Compiling $$dir ..." ; \
		make -C $$dir ; \
	done ;

clean:
	@for dir in $(SUBDIRS) ; \
	do \
		echo "Cleaning $$dir ..." ; \
		make -C $$dir clean; \
	done ;
	rm -f $(BIN) $(OBJS)
