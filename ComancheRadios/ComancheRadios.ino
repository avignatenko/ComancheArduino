#include <Encoder.h>
#include <Bounce2.h>

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
  KNOB_UPDATED = 3, // payload <int 0 (left), 1 (right) > <int 0 (inner), int 1 (outer)> <int -1 - left, 1 right>
  TOGGLE_PRESSED = 4, // payload <int 0 (left), 1 (right)> <int 0 released, 1 pressed>
  ONOFF_PRESSED = 5, // payload <int 0 (left), 1 (right)> <int 0 released, 1 pressed>
};

// setup encoders

int32_t knobs_positions[2][2] = {0};

Encoder* knobs[2][2] = {
  {new Encoder(14, 15), new Encoder(23, 24)},
  {new Encoder(32, 33), new Encoder(41, 42)}
};

// setup buttons

Bounce* createButton(int pin) {
  pinMode(pin, INPUT_PULLUP);
  Bounce* btn = new Bounce();
  btn->attach(pin);
  btn->interval(5); // interval in ms
  return btn;
};

Bounce* btns_toggle[2] = {
  createButton(36),
  createButton(18),
};

Bounce* btns_onoff[2] = {
  createButton(44),
  createButton(26),

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

  // setup knobs
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      knobs_positions[i][j] = knobs[i][j]->read() / 2;
    }
  }
} // setup()


void loop()
{
  // updat knobs
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      int32_t new_pos = knobs[i][j]->read() / 2;
      if (new_pos != knobs_positions[i][j]) {
        //Serial.print(String("knob ") + i + " " + j + " new pos: " + new_pos);
        //Serial.println();
#ifdef SIM
        bool cw = (new_pos > knobs_positions[i][j]);
        int32_t payload[3] = { i, j,  cw ? 1 : -1};
        messagePort->SendMessage(KNOB_UPDATED, payload, 3);

#endif
        knobs_positions[i][j] = new_pos;

      }
    }
  }

  // updfate buttons

  // check buttons for swap
  for (int i = 0; i < 2; ++i) {
    bool updated = btns_toggle[i]->update();
    if (updated) {
      int32_t payload[2] = {i, !btns_toggle[i]->read()};
#ifdef SIM
      messagePort->SendMessage(TOGGLE_PRESSED, payload, 2);
#else
      Serial.print(i);
#endif
    }
  }

// check buttons for on/off
  for (int i = 0; i < 2; ++i) {
    bool updated = btns_onoff[i]->update();
    if (updated) {
      int32_t payload[2] = {i, !btns_onoff[i]->read()};
#ifdef SIM
      messagePort->SendMessage(ONOFF_PRESSED, payload, 2);
#else
      Serial.print(i);
#endif
    }
  }

#ifdef SIM
  // Make sure this function is called regularly
  messagePort->Tick();
#endif

} // loop()
