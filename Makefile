# DIRECTORIES
EXEC = ./dsd

# SETTINGS
CC = gcc
CCCFLAGS = -Wall -Wextra -MMD
LDLIBS = -lmbe -lm

# Removes one of the dsd_symbol methods
# Use dsd_symbol.c for pipes
# Use dsd_symbol2.c for reading from file
DSDSYMBOL_REMOVE = dsd_symbol2.c

# FILES
CCFILES = $(wildcard *.c)
CCOBJECTS = $(subst ${DSDSYMBOL_REMOVE:.c=.o},,${CCFILES:.c=.o})
CCDEPENDS = ${CCOBJECTS:.o=.d}

# MAKE METHODS
${EXEC}: ${CCOBJECTS}
	${CC} ${CCFLAGS} ${LDFLAGS} ${CCOBJECTS} -o ${EXEC} ${LDLIBS}

-include ${CCDEPENDS}

.PHONY: build clean

build: ${EXEC}

clean:
	@echo "Cleaning..."
	@-rm -f *.d
	@-rm -f *.o
