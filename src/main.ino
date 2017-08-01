
#include "pinout.h"
#include "encoders.h"
#include "motors.h"
#include "TB6612.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

IntervalTimer encoderRefresh;


void setup() {
  // initialize digital pin LED_BUILTIN as an output.

  Serial.begin(9600); // Configuración del puerto serie
  analogWriteResolution(16);         // analogWrite value 0 to 65535
  analogWriteFrequency(driver_PWMA, 732.4218); // Teensy 3.0 pin 4 also changes to 732.4218 kHz
  analogWriteFrequency(driver_PWMB, 732.4218);
  setupInterrupts();
  encoderRefresh.begin(encoderFun, 10000);
  motorBrake(2);

  display.begin(SSD1306_SWITCHCAPVCC, 0x7A);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.drawPixel(10, 10, WHITE);
  display.display();

}


// the loop function runs over and over again forever
void loop() {
  digitalWrite(13, HIGH);


  /*motorSpeed(0, 0, 10000);
  delay(500);
  motorSpeed(1, 0, 10000);
  delay(500);
  motorSpeed(2, 0, 0);
  delay(500);
  motorSpeed(2, 0, 10000);
  delay(500);
  motorSpeed(0, 1, 10000);
  delay(500);
  motorSpeed(1, 1, 10000);
  delay(500);
  motorSpeed(2, 1, 0);
  delay(500);
  motorSpeed(2, 1, 10000);
  delay(500);*/
  //motorSpeed(2, 0, 20000);
  delay(10000);

}

void encoderFun(){
  updateEncoderData();
  Serial.printf("Enc L: %i, Enc R: %i, Distance %i, Angle %i \n",encoderL,encoderR,readDistance(),readAngle());

}
