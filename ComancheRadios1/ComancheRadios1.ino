#include <Encoder.h>
#include <Bounce2.h>

#define SIM

#ifdef SIM
#include <si_message_port.hpp>
SiMessagePort* messagePort;

// upper - H
// lower - E
SiMessagePortChannel kChannel = SI_MESSAGE_PORT_CHANNEL_E;

#endif

#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

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

enum Flags
{
  NM = (1 << 0),
  KT = (1 << 1),
  MIN = (1 << 2),
  HLD = (1 << 3),
  LOC = (1 << 4),
  TO = (1 << 5),
  FR = (1 << 6)
};

enum Messages {
  REFRESH_STATE = 0, // payload: none
  KNOB_UPDATED = 1, //-- payload <int 0 (left), 1 (right) < int 0 (inner), int 1 (outer)> <int -1 - left, 1 right>
  TOGGLE_PRESSED = 2, // payload <int 0 (left), 1 (right)> <int 0 released, 1 pressed>
  ONOFF_PRESSED = 3, // payload <int 0 (left), 1 (right)> <int 0 released, 1 pressed>
  SELECTOR_SET = 4, // payload <mode 0..6>

  DISPLAYS_ENABLE = 5, // payload: <int 0|1>,
  SET_DIGITS = 6, // payload: <int 0 (left), 1 (right)> <int 0 (left), 1 (right)> <int whole> <int frac>
  SET_FLAGS = 7, // payload< <int flags>
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

Bounce* btns_selector[6] = {
  createButton(10),
  createButton(50),
  createButton(11),
  createButton(51),
  createButton(12),
  createButton(52)
};

hd44780_I2Cexp* lcds[2];

float s_digits[2][2] = {{188.00, 188.01}, {188.02, 0.0}};
bool s_digits_refresh[2][2] = {{true, true}, {true, true}};
int s_flags =  NM |  KT | MIN | HLD | LOC | TO | FR;

bool s_flags_refresh = true;

void set_backlight(int dspl, bool enable) {
  lcds[dspl]->setBacklight(enable ? 255 : 0);
}

void set_display_text(int dspl, int line, int pos, const String& text) {
  lcds[dspl]->setCursor(pos, line);
  lcds[dspl]->print(text);
}

void display_enable(int dspl, bool enable)
{
  set_backlight(0, enable);
  set_backlight(1, enable);

  if (!enable) {
    set_display_text(0, 0, 0, "                ");
    set_display_text(0, 1, 0, "                ");
    set_display_text(1, 0, 0, "                ");
    set_display_text(1, 1, 0, "                ");
  }
}

#ifdef SIM

static void new_message_callback(uint16_t message_id, struct SiMessagePortPayload* payload) {
  // Do something with a message from Air Manager or Air Player

  // The arguments are only valid within this function!
  // Make a clone if you want to store it
  //messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Received something with id " + message_id);

  switch (message_id)
  {
    case REFRESH_STATE: break;
    case DISPLAYS_ENABLE: {
        int enable = payload->data_int[0];
        display_enable(0, enable);
        display_enable(1, enable);

        break;
      }

    case SET_DIGITS: {
        int dspl = payload->data_int[0];
        int side = payload->data_int[1];
        int number = payload->data_int[2];
        int number_frac = payload->data_int[3];

        s_digits_refresh[dspl][side] = true;
        s_digits[dspl][side] = number + number_frac / 100.0;
        break;
      }

    case SET_FLAGS: {
        s_flags_refresh = true;
        s_flags = payload->data_int[0];
        break;
      }

  }
}
#endif


void setup()
{
#ifdef SIM
  // Init library on channel A and Arduino type MEGA 2560
  messagePort = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_MEGA_2560, kChannel, new_message_callback);
#else
  Serial.begin(9600);
#endif

  Wire.setClock(400000L);

  lcds[0] = new hd44780_I2Cexp(0x26); // set the LCD address to 0x27 for a 16 chars and 2 line display
  lcds[1] = new hd44780_I2Cexp(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display

  for (int i = 0; i < 2; ++i) {
    lcds[i]->begin(16, 2); // initialize the lcd
  }

  display_enable(0, false);
  display_enable(1, false);

  // setup knobs
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      knobs_positions[i][j] = knobs[i][j]->read() / 2;
    }
  }
} // setup()


void loop()
{

  // update knobs

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

  // update buttons

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

  // check buttons for selector
  for (int i = 0; i < 6; ++i) {
    bool updated = btns_selector[i]->update();
    if (updated && !btns_selector[i]->read()) {
#ifdef SIM
      messagePort->SendMessage(SELECTOR_SET, (int32_t)i);
#else
      Serial.print("Selector ");
      Serial.print(i);
      Serial.println();
#endif
    }
  }

  // update monitor
  if (s_flags_refresh) {

    // group with pos = 0
    if (s_flags & NM) set_display_text(1, 1, 0, "nm");
    if (s_flags & KT) set_display_text(1, 1, 0, "kt");
    if (s_flags & MIN) set_display_text(1, 1, 0, "min");
    if ((s_flags & (NM|KT|MIN)) == 0) set_display_text(1, 1, 0, "   ");    

    set_display_text(1, 1, 4, (s_flags & HLD) ? "hold" : "    ");
    set_display_text(1, 1, 10, (s_flags & LOC) ? "loc" : "   ");

    // group with pos = 14
    if (s_flags & TO) set_display_text(1, 1, 14, "to");
    if (s_flags & FR) set_display_text(1, 1, 14, "fr");
    if ((s_flags & (TO|FR)) == 0) set_display_text(1, 1, 14, "  ");    

    s_flags_refresh = false;
  }

  for (int dspl = 0; dspl < 2; ++dspl)
    for (int part = 0; part < 2; ++part) {
      if (!s_digits_refresh[dspl][part])
        continue;

      String digitText = (s_digits[dspl][part] == 0.0 ? "  --- " : String(s_digits[dspl][part], 2));
      set_display_text(dspl, 0, part * 10, digitText);

      s_digits_refresh[dspl][part] = false;
    }

#ifdef SIM
  // Make sure this function is called regularly
  messagePort->Tick();
#endif

} // loop()
