


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


// A4 as SDA, A5 as SCL 
LiquidCrystal_PCF8574 lcd(0x26, 45, 37); // set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_PCF8574 lcd2(0x27, 27, 19); // set the LCD address to 0x27 for a 16 chars and 2 line display

int show = -1;

void setup()
{
  int error;

  Serial.begin(115200);
  Serial.println("LCD...");

  // wait on Serial to be available on Leonardo
  while (!Serial)
    ;

  Serial.println("Dose: check for LCD");
/*
  // See http://playground.arduino.cc/Main/I2cScanner how to test for a I2C device.
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();
  Serial.print("Error: ");
  Serial.print(error);

  if (error == 0) {
    Serial.println(": LCD found.");
    show = 0;
    lcd.begin(16, 2); // initialize the lcd

  } else {
    Serial.println(": LCD not found.");
  } // if
*/
    show = 0;
    lcd.begin(16, 2); // initialize the lcd
     lcd2.begin(16, 2); // initialize the lcd

} // setup()


void loop()
{
    lcd.setBacklight(255);
    lcd.setCursor(0, 0);
    lcd.print("188.00   188.88");
    lcd.setCursor(0, 1);
    lcd.print("nm kt min hold   loc to fr");

    lcd2.setBacklight(255);
    lcd2.setCursor(0, 0);
    lcd2.print("188.00   188.88");
    lcd2.setCursor(0, 1);
    lcd2.print("nm kt min hold   loc to fr");
} // loop()
