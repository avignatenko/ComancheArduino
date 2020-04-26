
#include <Servo.h>
#include <Encoder.h>

#define SIM

#ifdef SIM
#include <si_message_port.hpp>

SiMessagePort* messagePort;
#endif


Servo myServo;  // create servo object to control a servo
Encoder myEnc(2, 3);

const int servoLeft = 4;
const int servoRight = 150;
const int servoCenter = (servoLeft + servoRight) / 2;


long targetServoAngle = servoCenter;
long oldEncPosition  = targetServoAngle * 2;

enum Messages  {
  SET_TRIM = 1, //-- payoad: int -100 .. 100
};

#ifdef SIM
static void new_message_callback(uint16_t message_id, struct SiMessagePortPayload* payload) {
}

#endif

unsigned long servo_start_time = -1;
const unsigned long servo_work_duration = 500;

void write_servo(int position) {

  if (position == myServo.read()) {
    if (servo_start_time < 0) return;
    if (millis() - servo_start_time > servo_work_duration) {
      myServo.detach();
      servo_start_time = -1;
    }
    return;  
  }

  if (!myServo.attached())
    myServo.attach(9);  // attaches the servo on pin 9 to the servo object
  
  myServo.write(position);
  
  servo_start_time = millis();
}

void setup() {
  myServo.attach(9);  // attaches the servo on pin 9 to the servo object
  myEnc.write(oldEncPosition);
#ifdef SIM
  // Init library on channel A
  messagePort = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_LEONARDO, SI_MESSAGE_PORT_CHANNEL_A, new_message_callback);
#else
  Serial.begin(9600);
#endif

}


void loop() {
  // deal with encoder
  long newPosition = myEnc.read();
  if (newPosition != oldEncPosition) {
    targetServoAngle = constrain(newPosition / 2, servoLeft, servoRight);
    int ratio = (targetServoAngle - servoCenter) * 100 * 2 / (servoRight - servoLeft);
#ifdef SIM
    int32_t payload[1] = { ratio };
    messagePort->SendMessage(SET_TRIM, payload, 1);
#else
    Serial.println((String)targetServoAngle + " " + ratio);
#endif
    oldEncPosition = newPosition;

  }

  write_servo(targetServoAngle);

#ifdef SIM
  // Make sure this function is called regularly
  messagePort->Tick();
#endif

}

