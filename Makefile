NAME = predprey
OBJS = predprey.o game.o util.o
HDRS = *.h

$(NAME): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(OBJS) -l:libraylib.a -lm $(LDFLAGS) -o $@

clean:
	$(RM) *.o

WINDOWS_OBJS = winmain.o

# Will work with MinGW. MSVC needs raylib.lib instead of libraylib.a
$(NAME).exe: $(OBJS) $(WINDOWS_OBJS) $(HDRS)
	$(CC) -mwindows $(CFLAGS) $(OBJS) $(WINDOWS_OBJS) -l:libraylib.a -lm -lgdi32 -lopengl32 -lwinmm $(LDFLAGS) -o $@
