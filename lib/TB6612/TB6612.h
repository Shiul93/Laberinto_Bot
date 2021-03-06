#ifndef TB6612_h
#define TB6612_h
#include <Arduino.h>

#define ANALOG_W_RES 65535

class TB6612 {
  public:
    TB6612 (int AIN1, int AIN2, int BIN1, int BIN2, int PWMA, int PWMB, int STBY);

    void forward (int velocidad_L, int velocidad_R, int tiempo);
    void reverse (int velocidad_L, int velocidad_R, int tiempo);
    //void drive (int velocidad_L, int velocidad_R, int tiempo);
    void rotate (int velocidad, int tiempo);
    void setSpeed(int speed, int motor);
    void brake (int tiempo);
    void coast (int tiempo);


    int _AIN1;
    int _AIN2;
    int _BIN1;
    int _BIN2;
    int _PWMA;
    int _PWMB;
    int _STBY;

};
#endif
