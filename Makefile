NAME = predprey
SRCS = predprey.c game.c util.c
OBJS = $(SRCS:.c=.o)
HDRS = *.h

$(NAME): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(OBJS) -l:libraylib.a -lm $(LDFLAGS) -o $@

clean:
	$(RM) *.o

OBJS += winmain.c
$(NAME).exe: $(OBJS) $(HDRS)
	$(CC) -mwindows $(CFLAGS) $(OBJS) -l:libraylib.a -lm -lgdi32 -lopengl32 -lwinmm $(LDFLAGS) -o $@
