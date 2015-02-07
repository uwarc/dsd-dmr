/*
 * Copyright (C) 2010 DSD Author
 * GPG Key ID: 0x3F1D7FD0 (74EF 430D F7F2 0A48 FCE6  F630 FAA2 635D 3F1D 7FD0)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "dsd.h"

int
openAudioInDevice (dsd_opts *opts, const char *audio_in_dev)
{
	if((audio_in_dev[0] == '-') && (audio_in_dev[1] == '\0')) {
      opts->audio_in_type = 0;
      opts->audio_in_fd = STDIN_FILENO;
	} else {
      struct stat st;
      opts->audio_in_type = 1;
      opts->audio_in_fd = open(audio_in_dev, O_RDONLY);
      if(opts->audio_in_fd < 0) {
          return -1;
      }
      if (stat(audio_in_dev, &st) < 0) {
        return -2;
      }
      if (S_ISFIFO(st.st_mode)) {
        opts->audio_in_type = 0;
      }
  }

  return 0;
}

