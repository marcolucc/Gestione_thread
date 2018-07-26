# Sources
SRCS= elaborato.c
OBJS=$(SRCS:.c=.o)
EXECUTABLE=elaborato.x

# Config
CC=gcc
CFLAGS= -c
LDFLAGS= -pthread 
LD=gcc

# Target

all: $(EXECUTABLE)  
	@echo Building $(EXECUTABLE)


$(EXECUTABLE): $(OBJS)
	$(CC) -o $@ $<

$(OBJS): $(SRCS) 
	#$(CC) $(CFLAGS) $(LDFLAGS) -o $(OBJS) $< 
	$(CC) $(OBJS) -o $@ $(LDFLAGS)


clean:
	@rm -f $(EXECUTABLE)
	@rm -f $(OBJS)
	
.PHONY: all clean
