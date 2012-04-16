
objs-y := core.o  ep0.o musb_core.o  musb_udc.o

OBJS_C = $(objs-y)
SRCS_C := $(OBJS_C:.o=.c)

all : $(BUILT_IN_OBJ)

$(BUILT_IN_OBJ) : $(OBJS_C)
	$(LD) $(LDFLAGS) -r $^ -o $@

$(OBJS_C) : $(SRCS_C)
	$(CC) $(CFLAGS) -c $(SRCS_C)

clean:
	@rm -vf *.o

