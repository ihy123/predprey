NAME = predprey
SRCS = predprey.c game.c util.c
OBJS = $(SRCS:.c=.o)
HDRS = *.h

$(NAME): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(OBJS) -l:libraylib.a -lm $(LDFLAGS) -o $@

clean:
	$(RM) *.o

$(NAME).exe: $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(OBJS) -l:libraylib.a -lm -lgdi32 -lopengl32 -lwinmm $(LDFLAGS) -o $@
