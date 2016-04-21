OBJS=raspberry_pi_revision.o rpiinfo.o
BIN=rpiinfo

DEBUG=-g
CFLAGS+=-Wall -g -O3

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) -c $< -o $@ -Wno-deprecated-declarations

$(BIN): $(OBJS)
	$(CC) $(DEBUG) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
