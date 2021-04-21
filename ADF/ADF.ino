
#include <Encoder.h>
#include <Bounce2.h>
#include <Adafruit_CharacterOLED.h>

#define SIM

#ifdef SIM
#include <si_message_port.hpp>

SiMessagePort* messagePort;
SiMessagePortChannel kChannel = SI_MESSAGE_PORT_CHANNEL_L;
#endif

enum Messages {
  SET_DISPLAY_ENABLE = 1, // payload: <int 0|1>
  SET_DIGITS = 2, // payload <int 0 left | 1 right> <int value>
  SET_FLAGS = 3, // payload <int flag1> <int flag2>
  SET_ARROWS = 4, // payload: <int 0 left | 1 right> <int blink>

  TRANSFER_BUTTON_PRESSED = 5,
  ONOFF_BUTTON_PRESSED = 6,
  MODE_SWITCH_CHANGED = 7,
  KNOB_UPDATED = 8, //-- payload < int 0 (inner), int 1 (outer)> <int -1 - left, 1 right>
  KNOB_BUTTON_PRESSED = 9 // payload <int 0|1>
};


enum Flags1
{
  ANT = 1,
  ADF = 2,
  BFO = 3
};

enum Flags2
{
  FLT = 4,
  ET = 5,
  FREQ = 6
};

Adafruit_CharacterOLED lcd(OLED_V2, 2, 8, 3, 7, 4, 6, 5);

long knobs_cur_value[2] = { -999, -999};

Encoder* knobs[2] = {new Encoder(9, 10), new Encoder(11, 12)};

Bounce* createButton(int pin) {
  pinMode(pin, INPUT_PULLUP);
  Bounce* btn = new Bounce();
  btn->attach(pin);
  btn->interval(5); // interval in ms
  return btn;
};

Bounce* btn_enc = createButton(13);
Bounce* btn_mode[2] = {createButton(A0), createButton(A1)};
Bounce* btn_on_off = createButton(A2);
Bounce* btn_transfer = createButton(A5);


void display_enable(bool enable) {
  if (enable) lcd.display(); else lcd.noDisplay();
}


void print_digits(int side, int number) {

  char text[4];
  sprintf(text, "%04d", number);

  lcd.setCursor(side == 0 ? 0 : 12, 0);
  lcd.print(text);
}

void print_arrow(int side, bool blink) {

  lcd.setCursor(side == 0 ? 5 : 10, 0);
  lcd.print(side == 0 ? "<" : ">");

  lcd.setCursor(side == 0 ? 10 : 5, 0);
  lcd.print(" ");
}

void print_flags(Flags1 flags1, Flags2 flags2) {

  lcd.setCursor(0, 1);
  switch (flags1)
  {
    case ANT: lcd.print("ANT XXX"); break;
    case ADF: lcd.print("ADF XXX"); break;
    case BFO: lcd.print("ADF BFO"); break;
  }

  lcd.setCursor(11, 1);
  switch (flags2)
  {
    case FLT: lcd.print("FLT"); break;
    case ET: lcd.print(" ET"); break;
    case FREQ: lcd.print("   "); break;

  }
}

#ifdef SIM
static void new_message_callback(uint16_t message_id, struct SiMessagePortPayload* payload) {
  // Do something with a message from Air Manager or Air Player

  // The arguments are only valid within this function!
  // Make a clone if you want to store it
  messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Received something with id " + message_id);

  switch (message_id)
  {
    case SET_DISPLAY_ENABLE: {
        int enable = payload->data_int[0];
        display_enable(enable);
         messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Display enabled: " + enable);

        break;
      }

    case SET_DIGITS: {
        int side = payload->data_int[0];
        int number = payload->data_int[1];
        print_digits(side, number);
        break;
      }

    case SET_FLAGS: {
        int flags1 = payload->data_int[0];
        int flags2 = payload->data_int[1];
        print_flags(static_cast<Flags1>(flags1), static_cast<Flags2>(flags2));
        break;
      }

    case SET_ARROWS: {
        int arrows = payload->data_int[0];
        int blink = payload->data_int[1];
        print_arrow(arrows, blink);
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

  display_enable(false);

}


void loop()
{


  // encoders
  for (int j = 0; j < 2; ++j) {
    long new_pos = knobs[j]->read() / 2;
    if (knobs_cur_value[j] != new_pos) {
#ifdef SIM
      bool cw = (new_pos > knobs_cur_value[j]);
      int32_t payload[3] = { j,  cw ? 1 : -1};
      messagePort->SendMessage(KNOB_UPDATED, payload, 3);
#else
      Serial.println(String("knob ") + j + " new pos: " + new_pos);
#endif
      //print_digits(j, new_pos);
      //print_arrow(j, true);
      knobs_cur_value[j] = new_pos;
    }
  }

  // button enc
  bool btn_enc_updated = btn_enc->update();
  if (btn_enc_updated) {
#ifdef SIM
    int32_t payload[1] = { !btn_enc->read()};
    messagePort->SendMessage(KNOB_BUTTON_PRESSED, payload, 1);
#else
    Serial.println(String("Button: ") + (!btn_enc->read() ? "pressed" : "none"));
#endif
  }

  // mode button
  bool btn_mode_updated[2] = {btn_mode[0]->update(), btn_mode[1]->update()};
  if (btn_mode_updated[0] || btn_mode_updated[1]) {
    int mode = (!btn_mode[0]->read() ? 1 : (!btn_mode[1]->read() ? 2 : 0));
#ifdef SIM
    int32_t payload[1] = { mode};
    messagePort->SendMessage(MODE_SWITCH_CHANGED, payload, 1);
#else
    Serial.println(String("mode ") + mode);
#endif
  }

  // on-off button
  bool btn_on_off_updated = btn_on_off->update();
  if (btn_on_off_updated) {
    bool on = !btn_on_off->read();
#ifdef SIM
    int32_t payload[1] = { on};
    messagePort->SendMessage(ONOFF_BUTTON_PRESSED, payload, 1);
#else
    Serial.println(String("on/off ") + on);
#endif
  }

  // pot value
  //if (!btn_on_off->read()) {
  //  int pot_value = 1023 - analogRead(A3);
  //  Serial.println(String("pot ") + pot_value);
  //}

  // transfer button
  bool btn_transfer_updated = btn_transfer->update();
  if (btn_transfer_updated) {
#ifdef SIM
    int32_t payload[1] = { !btn_transfer->read()};
    messagePort->SendMessage(TRANSFER_BUTTON_PRESSED, payload, 1);
#else
    Serial.println(String("transfer ") + !btn_transfer->read());
#endif
  }

#ifdef SIM
  // Make sure this function is called regularly
  messagePort->Tick();
#endif

}
