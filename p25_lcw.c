#include <stdint.h>
#include <stdio.h>

static char *lcwids[19] = {
    "Group Voice Channel User",
    "Unknown (1)",
    "Group Voice Channel Update",
    "Unit to Unit Voice Channel User",
    "Group Voice Channel Update - Explicit",
    "Unit to Unit Answer Request",
    "Telephone Interconnect Voice Channel User",
    "Telephone Interconnect Answer Request",

    "Call Termination/Cancellation", // +7
    "Group Affiliation Query",
    "Unit Registration Command",
    "Unknown (18)",
    "Status Query",
    "Status Update",
    "Message Update",
    "Call Alert",
    "Extended Function Command",
    "Channel Identifier Update",
    "Channel Identifier Update - Explicit",
};

unsigned int decode_p25_lcf(unsigned int lcinfo[3], char *strbuf)
{
  unsigned char lcformat = (lcinfo[0] & 0xFF);
  unsigned int channel = 0;
  unsigned int len = 0;
  lcinfo[0] >>= 8;

  if (lcformat == 0) {
    len = snprintf (strbuf, 1023, "LCW: Group Voice Ch User%s, talkgroup: %u, radio id: %u (session mode: %s)\n",
            ((lcinfo[1] & 0x00800000) ? " Emergency" : ""),  (lcinfo[1] & 0xffff), lcinfo[2],
            ((lcinfo[1] & 0x00100000) ? "Circuit" : "Packet"));
  } else if (lcformat == 1) {
    len = snprintf (strbuf, 1023, "LCW: Reserved (1)\n");
  } else if (lcformat == 2) {
    channel = (((lcinfo[1] & 0x0f) << 16) | (lcinfo[2] >> 16));
    len = snprintf (strbuf, 1023, "LCW: Group Voice Ch Update, Channel A: 0x%02x/0x%06x, Group Address A: 0x%04x",
            ((lcinfo[0] >> 12) & 0x0f), (lcinfo[0] & 0x0fff), (lcinfo[1] >> 8));
    if (channel) {
        len += snprintf (strbuf+len, 1023-len, " Channel A: 0x%02x/0x%06x, Group Address A: 0x%04x",
                   ((lcinfo[1] >> 4) & 0x0f), channel, (lcinfo[2] & 0xffff));
    }
    strbuf[len++] = '\n';
  } else if (lcformat == 3) {
    len = snprintf (strbuf, 1023, "LCW: Unit-to-Unit Voice Ch Update, from %u to %u\n", lcinfo[2], lcinfo[1]);
  } else if (lcformat == 4) {
    channel = (((lcinfo[1] & 0x0f) << 16) | (lcinfo[2] >> 16));
    len = snprintf (strbuf, 1023, "LCW: Group Voice Ch Update (Explicit), Group Address: 0x%04x, TX Channel: 0x%02x/0x%06x, RX Channel: 0x%02x/0x%06x\n",
            (lcinfo[1] >> 8), ((lcinfo[1] >> 4) & 0x0f), channel, ((lcinfo[2] >> 12) & 0x0f), (lcinfo[2] & 0x0fff));
  } else if (lcformat == 5) {
    len = snprintf (strbuf, 1023, "LCW: Unit-to-Unit Answer Request, from %u to %u\n", lcinfo[2], lcinfo[1]);
  } else if (lcformat == 6) {
    channel = ((lcinfo[1] & 0xff) / 10);
    len = snprintf (strbuf, 1023, "LCW: Telephone Interconnect Voice Ch User %s, address: %u, timer: %u secs\n",
                    ((lcinfo[1] & 0x00800000) ? "Emergency" : ""), lcinfo[2], channel);
  } else if (lcformat == 7) {
    len = snprintf (strbuf, 1023, "LCW: Telephone Interconnect Answer Request, (%u%u%u) %u%u%u-%u%u%u%u to %u\n",
                    ((lcinfo[0] >> 12) & 0x0f), ((lcinfo[0] >> 8) & 0x0f), ((lcinfo[0] >> 4) & 0x0f), ((lcinfo[0] >> 0) & 0x0f),
                    ((lcinfo[1] >> 20) & 0x0f), ((lcinfo[1] >> 16) & 0x0f), ((lcinfo[1] >> 12) & 0x0f),
                    ((lcinfo[1] >>  8) & 0x0f), ((lcinfo[1] >>  4) & 0x0f), ((lcinfo[1] >>  0) & 0x0f),
                      lcinfo[2]);
  } else if (lcformat == 15) {
    len = snprintf (strbuf, 1023, "LCW: Call Termination or Cancellation, radio id: %u\n", lcinfo[2]);
  } else if (lcformat == 16) {
    len = snprintf (strbuf, 1023, "LCW: Group Affiliation Query, from %u to %u\n", lcinfo[2], lcinfo[1]);
  } else if (lcformat == 17) {
    len = snprintf (strbuf, 1023, "LCW: Unit Registration, unit %u registering on system %u, network %u\n",
                    lcinfo[2], (lcinfo[1] & 0x0fff), (((lcinfo[0] & 0xff) << 12) | (lcinfo[1] >> 12)));
  } else if (lcformat == 19) {
    len = snprintf (strbuf, 1023, "LCW: Status Query, from %u to %u\n", lcinfo[2], lcinfo[1]);
  } else if (lcformat == 20) {
    len = snprintf (strbuf, 1023, "LCW: Status Update, from %u to %u: User Status: 0x%02x, Unit Status: 0x%02x\n",
                    lcinfo[2], lcinfo[1], ((lcinfo[0] >> 8) & 0xff), (lcinfo[0] & 0xff));
  } else if (lcformat == 21) {
    len = snprintf (strbuf, 1023, "LCW: Message Update: 0x%04x (talkgroup: %u, radio_id: %u)\n",
                    (lcinfo[0] & 0xffff), lcinfo[1], lcinfo[2]);
  } else if (lcformat == 22) {
    len = snprintf (strbuf, 1023, "LCW: Call Alert, from %u to %u\n", lcinfo[2], lcinfo[1]);
  } else if (lcformat == 23) {
    unsigned int extended_function_command = (lcinfo[0] & 0xffff);
    len = snprintf (strbuf, 1023, "LCW: Extended Function 0x%04x, arg: 0x%06x, target: %u\n",
                    extended_function_command, lcinfo[1], lcinfo[2]);
  } else if (lcformat == 24) {
    long offset = 250000L * (((lcinfo[0] << 6) & 0x07) | (lcinfo[1] >> 18));
    unsigned long base_freq = 5 * (((lcinfo[1] & 0xff) << 24UL) | lcinfo[2]);
    len = snprintf (strbuf, 1023, "LCW: Channel Identifier Update: Id: %u, Frequency: %lu%ld, Bandwidth: %ukHz, Channel Spacing: %ukHz\n",
            ((lcinfo[0] >> 12) & 0x0f), base_freq, -offset, (((lcinfo[0] >> 3) & 0x1ff) >> 3), ((lcinfo[1] >> 8) & 0x3ff));
  } else if (lcformat == 25) {
    long offset = 250000LL * (((lcinfo[0] << 6) & 0x3fff) | (lcinfo[1] >> 18));
    unsigned long base_freq = 5LL * (((lcinfo[1] & 0xff) << 24UL) | lcinfo[2]);
    len = snprintf (strbuf, 1023, "LCW: Channel Identifier Update Explicit: Id: %u, Frequency: %lu%ld, Bandwidth: %ukHz, Channel Spacing: %ukHz\n",
            ((lcinfo[0] >> 12) & 0x0f), base_freq, -offset, (((lcinfo[0] >> 8) & 0x0f) >> 3), ((lcinfo[1] >> 8) & 0x3ff));
  } else  {
    len = snprintf (strbuf, 1023, "LCW: Unknown (lcformat: 0x%02x), hexdump: 0x%04x, 0x%06x, 0x%06x\n",
                    lcformat, lcinfo[0], lcinfo[1], lcinfo[2]);
  }
  return len;
}

