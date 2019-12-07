#include <Encoder.h>
#include <Bounce2.h>

#define SIM

#ifdef SIM
#include <si_message_port.hpp>

SiMessagePort* messagePort;
#endif



// socker 4 pins
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

const int kNumPins = 4;
Pin pins[kNumPins] = {{41, 42}, {45, 46}, {36, 37}, {32, 33}};
Encoder encoders[kNumPins] = {Encoder(43, 44), Encoder(47, 48), Encoder(38, 39), Encoder(34, 35)};
const int enableDisplayPin = 40;

Bounce* rotator_buttons = new Bounce[10];
Bounce* ident_button = new Bounce;

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
  ident_button->attach(panel2_btn1, INPUT_PULLUP);
  ident_button->interval(10);

  rotator_buttons[0].attach(panel2_rot_pos_1, INPUT_PULLUP);
  rotator_buttons[0].interval(30);              // interval in ms
  rotator_buttons[1].attach(panel2_rot_pos_2, INPUT_PULLUP);
  rotator_buttons[1].interval(30);              // interval in ms
  rotator_buttons[2].attach(panel2_rot_pos_4, INPUT_PULLUP);
  rotator_buttons[2].interval(30);              // interval in ms
  rotator_buttons[3].attach(panel2_rot_pos_8, INPUT_PULLUP);
  rotator_buttons[3].interval(30);              // interval in ms

}


enum Messages  {
  REFRESH_STATE = 0, // payload: none
  SET_DIGIT = 1, //-- payoad: int selector (0 .. 3), int digit 0 .. 7
  ENCODER_DIGIT_ROTATED = 2, //, -- payoad: int selector (0 .. 3), int +1 , -1
  ENABLE_DISPLAY = 3, // payload: int 0 (disable), 1 (enable)
  ROTATOR_SWITCHED = 4, // payload: int position (0 .. 9)
  IDENT_PRESSED = 5, // payload: int pressed (0, 1)
  SET_IDENT_LIGHT = 6, // payload: int on/off: 1, 0
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
  for (int i = 0; i < 4; ++i) rotator_buttons[i].update();
  ident_button->update();

  int btn = !ident_button->read();
  if (identButtonPressed < 0 || identButtonPressed != btn) {
    identButtonPressed = btn;
#ifdef SIM
    messagePort->SendMessage(IDENT_PRESSED, (int32_t)btn);
#endif
  }

  // read rotator
  int btn_rot_1 = !rotator_buttons[0].read();
  int btn_rot_2 = !rotator_buttons[1].read();
  int btn_rot_4 = !rotator_buttons[2].read();
  int btn_rot_8 = !rotator_buttons[3].read();

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
  Serial.println((String)btn_rot_1 + (String)btn_rot_2 + (String)btn_rot_4 + (String)btn_rot_8 + " " + (String)newRotatorPos);
  digitalWrite(panel2_led, newRotatorPos == 3);
#endif

#ifdef SIM
  // Make sure this function is called regularly
  messagePort->Tick();
#endif
}
