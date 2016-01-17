# SETTINGS
CC = gcc
CFLAGS += -Wall -Wextra -MMD -O2 -fno-math-errno -fno-trapping-math -fno-omit-frame-pointer -fno-asynchronous-unwind-tables -fwrapv
LDLIBS = -lmbe -lm

# FILES
CCFILES = dsd_main.c dsd_audio.c dsd_datascope.c dsd_dibit.c dsd_file.c dsd_frame.c dsd_frame_sync.c dsd_4fsk.c dmr_data.c dmr_voice.c dstar.c nxdn.c p25p1.c bch.c fec.c
CCOBJECTS = ${CCFILES:.c=.o}
CCDEPENDS = ${CCOBJECTS:.o=.d}

# MAKE METHODS
dsd: ${CCOBJECTS}
	${CC} ${LDFLAGS} ${CCOBJECTS} -o dsd ${LDLIBS}

-include ${CCDEPENDS}

.PHONY: build clean dsd

build: dsd

clean:
	@echo "Cleaning..."
	@-rm -f *.d
	@-rm -f *.o
