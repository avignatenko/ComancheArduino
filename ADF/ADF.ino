
#include <Encoder.h>
#include <Bounce2.h>
#include <Adafruit_CharacterOLED.h>

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

void setup()
{
  // Print a message to the LCD.
  lcd.begin(16, 2);
  lcd.print("hello OLED World");


  Serial.begin(9600);
}

void printSecondLine(String s)
{
  lcd.setCursor(0, 1);
  lcd.print(s);
}

void loop()
{


  // encoders
  for (int j = 0; j < 2; ++j) {
    long new_pos = knobs[j]->read() / 2;
    if (knobs_cur_value[j] != new_pos) {
      Serial.println(String("knob ") + j + " new pos: " + new_pos);
      knobs_cur_value[j] = new_pos;
    }
  }

  // button enc
  bool btn_enc_updated = btn_enc->update();
  if (btn_enc_updated) {
    printSecondLine(String("Button: ") + (!btn_enc->read() ? "pressed" : "none"));

  }

  // mode button
  bool btn_mode_updated[2] = {btn_mode[0]->update(), btn_mode[1]->update()};
  if (btn_mode_updated[0] || btn_mode_updated[1]) {
      int mode = (!btn_mode[0]->read() ? 1 : (!btn_mode[1]->read() ? 2 : 0)); 
      Serial.println(String("mode ") + mode);
  }

  // on-off button
  bool btn_on_off_updated = btn_on_off->update();
  if (btn_on_off_updated) {
    Serial.println(String("on/off ") + !btn_on_off->read());
  }

  // pot value
  if (!btn_on_off->read()) {
    int pot_value = 1023 - analogRead(A3);
     Serial.println(String("pot ") + pot_value);
  }

}
