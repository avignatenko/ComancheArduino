
#define SIM

#ifdef SIM
#include <si_message_port.hpp>

SiMessagePort* messagePort;
#endif


#include "LiquidCrystal_PCF8574.h"

// own setup:
// display 1
// dsp 1 = sda = 12  --- port D27
// dsp 2 = scl = 13  --- port D19
// display 2
// dsp 1 = sda = 12  --- port D45
// dsp 2 = scl = 13  --- port D37

// encoders
// encoder 1
//  A0 - 3 -- port 14
//  B0 - 5 -- port 15
//  A1 - 4 -- port 23
//  A2 - 6 -- port 24
// encoder 2
//  A0 - 3 -- port 32
//  B0 - 5 -- port 33
//  A1 - 4 -- port 41
//  A2 - 6 -- port 42


enum Messages  {
  REFRESH_STATE = 0, // payload: none
  ENABLE_DISPLAY_BACKLIGHT = 1, // payload: <int 0 (left), 1 (right)> <int 0 (disable), 1 (enable)>
  SET_DISPLAY_TEXT = 2, // payload: <int 0 (left), 1 (right)> <int 0 (line 1), 1 (line 2)> || <string text>
};


// A4 as SDA, A5 as SCL
LiquidCrystal_PCF8574* lcds[2];

void set_backlight(int dspl, bool enable) {
   lcds[dspl]->setBacklight(enable ? 255 : 0);
}


void set_display_text(int dspl, int line, const char* text) {
    lcds[dspl]->setCursor(0, line);
    lcds[dspl]->print(text);
}

#ifdef SIM

static int s_dspl = 0; // selected display
static int s_line = 0; // selected line

static void new_message_callback(uint16_t message_id, struct SiMessagePortPayload* payload) {
  // Do something with a message from Air Manager or Air Player

  // The arguments are only valid within this function!
  // Make a clone if you want to store it
  messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Received something with id " + message_id);
  
  switch (message_id)
  {
    case REFRESH_STATE: break;
    case ENABLE_DISPLAY_BACKLIGHT: {
        int dspl = payload->data_int[0];
        int enable = payload->data_int[1];
        set_backlight(dspl, enable);
        messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Enable display " + dspl + " " + enable);
 
        break;
      }
    case SET_DISPLAY_TEXT: {
        switch (payload->type) {
          case SI_MESSAGE_PORT_DATA_TYPE_INTEGER: {
              int dspl = payload->data_int[0];
              int line = payload->data_int[1];
              s_dspl = dspl;
              s_line = line;
              break;
            }
          case SI_MESSAGE_PORT_DATA_TYPE_STRING: {
              const char* text = payload->data_string;
              set_display_text(s_dspl, s_line, text);
              break;
            }
        }
        break;
      }
  }

}
#endif


void setup()
{
#ifdef SIM
  // Init library on channel A and Arduino type MEGA 2560
  messagePort = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_MEGA_2560, SI_MESSAGE_PORT_CHANNEL_H, new_message_callback);
#else
  Serial.begin(9600);
#endif

  lcds[0] = new LiquidCrystal_PCF8574(0x26, 45, 37); // set the LCD address to 0x27 for a 16 chars and 2 line display
  lcds[1] = new LiquidCrystal_PCF8574(0x27, 27, 19); // set the LCD address to 0x27 for a 16 chars and 2 line display

  lcds[0]->begin(16, 2); // initialize the lcd
  lcds[1]->begin(16, 2); // initialize the lcd

  for (int i = 0; i < 2; ++i) {
    lcds[i]->begin(16, 2); // initialize the lcd
    lcds[i]->setBacklight(0);
    lcds[i]->setCursor(0, 0);
    lcds[i]->print("188.00   188.88");
    lcds[i]->setCursor(0, 1);
    lcds[i]->print("nm kt min hold   loc to fr");

  }

} // setup()


void loop()
{
#ifdef SIM
  // Make sure this function is called regularly
  messagePort->Tick();
#endif

} // loop()
