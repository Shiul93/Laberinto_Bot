#include "arduino.h"
#include "main.h"
#include <SPI.h>
#include <Wire.h>
#include "pinout.h"
#include "TB6612.h"
#include "motors.h"
#include "encoders.h"
#include "elapsedMillis.h"
#include "IntervalTimer.h"
#include <Adafruit_NeoPixel.h>
#include "screen.h"
#include <VL53L0X.h>
#include "sensors.h"




int distFL, distCL, distCR, distFR;
double  errP, errI, errD, lastErr, lastErrL, lastErrR = 0;

bool completed = true;

double pid_err_print;
double g_kp, g_ki, g_kd = 0; //Gyro pid
double w_kp, w_ki, w_kd = 0; //Wall follow pid
double d_kp, d_ki, d_kd = 0; //Distance pid
double s_kp, s_ki, s_kd = 0; //Speed pid
double r_kp, r_ki, r_kd = 0; //Speed pid



int almost_checks = 20;
int almost_count = 0;
bool almost = false;

long lastTickL = 0;
long lastTickR = 0;
double speedL = 0;
double speedR = 0;
long lastSpeedCheck = 0;
int lastSpeedSetL = 0;
int lastSpeedSetR = 0;



int meanIndex = 0;
int distFLarray[] = {0,0,0};
int distCLarray[] = {0,0,0};
int distCRarray[] = {0,0,0};
int distFRarray[] = {0,0,0};
float accelArray[] = {0,0,0};
float gyroArray[] = {0,0,0};


VL53L0X laserFL,laserCL, laserCR, laserFR;


IntervalTimer sysTimer;
unsigned long sysTickCounts = 0;
elapsedMicros systickMicros;
unsigned int sysTickMilisPeriod =  100;
unsigned int sysTickSecond = 1000/sysTickMilisPeriod;

unsigned int longCount = 0;
unsigned int mediumCount = 0;
char screenShow = 'e';
char activeBehavior = 'o';

int hb = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, 8);
bool activateController = false;


int sign(int x) {
  return (x > 0) - (x < 0);
}
void updateDistances(){//1ms


  distFL = laserFL.readRangeContinuousMillimeters();
  distCL = laserCL.readRangeContinuousMillimeters();
  distCR = laserCR.readRangeContinuousMillimeters();
  distFR = laserFR.readRangeContinuousMillimeters();
  distFLarray[meanIndex] = (distFL<1200)? distFL : 0;
  distCLarray[meanIndex] = (distCL<1200)? distCL : 0;
  distCRarray[meanIndex] = (distCR<1200)? distCR : 0;
  distFRarray[meanIndex] = (distFR<1200)? distFR : 0;
  distFL = (distFLarray[0]+distFLarray[1]+distFLarray[2])/3;
  distCL = (distCLarray[0]+distCLarray[1]+distCLarray[2])/3;
  distCR = (distCRarray[0]+distCRarray[1]+distCRarray[2])/3;
  distFR = (distFRarray[0]+distFRarray[1]+distFRarray[2])/3;
  meanIndex++;
  meanIndex = meanIndex%3;



}

void sysTick() {
  systickMicros =0;


  sysTickCounts++;
  mediumCount++;
  longCount++;


}

void setup(){

  w_kp = 0.27;
  w_ki = 0.01;
  w_kd = 0.02;

  g_kp = 0.50 ;
  g_ki = 0.05;
  g_kd = 0.07;

  d_kp = 0.3 ;
  d_ki = 0.05;
  d_kd = 0.15;

  s_kp = 50 ;
  s_ki = 0;
  s_kd = 10;

  r_kp = 0.40 ;
  r_ki = 0.15;
  r_kd = 0.07;


  Wire.begin();
  setupScreen();
  //setupPins();


  delay(1000);
  displayString("Starting Serial");
  Serial.begin(9600);
  delay(1000);

  Serial.println("----- BEGIN SERIAL -----");
  Serial1.begin(115200);
  Serial1.println("----------BEGIN BT---------");
  displayString("Starting interrupts");

  setupInterrupts();
  displayString("Starting Leds");

  strip.begin();
  strip.setPixelColor(0, 10, 0, 0);
  strip.setPixelColor(1, 10, 0, 0);
  strip.show();
  delay(500);
  strip.setPixelColor(0, 0, 10, 0);
  strip.setPixelColor(1, 0, 10, 0);
  strip.show();
  delay(500);
  strip.setPixelColor(0, 0, 0, 10);
  strip.setPixelColor(1, 0, 0, 10);
  strip.show();
  delay(500);
  strip.setPixelColor(0, 0, 0, 0);
  strip.setPixelColor(1, 0, 0, 0);
  strip.show();

  displayString("Starting Distance Sensors");

  setupDistanceSensors();
  delay(100);
  laserFL.startContinuous();
  laserCL.startContinuous();
  laserCR.startContinuous();
  laserFR.startContinuous();

  sysTimer.begin(sysTick, sysTickMilisPeriod);
  displayString("Starting IMU");
  startIMU();


  digitalWrite(buzzer_pin, HIGH);
  delay(100);
  digitalWrite(buzzer_pin, LOW);
  delay(100);
  digitalWrite(buzzer_pin, HIGH);
  delay(100);
  digitalWrite(buzzer_pin, LOW);
  delay(100);
  displayString("Startup OK");



  updateDistances();







}
void encoderFun(){
  updateEncoderData();
  //Serial.printf("Enc L: %i, Enc R: %i, Distance %i, Angle %i \n",encoderL,encoderR,readDistance(),readAngle());
  //Serial1.printf("Enc L: %i, Enc R: %i, Distance %i, Angle %i \n",encoderL,encoderR,readDistance(),readAngle());

}
void loop(){


  if(sysTickCounts>= sysTickMilisPeriod)//32 ms
  {
    encoderFun();
    updateDistances();
    checkSpeed();
    readAccelValues(accelArray);
    readGyroValues(gyroArray);

    sysTickCounts = 0;
    //followBehavior(kp,ki,kd);
    if (activeBehavior == 'g'){
      gyroBehavior(g_kp,g_ki,g_kd);
    }else if (activeBehavior == 'w'){
      followBehavior(w_kp,w_ki,w_kd);
    }else if (activeBehavior == 'd'){
      if (!completed){
        distanceBehavior(300,readDistance(),d_kp,d_ki,d_kd);
      }
    }else if (activeBehavior == 'r'){
      if (!completed){
        rotateBehavior(360,readAngle(),r_kp,r_ki,r_kd);
      }
    }else if (activeBehavior == 's'){
      speedBehavior(0.2,s_kp,s_ki,s_kd);
    }else {
      motorCoast(RLMOTOR);
    }

    //encoderFun();



  }

  if(mediumCount>= 10*sysTickMilisPeriod)//320 ms
  {
    mediumCount = 0;
    if (screenShow == 'p'){
      if (activeBehavior == 'g'){
        displayPID(g_kp*errP,g_ki*errI, g_kd*errD, pid_err_print);
      }else if (activeBehavior == 'w'){
        displayPID(w_kp*errP,w_ki*errI, w_kd*errD, pid_err_print);
      }else if (activeBehavior == 'd'){
        displayPID(d_kp*errP,d_ki*errI, d_kd*errD, pid_err_print);
      }else if (activeBehavior == 's'){
        displayPID(s_kp*errP,s_ki*errI, s_kd*errD, pid_err_print);
      }else if (activeBehavior == 'r'){
        displayPID(r_kp*errP,r_ki*errI, r_kd*errD, pid_err_print);
      }
    }else  if (screenShow =='e'){
      displayENC(encoderR,encoderL,readDistance(), readAngle(), speedR, speedL);
    } else {
      displayGyro(gyroArray);
    }
  }

  if(longCount>= (1000/3)*sysTickMilisPeriod)//3200ms
  {
  longCount = 0;
  if (Serial1.available()){
    strip.setPixelColor(0, 0, 0, 10);
    strip.setPixelColor(1, 0, 0, 10);
    strip.show();
    delay(100);
    strip.setPixelColor(0, 0, 0, 0);
    strip.setPixelColor(1, 0, 0, 0);
    strip.show();
    String read = Serial1.readString(1);
    if (read =='p'){
      screenShow = 'p';
    }else if (read =='e'){
      screenShow = 'e';
    }else if (read =='a'){
      screenShow = 'a';
    }else if (read =='g'){
      screenShow = 'g';
    }else if (read =='r'){
      encoderReset();
    }
    else if (read =='b'){
      read = Serial1.readString(1);
      if (read =='g'){
        activeBehavior = 'g';
      }else if (read =='s'){
        activeBehavior = 's';
      }else if (read =='w'){
        activeBehavior = 'w';

      }else if (read =='o'){
        activeBehavior = 'o';

      }else if (read == 'd'){
        encoderReset();
        activeBehavior = 'd';
        completed = false;
      }else if (read == 'r'){
        encoderReset();
        activeBehavior = 'r';
        completed = false;
      }
    }
    Serial1.readString(2);
    Serial1.println("ACK");
    Serial1.println(read);

  }
  //printDistances();
  }



}

void followBehavior(double kp,double ki,double kd){
  int error = distCL-distCR;
  errP = error;
  errI = abs(errI+error)>1200 ? sign(errI+error)*1200 : (errI+error);
  errI = ((error*errI)<0) ? 0 : errI;

  errD = error-lastErr;
  errD = abs(error-lastErr)>1200 ? sign(error-lastErr)*1200 : (error-lastErr);

  lastErr = error;
  double pidErr = kp*errP + ki*errI + kd*errD;
  pid_err_print = pidErr;
  int newerror = pidErr;
  //Serial.printf("eP: %i eI: %i eD: %i PID_Err: %i \n",errP,errI,errD,newerror);

  if (abs(newerror) > 5){

  motorSpeed(RMOTOR, (error>0), abs(newerror));
  motorSpeed(LMOTOR, (error<=0), abs(newerror));
  //motorSpeed(RMOTOR, true, newerror);
  //motorSpeed(LMOTOR, true, newerror);
}else{
  motorBrake(RLMOTOR);
}
}

void gyroBehavior(double kp,double ki,double kd){
  int error = -gyroArray[2];
  errP = error;
  errI = errI+error;
  errI = ((error*errI)<0) ? 0 : errI;

  errD = error-lastErr;


  lastErr = error;
  double pidErr = kp*errP + ki*errI + kd*errD;
  pid_err_print = pidErr;
  int newerror = pidErr;

  if (abs(newerror) > 5){

    motorSpeed(RMOTOR, (error>0), abs(newerror));
    motorSpeed(LMOTOR, (error<=0), abs(newerror));
  }else{
    motorBrake(RLMOTOR);
  }

}


void speedBehavior(double speed,double kp,double ki,double kd){
  double errorL = speed-speedL;
  errP = errorL;
  errI = errI+errorL;
  errI = ((errorL*errI)<0) ? 0 : errI;
  errD = errorL-lastErrL;


  lastErrL = errorL;
  double pidErrL = kp*errP + ki*errI + kd*errD;
  pid_err_print = pidErrL;
  int newerrorL = lastSpeedSetL +  pidErrL;
  lastSpeedSetL = newerrorL;


  double errorR = speedL -speedR;
  errP = errorR;
  errI = errI+errorR;
  errI = ((errorR*errI)<0) ? 0 : errI;
  errD = errorR-lastErrR;


  lastErrR = errorR;
  double pidErrR = kp*errP + ki*errI + kd*errD;
  pid_err_print = pidErrR;
  int newerrorR = lastSpeedSetR +  pidErrR;
  lastSpeedSetR = newerrorR;




    motorSpeed(RMOTOR, (lastSpeedSetR>0), abs(newerrorR));
    motorSpeed(LMOTOR, (lastSpeedSetL>0), abs(newerrorL));

}

void distanceBehavior(int mm,int distance, double kp,double ki,double kd){
  completed = false;
  almost = false;

  int error = mm - distance;
  errP = error;
  errI = errI+error;
  errI = ((error*errI)<0) ? 0 : errI;

  errD = error-lastErr;


  lastErr = error;
  double pidErr = kp*errP + ki*errI + kd*errD;
  pid_err_print = (abs(pidErr)> 30)? sign(pidErr)*30 : pidErr;
  int newerror = (abs(pidErr)> 30)? sign(pidErr)*30 : pidErr;
  newerror = (abs(pidErr)< 13)? sign(pidErr)*13 : pidErr;
  newerror = (abs(pidErr)>50)? 50 : pidErr;


  if (abs(newerror) > 3){

  motorSpeed(RMOTOR, (error>0), abs(newerror));
  motorSpeed(LMOTOR, (error>0), abs(newerror));
  }else{
    if (almost_count > almost_checks){
      completed = true;
      almost_count = 0;
      motorBrake(RLMOTOR);

    }else{
      almost_count++;
      motorSpeed(RMOTOR, (error>0), abs(newerror));
      motorSpeed(LMOTOR, (error>0), abs(newerror));
    }
}

}

void rotateBehavior(int degrees,int angle, double kp,double ki,double kd){
  completed = false;
  almost = false;

  int error = degrees-readAngle();
  errP = error;
  errI = errI+error;
  errI = ((error*errI)<0) ? 0 : errI;

  errD = error-lastErr;


  lastErr = error;
  double pidErr = kp*errP + ki*errI + kd*errD;
  pid_err_print = (abs(pidErr)> 30)? sign(pidErr)*30 : pidErr;
  int newerror = (abs(pidErr)> 30)? sign(pidErr)*30 : pidErr;
  newerror = (abs(pidErr)< 17)? sign(pidErr)*17 : pidErr;
  newerror = (abs(pidErr)>50)? 50 : pidErr;


  if (abs(newerror) > 3){

  motorSpeed(RMOTOR, (error>0), abs(newerror));
  motorSpeed(LMOTOR, (error<0), abs(newerror));
  }else{
    if (almost_count > almost_checks){
      completed = true;
      almost_count = 0;
      motorBrake(RLMOTOR);

    }else{
      almost_count++;
      motorSpeed(RMOTOR, (error>0), abs(newerror));
      motorSpeed(LMOTOR, (error<0), abs(newerror));
    }
}
}


void printDistances(){
  Serial.print("FL: ");
  Serial.print(distFL);
  Serial.print(" CL: ");
  Serial.print(distCL);
  Serial.print(" CR: ");
  Serial.print(distCR);
  Serial.print(" FL: ");
  Serial.print(distFL);

  Serial.println();
}

void checkSpeed(){

  //TODO Comprobar direccion
  long check = millis();
  double time_elapsed = check - lastSpeedCheck;
  lastSpeedCheck = check;

  int encCheck = encoderL;
  int direction = (lastTickL < encCheck) ? 1 : -1;
  speedL = direction * abs(abs(lastTickL)-abs(encCheck))/(time_elapsed);
  lastTickL = encCheck;

  encCheck = encoderR;
  direction = (lastTickR < encCheck) ? 1 : -1;
  speedR = direction * abs(abs(lastTickR)-abs(encoderR))/(time_elapsed);
  lastTickR = encCheck;

}

void setupDistanceSensors() {
  pinMode(sensor_rst1, OUTPUT);
  pinMode(sensor_rst2, OUTPUT);
  pinMode(sensor_rst3, OUTPUT);
  pinMode(sensor_rst4, OUTPUT);

  digitalWrite(sensor_rst1, LOW);
  digitalWrite(sensor_rst2, LOW);
  digitalWrite(sensor_rst3, LOW);
  digitalWrite(sensor_rst4, LOW);


  delay(500);
  //Wire.begin();

  pinMode(sensor_rst1, INPUT);
  delay(150);
  laserFL.init(true);
  delay(100);
  laserFL.setAddress((uint8_t)22);

  pinMode(sensor_rst2, INPUT);
  delay(150);
  laserCL.init(true);
  delay(100);
  laserCL.setAddress((uint8_t)25);

  pinMode(sensor_rst3, INPUT);
  delay(150);
  laserCR.init(true);
  delay(100);
  laserCR.setAddress((uint8_t)28);

  pinMode(sensor_rst4, INPUT);
  delay(150);
  laserFR.init(true);
  delay(100);
  laserFR.setAddress((uint8_t)31);

  laserFL.setTimeout(500);
  laserCL.setTimeout(500);
  laserCR.setTimeout(500);
  laserFR.setTimeout(500); }
