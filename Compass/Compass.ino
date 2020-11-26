#include <AccelStepper.h>
#include <si_message_port.hpp>

#define SIM

#ifdef SIM
SiMessagePort* messagePort;
SiMessagePortChannel kChannel = SI_MESSAGE_PORT_CHANNEL_K;
#endif

AccelStepper stepper(AccelStepper::HALF4WIRE, 5, 3, 4, 2);
const long kTotalSteps = 2038 * 2;
const long kTotalCalibrationSteps = kTotalSteps * 1.05;

const int kQRD1114Pin = A0; // Sensor output voltage

int g_calibrationStatus = 0;
int g_minValue[2] = {1024, 1024};
int g_minPos[2] = { -1, -1};

int s_markDegrees = 310;

enum Mode
{
  POSITION,
  SPEED1,
  CALIBRATION
};

Mode s_mode = POSITION;

enum Messages
{
  STEPPER_PARAMETERS = 0, // <int - max speed> <int - acceleration>
  STEPPER_MOVE = 1, // <float> - desired position (angle degrees / speed)
  STEPPER_CALIBRATE = 2, // <float> - current position (angle degrees)
  STEPPER_AUTO_CALIBRATE = 3, // <float> - mark position (angle degrees)
  STEPPER_MODE = 4, // <int mode> 0 - position, 1 - speed
};

//Normalizes any number to an arbitrary range
//by assuming the range wraps around when going below min or above max
float normalize( const float value, const float start, const float end )
{
  const float width       = end - start   ;   //
  const float offsetValue = value - start ;   // value relative to 0

  return ( offsetValue - ( floor( offsetValue / width ) * width ) ) + start ;
  // + start to reset back to start of original range
}

float positionToAngle(long pos) {
  return normalize(pos * 360.0 / kTotalSteps, 0.0, 360.0);
}

long angleToPosition(float angle) {
  return kTotalSteps * angle / 360.0;
};


void startCalibration(int markDegrees) {
  s_markDegrees = markDegrees;
  s_mode = CALIBRATION;

}

void runToAngle(float angle) {
  float curAngle = positionToAngle(stepper.currentPosition()); 
  float newAngle = angle;
  float d1 = newAngle - curAngle;
  float d2 = copysign(360 - abs(d1), -d1);

  long delta = angleToPosition(abs(d1) > abs(d2) ? d2 : d1);

#ifndef SIM
  Serial.print(String("cur: ") + curAngle + " ");
  Serial.print(String("newAngle: ") + newAngle + " ");
  Serial.print(String("d1: ") + d1 + " ");
  Serial.print(String("d2: ") + d2 + " ");
  Serial.print(String("delta: ") + delta + " ");
  Serial.println();
#endif

  stepper.move(delta);
}

#ifdef SIM

static void new_message_callback(uint16_t message_id, struct SiMessagePortPayload* payload) {

  switch (message_id)
  {
    case STEPPER_PARAMETERS: {
        messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"Received speeds: " + payload->data_int[0] + " " + payload->data_int[1]);
        stepper.setMaxSpeed(payload->data_int[0]);
        stepper.setAcceleration(payload->data_int[1]);
        break;
      }
    case STEPPER_MOVE: {
        if (s_mode == SPEED1) {
          stepper.setSpeed(payload->data_float[0]);
        } else if (s_mode == POSITION) {
          runToAngle(payload->data_float[0]);
        }
        break;
      }
    case STEPPER_CALIBRATE: {
        stepper.setCurrentPosition(angleToPosition(payload->data_float[0]));
        break;
      }
    case STEPPER_AUTO_CALIBRATE: {
        startCalibration(payload->data_float[0]);
        break;
      }
    case STEPPER_MODE: {
        if (s_mode != CALIBRATION)
          s_mode = (Mode)payload->data_int[0];
        break;
      }

  }
}
#endif

void calibrate() {

  if (g_calibrationStatus == 0) {
    g_calibrationStatus = 1;
    g_minValue[0] = g_minValue[1] = 1024;
    g_minPos[0] = g_minPos[1] = -1;

    stepper.setCurrentPosition(0);
    stepper.moveTo(kTotalCalibrationSteps);
  }

  int proximityADC = analogRead(kQRD1114Pin);

  if (proximityADC < g_minValue[g_calibrationStatus - 1]) {
    g_minValue[g_calibrationStatus - 1] = proximityADC;
    g_minPos[g_calibrationStatus - 1] = stepper.currentPosition();
  }

  if (stepper.distanceToGo() == 0) {
    if (g_calibrationStatus < 3) { // first or second pass?

#ifndef SIM
      Serial.println(
        String("Found min value: ") + g_minValue[g_calibrationStatus - 1] +
        String(" at ") + g_minPos[g_calibrationStatus - 1]);
#endif
      stepper.moveTo(kTotalCalibrationSteps - stepper.currentPosition());
      ++g_calibrationStatus;
    }

    if (g_calibrationStatus == 3) {
      int average = (g_minPos[0] + g_minPos[1]) / 2;
#ifndef SIM
      Serial.println(String("Average pos: ") + average);
#endif
      stepper.runToNewPosition(average);
      stepper.setCurrentPosition(angleToPosition(s_markDegrees));
      //stepper.setCurrentPosition(angleToPosition(positionToAngle(-average + stepper.currentPosition()) - s_markDegrees));
      //runToAngle(0);

      s_mode = POSITION;
      g_calibrationStatus = 0;

    }

    return;
  }

  stepper.run();

}

void setup()
{
#ifdef SIM
  messagePort = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_MEGA_2560, kChannel, new_message_callback);
#endif

  stepper.setMaxSpeed(500);
  stepper.setAcceleration(1000);
  pinMode(kQRD1114Pin, INPUT);

#ifndef SIM
  Serial.begin(115200);
  startCalibration(s_markDegrees);
#endif

}

void loop()
{
  if (s_mode == POSITION)
    stepper.run();
  else if (s_mode == SPEED1)
    stepper.runSpeed();
  else if (s_mode == CALIBRATION)
    calibrate();

#ifdef SIM
  messagePort->Tick();
#endif
}
