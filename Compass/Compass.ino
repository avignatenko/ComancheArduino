#include <AccelStepper.h>
#include <si_message_port.hpp>

SiMessagePort* messagePort;
SiMessagePortChannel kChannel = SI_MESSAGE_PORT_CHANNEL_J;

AccelStepper stepper(AccelStepper::HALF4WIRE, 12, 10, 11, 9); 
const int kTotalSteps = 4096;

enum Mode
{
  POSITION,
  SPEED
};

Mode s_mode = POSITION;

enum Messages
{
  STEPPER_PARAMETERS = 0, // <int - max speed> <int - acceleration>
  STEPPER_MOVE = 1, // <float> - desired position (angle degrees / speed)
  STEPPER_CALIBRATE = 2, // <float> - current position (angle degrees)
  STEPPER_MODE = 3, // <int mode> 0 - position, 1 - speed
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
  return normalize((pos % kTotalSteps) * 360.0 / kTotalSteps, 0.0, 360.0);
}

long angleToPosition(float angle) {
  return kTotalSteps * angle / 360.0;
};

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
        if (s_mode == SPEED) {
          stepper.setSpeed(payload->data_float[0]);
        } else {
          float curAngle = positionToAngle(stepper.currentPosition());
          float newAngle = payload->data_float[0];

          float d1 = newAngle - curAngle;  
          float d2 = copysign(360 - abs(d1), -d1);   

          long delta = angleToPosition(abs(d1) > abs(d2) ? d2 : d1);

          stepper.move(delta);

         //messagePort->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, (String)"calculated: " + stepper.currentPosition() + " " + curAngle + " " + newAngle + " " + delta);
        }
        break;
      }
    case STEPPER_CALIBRATE: {
        stepper.setCurrentPosition(angleToPosition(payload->data_float[0]));
        break;
      }

    case STEPPER_MODE: {
        s_mode = (Mode)payload->data_int[0];
        break;
      }

  }
}

void setup()
{
  messagePort = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_MEGA_2560, kChannel, new_message_callback);

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(200);
}

void loop()
{
  if (s_mode == POSITION)
    stepper.run();
  else
    stepper.runSpeed();
    
  messagePort->Tick();
}
