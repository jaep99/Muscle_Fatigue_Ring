/*
Program to control AD9833 and MCP4131
Adapted for Arduino Nano
AD9833 connected to FSYNC=5, MOSI=11, SCK=13
MCP4131 connected to CS=10, MOSI=11, SCK=13
*/

#include <SPI.h>
#include <MD_AD9833.h>
#include <MCP4131.h>

// ---- Pin Mapping for Arduino Nano ----
#define DATA 11   // Nano MOSI
#define CLK 13    // Nano SCK
#define FSYNC 5   // AD9833 FSYNC (any digital pin)

// MCP4131 uses hardware SPI:
const int chipSelect = 10;

long frequency;
unsigned int wiperValue;

MD_AD9833 AD(DATA, CLK, FSYNC);
MCP4131 Potentiometer(chipSelect);

const char CMD_HELP = '?';
const char BLANK = ' ';
const char PACKET_START = ':';
const char PACKET_END = ';';
const char CMD_FREQ = 'F';
const char CMD_PHASE = 'P';
const char CMD_OUTPUT = 'O';
const char CMD_GAIN = 'G';
const char OPT_FREQ = 'F';
const char OPT_PHASE = 'P';
const char OPT_SIGNAL = 'S';
const char OPT_1 = '1';
const char OPT_2 = '2';
const char OPT_GAIN = 'A';
const uint8_t PACKET_SIZE = 20;

void setup()
{
  Serial.begin(57600);
  AD.begin();
  usage();
  wiperValue = 64;
  Potentiometer.writeWiper(wiperValue);
}

// Set up menu that displays on Serial Monitor
void usage(void)
{
  Serial.print(F("\n[MD_AD9833_Tester]"));
  Serial.print(F("\n?\thelp - Type ?"));
  Serial.print(F("\n\n:<cmd><opt> <param>;"));
  Serial.print(F("\n:f1n;\tset frequency 1 to n Hz"));
  Serial.print(F("\n:f2n;\tset frequency 2 to n Hz"));
  Serial.print(F("\n:p1n;\tset phase 1 to n in tenths of a degree (1201 is 120.1 deg)"));
  Serial.print(F("\n:p2n;\tset phase 2 to n in tenths of a degree (1201 is 120.1 deg)"));
  Serial.print(F("\n:ofn;\toutput frequency n [n=1/2]"));
  Serial.print(F("\n:opn;\toutput phase n [n=1/2]"));
  Serial.print(F("\n:osn;\toutput signal type n [n=(o)ff/(s)ine/(t)riangle/s(q)uare]"));
  Serial.print(F("\n:gan;\tset gain n 1 to 6"));
}

// Convert to ASCII
uint8_t htoi(char c)
{
  c = toupper(c);

  if (c >= '0' && c <= '9')
    return(c - '0');
  else if (c >= 'A' && c <= 'F')
    return(c - 'A' + 10);
  else
    return(0);
}

char nextChar(void)
// Read the next character from the serial input stream
// Blocking wait
{
  while (!Serial.available())
    ; /* do nothing */
  return(toupper(Serial.read()));
}

char *readPacket(void)
// read a packet and return the contents
{
  static enum { S_IDLE, S_READ_CMD, S_READ_MOD, S_READ_PKT } state = S_IDLE;
  static char cBuf[PACKET_SIZE + 1];
  static char *cp;
  char c;

  switch (state)
  {
    case S_IDLE:   // waiting for packet start
      c = nextChar();
      if (c == CMD_HELP)
      {
        usage();
        break;
      }
      if (c == PACKET_START)
      {
        cp = cBuf;
        state = S_READ_CMD;
      }
      break;

    case S_READ_CMD:   // waiting for command char
      c = nextChar();
      if (c == CMD_FREQ || c == CMD_PHASE || c == CMD_OUTPUT || c == CMD_GAIN)
      {
        *cp++ = c;
        state = S_READ_MOD;
      }
      else
        state = S_IDLE;
      break;

    case S_READ_MOD:   // Waiting for command modifier
      c = nextChar();
      if (c == OPT_FREQ || c == OPT_PHASE || c == OPT_SIGNAL ||
          c == OPT_1 || c == OPT_2 || c == OPT_GAIN)
      {
        *cp++ = c;
        state = S_READ_PKT;
      }
      else
        state = S_IDLE;
      break;

    case S_READ_PKT:   // Reading parameter until packet end
      c = nextChar();
      if (c == PACKET_END)
      {
        *cp = '\0';
        state = S_IDLE;
        return(cBuf);
      }
      *cp++ = c;
      break;

    default:
      state = S_IDLE;
      break;
  }

  return(NULL);
}

void processPacket(char *cp)
// Assume we have a correctly formed packet from the parsing in readPacket()
// Depending on input from Serial Monitor, send commands to AD9833 and MCP4131
{
  uint32_t ul;   // Frequency Value
  uint32_t wV;   // Gain Value
  MD_AD9833::channel_t chan;
  MD_AD9833::mode_t mode;
  
  switch (*cp++)
  {
    case CMD_FREQ:
      switch (*cp++)
      {
        case OPT_1: chan = MD_AD9833::CHAN_0; break;
        case OPT_2: chan = MD_AD9833::CHAN_1; break;
        case OPT_GAIN: break;
      }
      ul = strtoul(cp, NULL, 10);
      // Serial.println(ul); for debug purposes
      AD.setFrequency(chan, ul);
      break;

    case CMD_PHASE:
      switch (*cp++)
      {
        case OPT_1: chan = MD_AD9833::CHAN_0; break;
        case OPT_2: chan = MD_AD9833::CHAN_1; break;
      }
      ul = strtoul(cp, NULL, 10);
      AD.setPhase(chan, (uint16_t)ul);
      break;

    case CMD_GAIN:
      cp++;
      wV = strtoul(cp, NULL, 10);
      // Serial.println(wV); for bebug purposes
      wV = map(wV, 1, 5, 12, 65);
      // Serial.println(wV); for debug purposes
      Potentiometer.writeWiper(wV);
      break;

    case CMD_OUTPUT:
      switch (*cp++)
      {
        case OPT_FREQ:
          switch (*cp)
          {
            case OPT_1: chan = MD_AD9833::CHAN_0; break;
            case OPT_2: chan = MD_AD9833::CHAN_1; break;
          }
          AD.setActiveFrequency(chan);
          break;

        case OPT_PHASE:
          switch (*cp)
          {
            case OPT_1: chan = MD_AD9833::CHAN_0; break;
            case OPT_2: chan = MD_AD9833::CHAN_1; break;
          }
          AD.setActivePhase(chan);
          break;

        case OPT_SIGNAL:
          switch (*cp)
          {
            case 'O': mode = MD_AD9833::MODE_OFF; break;
            case 'S': mode = MD_AD9833::MODE_SINE; break;
            case 'T': mode = MD_AD9833::MODE_TRIANGLE; break;
            case 'Q': mode = MD_AD9833::MODE_SQUARE1; break;
          }
          AD.setMode(mode);
          break;
      }
      break;
  }

  return;
}


void loop()
{
  char *cp;

  if ((cp = readPacket()) != NULL)
    processPacket(cp);
}