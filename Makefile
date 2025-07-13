NAME = predprey
SRCS = predprey.c game.c util.c
OBJS = $(SRCS:.c=.o)
HDRS = *.h
CFLAGS += #-Wall -Wpedantic
LDLIBS += -l:libraylib.a -lm

$(NAME): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $(NAME)

clean:
	$(RM) *.o
