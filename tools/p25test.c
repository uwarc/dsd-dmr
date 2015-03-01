#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "dsd.h"

typedef struct _p25test_state {
    unsigned char *dibit_buf;
    unsigned int byte_alloc, byte_offset;
    unsigned int dibit_offset;
} p25test_state;

static p25test_state s;

unsigned int getDibit(dsd_opts* opts, dsd_state* state)
{
    unsigned int dibit, offset;
    //printf("dibit_buf[%u]: 0x%02x\n", state->byte_offset, state->dibit_buf[state->byte_offset]);
    if (s.dibit_offset == 4) {
        s.byte_offset++;
        s.dibit_offset = 0;
    }
    offset = (s.dibit_offset << 1);
    dibit = ((s.dibit_buf[s.byte_offset] >> (6 - offset)) & 0x03);
    s.dibit_offset++;
    return dibit;
}

void
skipDibit (dsd_opts * opts, dsd_state * state, int count)
{
  int i;

  for (i = 0; i < count; i++) {
      getDibit (opts, state);
  }
}

static const char *p25frametypes[16] = {
    "HDU",
    "????",
    "????",
    "TDU",
    "????",
    "LDU1",
    "????",
    "TSDU",
    "????",
    "????",
    "LDU2",
    "????",
    "PDU",
    "????",
    "????",
    "TDULC"
};

void p25_test(dsd_opts *opts)
{
    dsd_state state;
    unsigned short nac = 0;
    unsigned char duid = 0;
    uint64_t syncword = 0;
    unsigned int i, dibit;

    for (i = 0; i < 24; i++) {
        syncword <<= 2;
        syncword |= getDibit (opts, &state);
    }

    if (syncword != 0x5575f5ff77ffUL) {
        printf("Sync Error.\n");
        return;
    }

    skipDibit (opts, &state, 6);

    // Read the DUID, 4 bits
    dibit = getDibit (opts, &state);
    duid  = (dibit << 2);
    dibit = getDibit (opts, &state);
    duid |= (dibit << 0);

    skipDibit (opts, &state, 10);
    skipDibit (opts, &state, 15);

    printf ("p25 NAC: 0x%03x, DUID: 0x%x -> %s\n",
            nac, duid, p25frametypes[duid]);
    state.duid = duid;
    process_p25_frame(opts, &state);
}

int main(int argc, char **argv)
{
  dsd_opts opts;
  struct stat st;
  int mbe_in_fd = open (argv[1], O_RDONLY);
  if (mbe_in_fd < 0) {
      printf ("Error: could not open %s\n", argv[1]);
      return -1;
  }
  fstat(mbe_in_fd, &st);
  s.dibit_offset = 0;
  s.byte_offset = 0;
  s.byte_alloc = st.st_size;
  s.dibit_buf = malloc(st.st_size+1);
  read(mbe_in_fd, s.dibit_buf, s.byte_alloc);
  close(mbe_in_fd);

  opts.errorbars = 1;
  opts.verbose = 9001;
  opts.p25enc = 1;
  opts.p25status = 1;
  opts.p25tg = 1;
  opts.mbe_out_dir[0] = 0;
  opts.mbe_out_path[0] = 0;
  opts.mbe_out_fd = -1;
  s.byte_alloc--;

  while (s.byte_offset < s.byte_alloc) {
    p25_test(&opts);
  }

  return 0;
}

