#include <stdint.h>
#include <stdio.h>

static char *lcwids[32] = {
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

    "System Service Broadcast", // +14

    "Secondary Control Channel Broadcast",
    "Adjacent Site Status Broadcast",
    "RFSS Status Broadcast",
    "Network Status Broadcast",

    "Unknown (36)",

    "Secondary Control Channel Broadcast - Explicit",
    "Adjacent Site Status Broadcast - Explicit",
    "RFSS Status Broadcast - Explicit",
    "Network Status Broadcast - Explicit"
};

unsigned int decode_p25_lcf(unsigned int lcinfo[3], char *strbuf)
{
  //unsigned char lcformat = (lcinfo[0] & 0xFF);
  unsigned char lcformat = (lcinfo[0] >> 16);
  unsigned int talkgroup = lcinfo[1];
  unsigned int radio_id = lcinfo[2];
  unsigned int channel = 0;
  unsigned int len = 0;
  //lcinfo[0] >>= 8;
  lcinfo[0] &= 0xFFFF;

  if (lcformat == 0) {
    len = snprintf (strbuf, 1023, "LCW: Group Voice Ch User%s, talkgroup: %u, radio id: %u (session mode: %s)\n",
            ((lcinfo[1] & 0x00800000) ? " Emergency" : ""),  (talkgroup & 0xffff), radio_id,
            ((lcinfo[1] & 0x00100000) ? "Circuit" : "Packet"));
  } else if (lcformat == 1) {
    len = snprintf (strbuf, 1023, "LCW: Reserved (1)\n");
  } else if (lcformat == 2) {
    channel = (((talkgroup & 0x0f) << 16) | (radio_id >> 16));
    len = snprintf (strbuf, 1023, "LCW: Group Voice Ch Update, Channel A: 0x%02x/0x%06x, Group Address A: 0x%04x",
            ((lcinfo[0] >> 12) & 0x0f), (lcinfo[0] & 0x0fff), (talkgroup >> 8));
    if (channel) {
        len += snprintf (strbuf+len, 1023-len, " Channel A: 0x%02x/0x%06x, Group Address A: 0x%04x",
                   ((talkgroup >> 4) & 0x0f), channel, (radio_id & 0xffff));
    }
    strbuf[len++] = '\n';
  } else if (lcformat == 3) {
    len = snprintf (strbuf, 1023, "LCW: Unit-to-Unit Voice Ch Update, from %u to %u\n", radio_id, talkgroup);
  } else if (lcformat == 4) {
    channel = (((talkgroup & 0x0f) << 16) | (radio_id >> 16));
    len = snprintf (strbuf, 1023, "LCW: Group Voice Ch Update (Explicit), Group Address: 0x%04x, TX Channel: 0x%02x/0x%06x, RX Channel: 0x%02x/0x%06x\n",
            (talkgroup >> 8), ((talkgroup >> 4) & 0x0f), channel, ((radio_id >> 12) & 0x0f), (radio_id & 0x0fff));
  } else if (lcformat == 5) {
    len = snprintf (strbuf, 1023, "LCW: Unit-to-Unit Answer Request, from %u to %u\n", radio_id, talkgroup);
  } else if (lcformat == 6) {
    channel = ((talkgroup & 0xff) / 10);
    len = snprintf (strbuf, 1023, "LCW: Telephone Interconnect Voice Ch User %s, address: %u, timer: %u secs\n",
            ((lcinfo[1] & 0x00800000) ? "Emergency" : ""), radio_id, channel);
  } else if (lcformat == 7) {
    len = snprintf (strbuf, 1023, "LCW: Telephone Interconnect Answer Request, (%u%u%u) %u%u%u-%u%u%u%u to %u\n",
            ((lcinfo[1] >> 12) & 0x0f), ((lcinfo[1] >> 8) & 0x0f), ((lcinfo[1] >> 4) & 0x0f), ((lcinfo[1] >> 0) & 0x0f),
            ((talkgroup >> 20) & 0x0f), ((talkgroup >> 16) & 0x0f), ((talkgroup >> 12) & 0x0f),
            ((talkgroup >>  8) & 0x0f), ((talkgroup >>  4) & 0x0f), ((talkgroup >>  0) & 0x0f),
            radio_id);
  } else if (lcformat == 15) {
    len = snprintf (strbuf, 1023, "LCW: Call Termination or Cancellation, radio id: %u\n", radio_id);
  } else if (lcformat == 16) {
    len = snprintf (strbuf, 1023, "LCW: Group Affiliation Query, from %u to %u\n", radio_id, talkgroup);
  } else if (lcformat == 17) {
    len = snprintf (strbuf, 1023, "LCW: Unit Registration, unit %u registering on system %u, network %u\n",
            radio_id, (talkgroup & 0x0fff), (((lcinfo[0] & 0xff) << 12) | (talkgroup >> 12)));
  } else if (lcformat == 19) {
    len = snprintf (strbuf, 1023, "LCW: Status Query, from %u to %u\n", radio_id, talkgroup);
  } else if (lcformat == 20) {
    len = snprintf (strbuf, 1023, "LCW: Status Update, from %u to %u: User Status: 0x%02x, Unit Status: 0x%02x\n",
            radio_id, talkgroup, ((lcinfo[0] >> 8) & 0xff), (lcinfo[0] & 0xff));
  } else if (lcformat == 21) {
    len = snprintf (strbuf, 1023, "LCW: Message Update: 0x%04x (talkgroup: %u, radio_id: %u)\n",
            (lcinfo[0] & 0xffff), talkgroup, radio_id);
  } else if (lcformat == 22) {
    len = snprintf (strbuf, 1023, "LCW: Call Alert, from %u to %u\n", radio_id, talkgroup);
  } else if (lcformat == 23) {
    unsigned int extended_function_command = (lcinfo[0] & 0xffff);
    len = snprintf (strbuf, 1023, "LCW: Extended Function 0x%04x, arg: 0x%06x, target: %u\n", extended_function_command, talkgroup, radio_id);
  } else if (lcformat == 24) {
    long offset = 250000L * (((lcinfo[0] << 6) & 0x07) | (talkgroup >> 18));
    unsigned long base_freq = 5 * (((talkgroup & 0xff) << 24UL) | radio_id);
    len = snprintf (strbuf, 1023, "LCW: Channel Identifier Update: Id: %u, Frequency: %lu%ld, Bandwidth: %ukHz, Channel Spacing: %ukHz\n",
            ((lcinfo[0] >> 12) & 0x0f), base_freq, -offset, (((lcinfo[0] >> 3) & 0x1ff) >> 3), ((talkgroup >> 8) & 0x3ff));
  } else if (lcformat == 25) {
    long offset = 250000LL * (((lcinfo[0] << 6) & 0x3fff) | (talkgroup >> 18));
    unsigned long base_freq = 5LL * (((talkgroup & 0xff) << 24UL) | radio_id);
    len = snprintf (strbuf, 1023, "LCW: Channel Identifier Update: Id: %u, Frequency: %lu%ld, Bandwidth: %ukHz, Channel Spacing: %ukHz\n",
            ((lcinfo[0] >> 12) & 0x0f), base_freq, -offset, (((lcinfo[0] >> 8) & 0x0f) >> 3), ((talkgroup >> 8) & 0x3ff));
  } else if (lcformat == 32) {
    //request_priority_level = (lcinfo[0] & 0x0f);
    len = snprintf (strbuf, 1023, "LCW: System Service Broadcast: Services Available: 0x%06x, Services Supported: 0x%06x\n",
            talkgroup, radio_id);
  } else if (lcformat == 33) {
    unsigned char cctype1 = ((talkgroup & 0x01) ? 'C' : ((talkgroup & 0x02) ? 'U' : 'B')); 
    unsigned char cctype2 = ((radio_id & 0x01) ? 'C' : ((radio_id & 0x02) ? 'U' : 'B')); 
    len = snprintf (strbuf, 1023, "LCW: Secondary Control Channel Broadcast: RFSSId: 0x%02x, SiteId: 0x%02x:"
            "Channel A: 0x%02x/0x%06x ControlChannel(%c): %c%c%c%c -> Channel B: 0x%02x/0x%06x ControlChannel(%c): %c%c%c%c\n",
            ((lcinfo[0] >> 8) & 0xff), ((lcinfo[0] >> 0) & 0xff),
            (talkgroup >> 2), ((talkgroup >> 8) & 0x0fff), cctype1,
            ((talkgroup & 0x10) ? 'A' : ' '), ((talkgroup & 0x20) ? 'D' : ' '),
            ((talkgroup & 0x40) ? 'R' : ' '), ((talkgroup & 0x80) ? 'V' : ' '),
            (radio_id >> 2), ((radio_id >> 8) & 0x0fff), cctype2,
            ((radio_id & 0x10) ? 'A' : ' '), ((radio_id & 0x20) ? 'D' : ' '),
            ((radio_id & 0x40) ? 'R' : ' '), ((radio_id & 0x80) ? 'V' : ' '));
  } else if ((lcformat == 34) || (lcformat == 35) || (lcformat == 39) || (lcformat == 40)) {
    unsigned char cctype2 = ((radio_id & 0x01) ? 'C' : ((radio_id & 0x02) ? 'U' : 'B')); 
    len = snprintf (strbuf, 1023, "LCW: RFSS or Adjacent Site Broadcast%s: Location Registration Area: 0x%02x, System: 0x%03x,"
            "     Site: RFSSId: 0x%02x, SiteId: 0x%02x, Channel: 0x%02x/0x%06x ControlChannel(%c): %c%c%c%c\n",
            ((lcformat > 35) ? " (Explicit) " : ""), ((lcinfo[0] >> 8) & 0xff),
            (((lcinfo[0] & 0x0f) << 8) | (talkgroup >> 16)),
            ((talkgroup >> 8) & 0xff), ((talkgroup >> 0) & 0xff), (radio_id >> 20),
            ((radio_id >> 8) & 0x0fff), cctype2,
            ((radio_id & 0x10) ? 'A' : ' '), ((radio_id & 0x20) ? 'D' : ' '),
            ((radio_id & 0x40) ? 'R' : ' '), ((radio_id & 0x80) ? 'V' : ' '));
  } else if (lcformat == 36) {
    unsigned char cctype2 = ((radio_id & 0x01) ? 'C' : ((radio_id & 0x02) ? 'U' : 'B')); 
    len = snprintf (strbuf, 1023, "LCW: Network Status Update: NetworkID: %u, SystemID: %u Channel: 0x%02x/0x%06x ControlChannel(%c): %c%c%c%c\n",
            (((lcinfo[0] & 0xff) << 12) | (talkgroup >> 12)), (talkgroup & 0x0fff),
            (radio_id >> 20), ((radio_id >> 8) & 0x0fff), cctype2,
            ((radio_id & 0x10) ? 'A' : ' '), ((radio_id & 0x20) ? 'D' : ' '),
            ((radio_id & 0x40) ? 'R' : ' '), ((radio_id & 0x80) ? 'V' : ' '));
  } else if (lcformat == 41) {
    len = snprintf (strbuf, 1023, "LCW: Network Status Update (Explicit): NetworkID: %u, SystemID: %u"
            "RX Channel: 0x%02x/0x%06x TX Channel: 0x%02x/0x%06x\n",
            (((lcinfo[0] & 0xffff) << 4) | (talkgroup >> 12)), ((talkgroup >> 8) & 0x0fff),
            ((radio_id >> 12) & 0x0f), (radio_id & 0x0fff),
            ((talkgroup >> 4) & 0x0f), (((talkgroup & 0x0f) << 8) | (radio_id >> 16)));
  } else  {
    len = snprintf (strbuf, 1023, "LCW: Unknown (lcformat: 0x%02x), hexdump: 0x%04x, 0x%06x, 0x%06x\n",
            lcformat, lcinfo[0], lcinfo[1], lcinfo[2]);
  }
  return len;
}

