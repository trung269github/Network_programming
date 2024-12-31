# Define the compiler
CC=gcc

# Define any compile-time flags
CFLAGS=

# Define any directories containing header files
INCLUDES=

# Define any libraries to link into executable
LIBS=

# Define the source files
SRCS=server.c FTP_Server.c

# Define the object files
OBJS=$(SRCS:.c=.o)

# Define the executable file
MAIN=ftpserver

.PHONY: depend clean

all:    $(MAIN)
	@echo  FTP server has been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^
