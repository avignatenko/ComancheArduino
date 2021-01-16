#include <AmperkaKB.h>
#include <Keyboard.h>
#include <Joystick.h>

AmperkaKB KB(7, 6, 5, 4, 3, 2, 1, 0);
Joystick_ joy(0x03, JOYSTICK_TYPE_JOYSTICK, 6, 0, false, false, false, false, false, false, false, false, false, false, false);
 
void setup()
{
  KB.begin(KB4x4);
  Keyboard.begin();
  joy.begin();
}

void loop()
{
  KB.read();
  if (KB.justPressed()) {
    if (KB.getChar >= '0' && KB.getChar <= '9')
      Keyboard.press(KB.getChar);
    else // other buttons are 10-15
      joy.pressButton(KB.getNum - 10);
  }
 
  if (KB.justReleased()) {
    if (KB.getChar >= '0' && KB.getChar <= '9')
      Keyboard.release(KB.getChar);
    else // other buttons are 10-15
      joy.releaseButton(KB.getNum - 10);
  }
  
}
