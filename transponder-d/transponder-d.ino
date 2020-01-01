#include <Encoder.h>
#include <Bounce2.h>

#define SIM

#ifdef SIM
#include <si_message_port.hpp>

SiMessagePort* messagePort;
#endif

// socket 1 pins
const int socket1_1 = -1;
const int socket1_2 = -1;
const int socket1_3 = 14;
const int socket1_4 = 23;
const int socket1_5 = 15;
const int socket1_6 = 24;
const int socket1_7 = 16;
const int socket1_8 = 25;
const int socket1_9 = 17;
const int socket1_10 = 26;
const int socket1_11 = 18;
const int socket1_12 = 27;
const int socket1_13 = 19;
const int socket1_14 = 28;
const int socket1_15 = 20;
const int socket1_16 = 29;
const int socket1_17 = 21;
const int socket1_18 = 30;
const int socket1_19 = 22;
const int socket1_20 = 31;



// socket 4 pins
const int socket4_1 = -1;
const int socket4_2 = -1;
const int socket4_3 = 10;
const int socket4_4 = 50;
const int socket4_5 = 11;
const int socket4_6 = 51;
const int socket4_7 = 12;
const int socket4_8 = 52;
const int socket4_9 = 13;
const int socket4_10 = 53;

// transponder panel 2 pins

const int panel2_5v = socket4_1; // +5v
const int panel2_gnd = socket4_2; // gnd
const int panel2_btn1 = socket4_3; // btn 1
const int panel2_rot_pos_1 = socket4_4; // rot pos 1
const int panel2_led = socket4_5; // led
const int panel2_rot_pos_2 = socket4_6;  // rot pos 2
const int panel2_rot_pos_8 = socket4_7; // rot pos 8
const int panel2_rot_pos_4 = socket4_8; // rot pos 4
const int panel2_nop1 = socket4_9; // nop
const int panel2_nop2 = socket4_10; // nop


struct Pin
{
  int clockPin;
  int resetPin;
};

Bounce* make_button(int pin) {
  pinMode(pin, INPUT_PULLUP);
  Bounce* btn = new Bounce();
  btn->attach(pin);
  btn->interval(30); // interval in ms
  return btn;
};


const int kNumPins = 4;
Pin pins[kNumPins] = {{41, 42}, {45, 46}, {36, 37}, {32, 33}};
Encoder encoders[kNumPins] = {Encoder(43, 44), Encoder(47, 48), Encoder(38, 39), Encoder(34, 35)};
const int enableDisplayPin = 40;

Bounce* rotator_buttons[4] = {
  make_button(panel2_rot_pos_1),
  make_button(panel2_rot_pos_2),
  make_button(panel2_rot_pos_4),
  make_button(panel2_rot_pos_8),
};

Bounce* ident_button = make_button(panel2_btn1);

const int kGpsButtonsNum = 15;
Bounce* gps_buttons[kGpsButtonsNum] = {
  make_button(socket1_7),
  make_button(socket1_8),
  make_button(socket1_9),
  make_button(socket1_10),
  make_button(socket1_11),
  make_button(socket1_12),
  make_button(socket1_13),
  make_button(socket1_14),
  make_button(socket1_15),
  make_button(socket1_16),
  make_button(socket1_17),
  make_button(socket1_18),
  make_button(socket1_19),
  make_button(socket1_20),
};

int gps_button_values[kGpsButtonsNum] = { -1};

// setup gps encoders

Encoder* gps_knobs[2] = {new Encoder(socket1_3, socket1_5), new Encoder(socket1_4, socket1_6)};

int brightness = 0;
int values[kNumPins] = {0, 0, 0, 0};
int rotatorPos = -1;
int identButtonPressed = -1;
/*
   Функция resetNumber обнуляет текущее значение
   на счётчике
*/
void resetNumber(int pin)
{
  // Для сброса на мгновение ставим контакт
  // reset в HIGH и возвращаем обратно в LOW
  digitalWrite(pins[pin].resetPin, HIGH);
  digitalWrite(pins[pin].resetPin, LOW);
}

/*
   Функция showNumber устанавливает показания индикаторов
   в заданное неотрицательное число `n` вне зависимости
   от предыдущего значения
*/
void showNumber(int pin, int n)
{
  // Первым делом обнуляем текущее значение
  resetNumber(pin);

  // Далее быстро «прокликиваем» счётчик до нужного
  // значения
  while (n--) {
    digitalWrite(pins[pin].clockPin, HIGH);
    digitalWrite(pins[pin].clockPin, LOW);
  }
}

void setup()
{
#ifdef SIM
  // Init library on channel A and Arduino type MEGA 2560
  messagePort = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_MEGA_2560, SI_MESSAGE_PORT_CHANNEL_D, new_message_callback);
#else
  Serial.begin(9600);
#endif

  // panel 1
  pinMode(enableDisplayPin, OUTPUT);
  digitalWrite(enableDisplayPin, LOW);

  for (int i = 0; i < kNumPins; ++i)
  {
    pinMode(pins[i].resetPin, OUTPUT);
    pinMode(pins[i].clockPin, OUTPUT);

    // Обнуляем счётчик при старте, чтобы он не оказался
    // в случайном состоянии
    resetNumber(i);
  }

  // panel 2

  // set the digital pin as output:
  pinMode(panel2_led, OUTPUT);


}


enum Messages  {
  REFRESH_STATE = 0, // payload: none
  SET_DIGIT = 1, //-- payoad: int selector (0 .. 3), int digit 0 .. 7
  ENCODER_DIGIT_ROTATED = 2, //, -- payoad: int selector (0 .. 3), int +1 , -1
  ENABLE_DISPLAY = 3, // payload: int 0 (disable), 1 (enable)
  ROTATOR_SWITCHED = 4, // payload: int position (0 .. 9)
  IDENT_PRESSED = 5, // payload: int pressed (0, 1)
  SET_IDENT_LIGHT = 6, // payload: int on/off: 1, 0
  GPS_BUTTON = 7, // payload: int button, int on/off
  GPS_KNOB_ROTATED = 8, //, -- payoad: int selector (0 .. 3), int +1 , -1
};


#ifdef SIM
static void new_message_callback(uint16_t message_id, struct SiMessagePortPayload* payload) {
  // Do something with a message from Air Manager or Air Player

  // The arguments are only valid within this function!
  // Make a clone if you want to store it
  //messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Received something with id " + message_id);

  if (message_id == REFRESH_STATE && payload == NULL) {
    rotatorPos = -1;
    identButtonPressed = -1;
  }
  else {
    if (message_id == SET_DIGIT && payload->type == SI_MESSAGE_PORT_DATA_TYPE_INTEGER && payload->len == 2) {
      int selector = payload->data_int[0];
      int number = payload->data_int[1];

      //messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Received SET_DIGIT: " + selector + " " + number);

      values[selector] = number;
    }

    if (message_id == ENABLE_DISPLAY && payload->type == SI_MESSAGE_PORT_DATA_TYPE_INTEGER && payload->len == 1) {
      int enable = payload->data_byte[0];

      //messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Received ENABLE_DISPLAY with " + enable);
      brightness = enable;
    }

    if (message_id == SET_IDENT_LIGHT && payload->type == SI_MESSAGE_PORT_DATA_TYPE_INTEGER && payload->len == 1) {
      int enable = payload->data_byte[0];

      //messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Received ENABLE_DISPLAY with " + enable);
      digitalWrite(panel2_led, enable > 0 ? HIGH : LOW);
    }

  }
}
#endif

long lastUpdate = 0;

void loop()
{
  // transponder rotators
  for (int i = 0 ; i < kNumPins; ++i)
  {
    long newPosition = encoders[i].read();

    int direction = 0;
    if (newPosition >= 4) direction = -1;
    if (newPosition <= -4) direction = 1;

    if (direction != 0) {
      encoders[i].write(0);

#ifdef SIM
      //messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Updated " + i + " to: " + newPosition);
      int32_t payload[2] = { i, direction };
      messagePort->SendMessage(ENCODER_DIGIT_ROTATED, payload, 2);
#else
        Serial.print(String("transponder: ") + i + " " + direction);
#endif
    }
  }

  long current = millis();
  if (lastUpdate < 0 || (current - lastUpdate) > 20)
  {
    for (int i = 0; i < kNumPins; ++i)
      showNumber(i, values[i]);

    lastUpdate = current;
  }

  // panel 2

  // update buttons before read
  for (int i = 0; i < 4; ++i) rotator_buttons[i]->update();
  ident_button->update();

  int btn = !ident_button->read();
  if (identButtonPressed < 0 || identButtonPressed != btn) {
    identButtonPressed = btn;
#ifdef SIM
    messagePort->SendMessage(IDENT_PRESSED, (int32_t)btn);
#endif
  }

  // read rotator
  int btn_rot_1 = !rotator_buttons[0]->read();
  int btn_rot_2 = !rotator_buttons[1]->read();
  int btn_rot_4 = !rotator_buttons[2]->read();
  int btn_rot_8 = !rotator_buttons[3]->read();

  int newRotatorPos = btn_rot_1 + (btn_rot_2 << 1) + (btn_rot_4 << 2) + (btn_rot_8 << 3);
  if (rotatorPos < 0 || rotatorPos != newRotatorPos) {
    rotatorPos = newRotatorPos;
#ifdef SIM
    messagePort->SendMessage(ROTATOR_SWITCHED, (int32_t)rotatorPos);
#endif
  }

  if (brightness > 0) {
    static int cur_led_brightness_step = 0;
    digitalWrite(enableDisplayPin, cur_led_brightness_step == 0 ? HIGH : LOW);
    ++cur_led_brightness_step;
    if (cur_led_brightness_step > 10 - brightness) cur_led_brightness_step = 0;
  } else {
    digitalWrite(enableDisplayPin, LOW);

  }
#ifndef SIM
  //Serial.println((String)btn_rot_1 + (String)btn_rot_2 + (String)btn_rot_4 + (String)btn_rot_8 + " " + (String)newRotatorPos);
  digitalWrite(panel2_led, newRotatorPos == 3);
#endif

  // gps
  for (int i = 0; i < 14; ++i) {
    gps_buttons[i]->update();
    int btn = !gps_buttons[i]->read();
#ifndef SIM
    if (btn) Serial.println((String)"btn pressed " + (String)i);
#else
    int& cur_value = gps_button_values[i];
    if (cur_value < 0 || cur_value != btn) {
      int32_t payload[2] = { i, btn};
      messagePort->SendMessage(GPS_BUTTON, payload, 2);
      cur_value = btn;
    }
#endif

    for (int i = 0 ; i < 2; ++i)
    {
      long newPosition = gps_knobs[i]->read();

      int direction = 0;
      if (newPosition >= 2) direction = -1;
      if (newPosition <= -2) direction = 1;

      if (direction != 0) {
        gps_knobs[i]->write(0);

#ifdef SIM
        //messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Updated " + i + " to: " + newPosition);
        int32_t payload[2] = { i, direction };
        messagePort->SendMessage(GPS_KNOB_ROTATED, payload, 2);
#else
        Serial.print(String("GPS: ") + i + " " + direction);
#endif
      }
    }



  }

#ifdef SIM
  // Make sure this function is called regularly
  messagePort->Tick();
#endif
}
