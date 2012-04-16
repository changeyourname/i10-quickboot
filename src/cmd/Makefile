
OBJS_C = command.o cmd_mem.o cmd_kermit.o cmd_go.o
SRCS_C := $(OBJS_C:.o=.c)

all: $(BUILT_IN_OBJ)

$(BUILT_IN_OBJ): $(OBJS_C)
	$(LD) -r $(LDFLAGS) -o $@ $^

$(OBJS_C):$(SRCS_C)
	$(CC) $(CFLAGS) -c $^

clean:
	@rm -vf *.o


