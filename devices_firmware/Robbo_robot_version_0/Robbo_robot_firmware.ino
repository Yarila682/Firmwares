#define FIRMWARE_VERSION "00009"
//toDO PIDpath обнулить при перполнении на обоих моторах

#include <EEPROM.h>
#include "Arduino.h"
#include "servotimer2.h"

#define SERIAL_SPEED 115200
#define SERIAL_ADDRESS 0
#define ADDRCALIB 1000
#define BLUE_CHECK 500


#define portOfPin(P)\
  (((P)>=0&&(P)<8)?&PORTD:(((P)>7&&(P)<14)?&PORTB:&PORTC))
#define ddrOfPin(P)\
  (((P)>=0&&(P)<8)?&DDRD:(((P)>7&&(P)<14)?&DDRB:&DDRC))
#define pinOfPin(P)\
  (((P)>=0&&(P)<8)?&PIND:(((P)>7&&(P)<14)?&PINB:&PINC))
#define pinIndex(P)((uint8_t)(P>13?P-14:P&7))
#define pinMask(P)((uint8_t)(1<<pinIndex(P)))

#define pinAsInput(P) *(ddrOfPin(P))&=~pinMask(P)
#define pinAsInputPullUp(P) *(ddrOfPin(P))&=~pinMask(P);digitalHigh(P)
#define pinAsOutput(P) *(ddrOfPin(P))|=pinMask(P)
#define digitalLow(P) *(portOfPin(P))&=~pinMask(P)
#define digitalHigh(P) *(portOfPin(P))|=pinMask(P)
#define isHigh(P)((*(pinOfPin(P))& pinMask(P))>0)
#define isLow(P)((*(pinOfPin(P))& pinMask(P))==0)
#define digitalState(P)((uint8_t)isHigh(P))







#define CONNECTION_LOST_TIME_INTERVAL 2000



#define SENSOR_TYPE_NONE 0
#define SENSOR_TYPE_LINE 1
#define SENSOR_TYPE_LED  2
#define SENSOR_TYPE_LIGHT 3
#define SENSOR_TYPE_TOUCH 4
#define SENSOR_TYPE_PROXIMITY 5
#define SENSOR_TYPE_ULTRASONIC 6
#define SENSOR_TYPE_COLOR 7


#define DIGITAL_PIN_1 4
#define DIGITAL_PIN_2 7
#define DIGITAL_PIN_3 8
#define DIGITAL_PIN_4 11
#define DIGITAL_PIN_5 12



ServoTimer2 myservo;



#define SENSOR_COUNT 5

#define SENSOR_RESPONSE_LENGTH 4



char blue_num;
const byte MOTOR_STATE_DISABLED = 0;
const byte MOTOR_STATE_ENABLED = 1;
const byte MOTOR_STATE_STEPS_LIMIT = 2;

const byte DIRECTION_FORWARD = 0;
const byte DIRECTION_BACKWARD = 0xFF;







struct Motor {
  public:
    byte direction;
    volatile byte currentSpeed;
    byte maxSpeed;
    volatile unsigned int stepsLimit;
    volatile unsigned int PIDpath;//счётчик для ПИД регулятора
    volatile unsigned int stepsCnt;
    volatile unsigned int stepsDistance;
    volatile unsigned int stepsPath;
    volatile unsigned int PIDspeed;//начальная скорость для корректировки ПИДа
    byte directionPin;
    byte speedPin;

    void initMotor(byte dPin, byte sPin) {
      direction = DIRECTION_FORWARD;
      currentSpeed = 0;
      maxSpeed = 32;
      stepsLimit = 0;
      PIDpath=0;
      PIDspeed=0;
      stepsCnt = 0;

      directionPin = dPin;
      speedPin = sPin;

      pinMode(directionPin, OUTPUT);
      pinMode(speedPin, OUTPUT);
    }

    void stop() {
      currentSpeed = 0;
      
//      analogWrite(speedPin, 0);
//      analogWrite(directionPin, 0);
      digitalWrite(speedPin, HIGH);
      digitalWrite(directionPin, HIGH);
   }


    void enableAndSetStepsLimit(unsigned int iStepsLimit) {
      stepsCnt = 0;
      stepsLimit = iStepsLimit;
    }






    void setSpeedAndDirection(byte speed, byte dir) {
      direction = dir;
      maxSpeed = speed;
      currentSpeed = maxSpeed;

      if(speed == 0){
         digitalWrite(speedPin, HIGH);
         digitalWrite(directionPin, HIGH);
      }
      else{
         if (dir == 0) {
           analogWrite(speedPin, currentSpeed * 4);
           analogWrite(directionPin, 0);
         }
         else {
           analogWrite(speedPin, 0);
           analogWrite(directionPin, currentSpeed * 4);
         }
      }
    }








    void onStep() {
      if (stepsLimit > 0) {
        stepsCnt++;
      }

      stepsPath++;
       PIDpath++;

      if (direction == DIRECTION_FORWARD) {
        stepsDistance++;
      }
      else {
        stepsDistance--;
      }


      if (stepsCnt > 65535) {
        stepsCnt = 0;
      }
      if (stepsPath > 65535) {
        stepsPath = 0;
      }
      
      if (PIDpath > 65535) {
        PIDpath = 0;
      }

      if (stepsDistance > 65535) {
        stepsDistance = 0;
      }
    }

};



float calibraterdl = 1 ;                                                 //my 
float calibrateldr = 1 ;
unsigned int lastENCl=0;
unsigned int lastENCr=0;
byte ENCdel=1;
byte commandState;
const byte COMMAND_STATE_WAITING_COMMAND = 0;
const byte COMMAND_STATE_WAITING_COMMAND_TYPE = 1;
const byte COMMAND_STATE_WAITING_DATA = 2;
const byte COMMAND_STATE_WAITING_CRC = 3;
const byte COMMAND_STATE_EXECUTING = 4;

byte commandType;
byte flag=0;

Motor leftMotor;
Motor rightMotor;

float speed_proportion = 1;

byte globalLeftMotorSpeed;
byte globalRightMotorSpeed;

byte globalLeftDir;
byte globalRightDir;


float m_lastInput = 0;
float m_lastResult = 0;







void onLeftMotorStep() {
  leftMotor.onStep();
  if ((leftMotor.stepsLimit > 0) && (leftMotor.stepsCnt >= leftMotor.stepsLimit)) {
    leftMotor.stepsLimit = 0;
    leftMotor.stop();
    if(flag==0)
    {
      rightMotor.setSpeedAndDirection(15,globalRightDir);
    }
    else 
    {
      rightMotor.stepsLimit = 0;
      rightMotor.stop();
    }
    flag=2;
  }
}

void onRightMotorStep() {
  rightMotor.onStep();
  if ((rightMotor.stepsLimit > 0) && ( rightMotor.stepsCnt >= rightMotor.stepsLimit)) {
    rightMotor.stepsLimit = 0;
    rightMotor.stop();
    if(flag==0)
    {
    leftMotor.setSpeedAndDirection(15,globalLeftDir);
    }
    else
    {
      leftMotor.stepsLimit = 0;
      leftMotor.stop();
    }
    flag=2;
  }

}





char chararrSerialRaw[50];
char chararrModel[21];
char chararrVersion[21];
char chararrPart[21];
char chararrSerial[21];
int MODEL_ID;



void parseSerialNumber() {
  EEPROM.get(SERIAL_ADDRESS, chararrSerialRaw);


  int iPointer = 0;
  while (chararrSerialRaw[iPointer] != '-') {
    iPointer++;
  }
  iPointer++;

  int iModelOffset = 0;
  while (chararrSerialRaw[iPointer] != '-') {
    chararrModel[iModelOffset] = chararrSerialRaw[iPointer];
    iModelOffset++;
    iPointer++;
  }
  iPointer++;

  int iVersionOffset = 0;
  while (chararrSerialRaw[iPointer] != '-') {
    chararrVersion[iVersionOffset] = chararrSerialRaw[iPointer];
    iVersionOffset++;
    iPointer++;
  }

  iPointer++;

  int iPartOffset = 0;
  while (chararrSerialRaw[iPointer] != '-') {
    chararrPart[iPartOffset] = chararrSerialRaw[iPointer];
    iPartOffset++;
    iPointer++;
  }

  iPointer++;


  int iSerialOffset = 0;
  while (chararrSerialRaw[iPointer] != 0 && (chararrSerialRaw[iPointer] >= '0' && chararrSerialRaw[iPointer] <= '9')) {
    chararrSerial[iSerialOffset] = chararrSerialRaw[iPointer];
    iSerialOffset++;
    iPointer++;
  }


  if (strcmp(chararrModel, "R") == 0
      && strcmp(chararrVersion, "1") == 0
      /* && (strcmp(chararrPart, "1") == 0 || strcmp(chararrPart, "2") == 0 || strcmp(chararrPart, "3") == 0 || strcmp(chararrPart, "4") == 0 || strcmp(chararrPart, "5") == 0) */) {

    MODEL_ID = 0;
  }
  else if (strcmp(chararrModel, "L") == 0
           && strcmp(chararrVersion, "1") == 0
           /* && strcmp(chararrPart, "1") == 0 */ ) {

    MODEL_ID = 1;
  }
  else if (strcmp(chararrModel, "L") == 0
           && strcmp(chararrVersion, "3") == 0
           /* && (strcmp(chararrPart, "1") == 0 || strcmp(chararrPart, "2") == 0 || strcmp(chararrPart, "3") == 0) */ ) {

    MODEL_ID = 2;
  }
  else {
    MODEL_ID = 9999;
  }

}


byte byteActiveColorSensor = 0;


//boolean time = false;

class ISensor {
  public:
    virtual byte getType();
    virtual void reset();
    virtual void iteration(byte data, byte data2);
    virtual boolean isReady();
    virtual byte* getResult();

    virtual void debugSetValue(byte value);
};



class AnalogSensor: public ISensor {
    int pin;
    byte type = 0;

    byte byteResult = 0;

  public:
    AnalogSensor(int pin, byte type) {
      this -> pin = pin;
      this -> type = type;
      pinMode(pin, INPUT);

      //Let's set 1 to digital out
      //So that the the lamps light
      switch (pin) {
        case A1: {
            pinMode(DIGITAL_PIN_1, OUTPUT);
            break;
          }
        case A2: {
            pinMode(DIGITAL_PIN_2, OUTPUT);
            break;
          }
        case A3: {
            pinMode(DIGITAL_PIN_3, OUTPUT);
            break;
          }
        case A4: {
            pinMode(DIGITAL_PIN_4, OUTPUT);
            break;
          }
        case A5: {
            pinMode(DIGITAL_PIN_5, OUTPUT);
            break;
          }
      }
    };

   void  debugSetValue(byte value){

       byteResult = value;
    
   }

    byte getType() {
      return type;
    }


    void reset() {
      byteResult = 0;
    }


    void iteration(byte data, byte data2) {
      byteResult = byte(analogRead(pin) >> 2);
    }

    boolean isReady() {
      return true;
    }

    byte* getResult() {
      return new byte[SENSOR_RESPONSE_LENGTH] {0, 0, 0, byteResult};
    }
};







class SonicSensor: public ISensor {
    boolean resetMode = true;
    boolean measuremnetWaiting;
    unsigned long time;
    int result = 0;
    int pin;

    boolean bReady = false;

   #define SONIC_FILTER_LENGTH 9
   int history[SONIC_FILTER_LENGTH] = {};
   int history_[SONIC_FILTER_LENGTH] = {};

//   unsigned long lLastTimeRun  = millis();

  public:
    SonicSensor(int pin) {
      this -> pin = pin;

      for(int i = 0; i < SONIC_FILTER_LENGTH; i++){
         history[i] = 0;
      }
    };



    byte getType() {
      return SENSOR_TYPE_ULTRASONIC;
    }



    void reset() {
//      if(abs(millis() - lLastTimeRun) < 5000) return;
//      lLastTimeRun = millis();
      
      measuremnetWaiting = false;
      resetMode = true;
      bReady = false;
      time = micros();
    }

    void iteration(byte data, byte data2) {
      //reset mode with delay
      if (resetMode) {
        if (micros() - time > 30000) {
          pinAsOutput(pin);
          digitalWrite(pin, HIGH);
          delayMicroseconds(9);
          digitalWrite(pin, LOW);
          pinAsInput(pin);
          time = micros();
          resetMode = false;
        }

        return;
      }


      // no response
      // we lost the impulse
      if (micros() - time > 100000) {
        bReady = true;
        return;
      }


      if (measuremnetWaiting) {
        if (LOW == digitalState(pin)) {
          result = ((micros() - time) *  34000) / 2000000 - 14;
          if (result < 0) {
            result = 0;
          }

          for(int i = 1; i < SONIC_FILTER_LENGTH; i++){
            history[i - 1] = history[i];
          }
          history[SONIC_FILTER_LENGTH - 1] = result;


          for(int i = 0; i < SONIC_FILTER_LENGTH; i++){
            history_[i] = history[i];
          }
          


          for(int i = 0; i < SONIC_FILTER_LENGTH; i++){
             for(int j = i + 1; j < SONIC_FILTER_LENGTH; j++){
               if(history_[i] < history_[j]){
                  int iTemp = history_[i];
                  history_[i] = history_[j];
                  history_[j] = iTemp;
               }
             }
          }


          result = history_[4];
          
          
          bReady = true;
        }
      }
      else {
        if (digitalState(pin) == HIGH) {
          measuremnetWaiting = true;
        }
      }
    }

    void  debugSetValue(byte value){

     
    
   }

    boolean isReady() {
      return bReady;
    }

    byte* getResult() {
      return new byte[SENSOR_RESPONSE_LENGTH] {0, 0, 0, (byte)result};
    }
};








class ColorSensor: public ISensor {
   int pin;
   
  
   unsigned int INTERVAL_BIT;
   unsigned int INTERVAL_BYTE;
   boolean booleanReady;

   unsigned long timeLowStart = 0;
   unsigned long timeLowEnd = 0;
   unsigned long timeHighStart = 0;
   unsigned long timeHighEnd = 0;

   unsigned long timeSync = 0;
    int INTERVAL_BIT_ALL=0;
    int INTERVAL_BIT_NEW=0;
    byte TRIES=0;
   boolean flag2=0;
   byte byteCounter = 0;
   byte bitCounter = 0;
   byte result[3] = {0, 0, 0};
   byte _result[3] = {0, 0, 0};

   byte iSynchronized = 0;
   
   
   int iSyncAttempts = 0;


  public:
    ColorSensor(int pin) {
      this -> pin = pin;
      pinAsInput(pin);
      INTERVAL_BIT  = 114;
      INTERVAL_BYTE = 912;
    };
    void  debugSetValue(byte value){

     
    
   }

    byte getType() {
      return SENSOR_TYPE_COLOR;
    };


void reset() {
      booleanReady = false;
      timeLowStart = 0;
      timeLowEnd = 0;
      timeHighStart = 0;
      timeHighEnd = 0;
      timeSync = 0;
      byteCounter = 0;
      bitCounter = 0;
      //      result[0] = 0;
      //      result[1] = 0;
      //      result[2] = 0;
      _result[0] = 0;
      _result[1] = 0;
      _result[2] = 0;
      iSynchronized = 0;
     if(TRIES<25&&flag2){
             TRIES++;
             if(TRIES>5){
      INTERVAL_BIT_ALL+=INTERVAL_BIT_NEW;
      INTERVAL_BIT=round(INTERVAL_BIT_ALL/(TRIES-5));
      INTERVAL_BYTE=INTERVAL_BIT*8;
    //  Serial.println(INTERVAL_BIT_ALL);
    //       Serial.println(TRIES); 
     // Serial.println(INTERVAL_BIT);
             }
      }
      else
      flag2=1;
    }

    void iteration(byte data, byte data2) {
      switch (iSynchronized) {
        case 0: {
            if (digitalState(pin) == HIGH) {
              iSynchronized = 1;
              timeLowStart = micros();
            }
            else {
              iSynchronized = 0;
            }
            break;
          }
        case 1: {
            if (digitalState(pin) == LOW) {
              timeLowEnd = micros();
              if (timeLowEnd - timeLowStart >= (110)*14) {
                iSynchronized = 2;
              }
            }
            else {
              iSynchronized = 0;
            }
            break;
          }
        case 2: {
               if (digitalState(pin) == HIGH) {
      //                Serial.println(timeLowEnd - timeLowStart);
       timeHighStart = micros();
              iSynchronized = 3;
            }
            else {
              timeLowEnd = micros();
            }
            break;
          }
        case 3: {
          INTERVAL_BIT_NEW=round((timeLowEnd-timeLowStart)/14);//mb +114*2/16
          timeSync=timeLowEnd+INTERVAL_BIT+20;
   //       Serial.println(INTERVAL_BIT);
          iSynchronized = 4;
          }
        case 4: {
            if (timeSync <= micros()) {
              timeSync += INTERVAL_BIT;
              if (digitalState(pin) == HIGH) {
                _result[byteCounter] |= (1 << bitCounter);
              }
              else {
                _result[byteCounter] &= ~(1 << bitCounter);
              }
              bitCounter++;
              if (bitCounter > 7) {
                bitCounter = 0;
                byteCounter++;
                if (byteCounter > 2) {
                  //We done
                  byteCounter = 0;
                  iSynchronized = 0;                     
                  if(abs(result[0] - _result[0]) >= 32 && abs(result[1] - _result[1]) < 5 && abs(result[2] - _result[2]) < 5
                          || abs(result[0] - _result[0]) < 5 && abs(result[1] - _result[1]) >= 32 && abs(result[2] - _result[2]) < 5
                          || abs(result[0] - _result[0]) < 5 && abs(result[1] - _result[1]) < 5 && abs(result[2] - _result[2]) >= 32){

                          }
                          else
                          {
                           
                  result[0] = _result[0];
                  result[1] = _result[1];
                  result[2] = _result[2];
                  
                  }
                  booleanReady = true;
                }
              }
            }
            break;
          }
      }
    }

    boolean isReady() {

      /*if (  (result[0] == 0) && (result[1] == 0) && (result[2] == 0 ) ) {
              booleanReady = false; 
        
      }else{
           booleanReady = true;
        
      }*/
      
      return booleanReady;
    }

    byte* getResult() {
      return new byte[SENSOR_RESPONSE_LENGTH] {0, result[0], result[1], result[2]};
    }
};




ISensor* sensors[5];



boolean fuck = false;


//void iterator(){
//
////   for(int f = 0; f < 5; f++){
////      sensors[f] -> iteration();
////   }
//
//
///*
//   pinMode(11, OUTPUT);
//   if(fuck){
//      digitalWrite(11, HIGH);
//      fuck = false;
//   }
//   else{
//      digitalWrite(11, LOW);
//      fuck = true;
//   }
//*/
//}
void calibrating()
{
  
    //считываем энкодеры
   delay(800);
    while(ENCdel>0)                                                 //my
    {
      delay(200);
      ENCdel=leftMotor.stepsPath-lastENCl+rightMotor.stepsPath-lastENCr;
      lastENCl=leftMotor.stepsPath;
      lastENCr=rightMotor.stepsPath;
    }
    
    if(leftMotor.stepsPath>rightMotor.stepsPath)
    {
      calibrateldr=1;
      calibraterdl = (float) rightMotor.stepsPath / (float)leftMotor.stepsPath;
      EEPROM.put(ADDRCALIB,calibraterdl);
    }
    else
    {
      calibraterdl=1;
      calibrateldr = (float)leftMotor.stepsPath / (float)rightMotor.stepsPath;
       EEPROM.put(ADDRCALIB,calibrateldr*(-1));
    }
    
}



void setup() {
  //Let's load S/N
  parseSerialNumber();

  Serial.begin(SERIAL_SPEED);

  //Let's set the PWR timer
  bitSet(TCCR1B, WGM12);

  myservo.attach(13);
  myservo.write(500);
  

  commandState = COMMAND_STATE_WAITING_COMMAND;


  leftMotor.initMotor(10, 9);
  rightMotor.initMotor(6, 5);

  attachInterrupt(1, onLeftMotorStep,  CHANGE);
  attachInterrupt(0, onRightMotorStep, CHANGE);
    m_lastInput = 0;
    m_lastResult = 0;
    float calib;
  /*  EEPROM.get(ADDRCALIB,calib);
    if(calib>0)
   {
    calibraterdl=calib;
    calibrateldr=1;
    }
    else
    {
      calibraterdl-1;
      calibrateldr=-calib;
    }
//    calibrating();*/
  EEPROM.get(BLUE_CHECK, blue_num);
    if(!blue_num)
  {
   Serial.write("$$$");
   delay(100);
   char a[3];
     a[0] =Serial.read();
    a[1] =Serial.read();
    a[2] =Serial.read();
    a[2] =Serial.read();
   delay(1000);
   if(a[0]=='C'){
      Serial.println("SQ,16");
      delay(100);
      a[0] =Serial.read();
      a[1] =Serial.read();
      a[2] =Serial.read();
      a[2] =Serial.read();
      delay(100);
      if(a[1]=='A'){
      digitalWrite(10,HIGH);
      delay(100);
      digitalWrite(10,LOW);
      EEPROM.put(BLUE_CHECK, '1');//   <----------------------------- У EEPROM ограничеено количество циклов записи, сначала думай, потом раскомменчивай!
      Serial.println("---");
      }
      else
      {
      digitalWrite(9,HIGH);
      delay(2000);
      digitalWrite(9,LOW);
      }
   }
    

    
  }


  
  sensors[0]  = new AnalogSensor(A1, SENSOR_TYPE_LINE);
  //   sensors[0]  = new ColorSensor(DIGITAL_PIN_1);
  sensors[1]  = new AnalogSensor(A2, SENSOR_TYPE_LINE);
  sensors[2]  = new AnalogSensor(A3, SENSOR_TYPE_LINE);
  sensors[3]  = new AnalogSensor(A4, SENSOR_TYPE_LINE);
  sensors[4]  = new AnalogSensor(A5, SENSOR_TYPE_LINE);


}
float last_input=0;
float kli = 5;//5
float kii=  0.995;//0.5
float diff=5;
void update_power_using_PID(byte leftSpeed, byte rightSpeed,float input ){
 float corrector_koef = 0; 
 int leftSpeedCorrected ;
 int rightSpeedCorrected;
        leftSpeedCorrected = leftSpeed + input*input*((input>0)+(input<0)*(-1))+kli*last_input+(input-last_input)*diff;//5
        rightSpeedCorrected= rightSpeed-input*input*((input>0)+(input<0)*(-1))-kli*last_input-(input-last_input)*diff;
        if(leftSpeedCorrected >63)
        {
        rightSpeedCorrected-=(leftSpeedCorrected-63);
        leftSpeedCorrected =63;
        }
        if(rightSpeedCorrected >63)
        {
          leftSpeedCorrected-=(rightSpeedCorrected-63);
          rightSpeedCorrected =63;
        }
        if(leftSpeedCorrected <0)
        leftSpeedCorrected =0;
        if(rightSpeedCorrected <0)
        rightSpeedCorrected =0;
       leftMotor.setSpeedAndDirection((byte)leftSpeedCorrected,    globalLeftDir);
       rightMotor.setSpeedAndDirection((byte)rightSpeedCorrected,  globalRightDir);
       if(kii!=1)
        last_input=last_input*kii+input;//0.5
       else
       last_input=0;
}


void printSensors() {


/*
  //Looking for UltraSonic in progress
  boolean pendingSensors[5] = {false, false, false, false, false};
  for (int i = 0; i < SENSOR_COUNT; i++) {
    byte byteType = sensors[i] -> getType();
    if (byteType == SENSOR_TYPE_ULTRASONIC) {
      SonicSensor sonicSensor = sensors[i];
      if (sonicSensor.isReady()) {
        //This one is ok
      }
      else {
        pendingSensors[i] = true;
      }
    }
  }
*/




  Serial.write('#');
  Serial.write( (byte)((leftMotor.stepsCnt >> 8) & 0xff));
  Serial.write( (byte)((leftMotor.stepsCnt) & 0xff));
  Serial.write( (byte)((rightMotor.stepsCnt >> 8) & 0xff));
  Serial.write( (byte)((rightMotor.stepsCnt) & 0xff));


  Serial.write( (byte)((leftMotor.stepsPath >> 8) & 0xff));
  Serial.write( (byte)((leftMotor.stepsPath) & 0xff));
  Serial.write( (byte)((rightMotor.stepsPath >> 8) & 0xff));
  Serial.write( (byte)((rightMotor.stepsPath) & 0xff));







  for (int i = 0; i < SENSOR_COUNT; i++) {
    byte* result = sensors[i] -> getResult();
    Serial.write(result, SENSOR_RESPONSE_LENGTH);
    delete[] result;
  }



  byte sensorValue = (byte) (analogRead(A0) >> 2);
  Serial.write(sensorValue);


/*
  //Checking the UltraSonic progress
  for (int i = 0; i < SENSOR_COUNT; i++) {
    byte byteType = sensors[i] -> getType();
    if (byteType == SENSOR_TYPE_ULTRASONIC && pendingSensors[i] && sensors[i] -> isReady()) {
      sensors[i] -> reset();
      sensors[i] -> iteration(0, 0);
    }
  }
*/


  //analog
  for (int i = 0; i < SENSOR_COUNT; i++) { //modified_by_yaroslav //for PID test purpose //i=0
    byte byteType = sensors[i] -> getType();
    if (byteType != SENSOR_TYPE_ULTRASONIC && byteType != SENSOR_TYPE_COLOR) {
      sensors[i] -> reset();
      sensors[i] -> iteration(0, 0);
    }
  }



  //sensors[0] -> reset();
  //while(!sensors[0] -> isReady()){
  //   sensors[0] -> iteration(0, 2);
  //}
  //sensors[1] -> reset();
  //while(!sensors[1] -> isReady()){
  //   sensors[1] -> iteration(1, 2);
  //}








  //color
  byte tmp = 0;
  byte totalColor = 0;
  for (int i = 0; i < SENSOR_COUNT; i++) {
    byte byteType = sensors[i] -> getType();
    if (byteType == SENSOR_TYPE_COLOR) {
      if (totalColor == byteActiveColorSensor) {
        sensors[i] -> reset();
      }
      totalColor++;
    }
  }


  //byteActiveColorSensor

  tmp = 0;

  unsigned long timeLimit = micros();
  while (totalColor > 0) {
    boolean allDone = true;

    for (int i = 0; i < SENSOR_COUNT; i++) {
      byte byteType = sensors[i] -> getType();
      if (byteType == SENSOR_TYPE_COLOR) {
        if (tmp < byteActiveColorSensor) {
          tmp++;
          continue;
        }

         unsigned long color_loop_breaker = micros();
         unsigned long color_loop_checker = micros();

        while (!sensors[i] -> isReady()) {
          sensors[i] -> iteration(i, totalColor);
          
          color_loop_checker = micros();

          if (( color_loop_checker - color_loop_breaker) > 1000 * 10){

             break;
          }
          

         /* byte * color_result_buf =  sensors[i]->getResult();
         if ( (color_result_buf[1] == 0) && (color_result_buf[2] == 0) && (color_result_buf[3] == 0 ) ) {
            break;
        
          } */
          
        }

        break;
      }
    }

    //Finished
    //      if(allDone || abs(micros() - timeLimit) > INTERVAL_BYTE * 10) break;
    if (allDone || abs(micros() - timeLimit) > 1000 * 10) break;
  }
 #ifdef TESTMODE_COLOR_SENSOR
      digitalLow(11);
   #endif
  
  byteActiveColorSensor++;
  if (byteActiveColorSensor == totalColor) {
    byteActiveColorSensor = 0;
  }


  //sonic
  for (int i = 0; i < SENSOR_COUNT; i++) {
    byte byteType = sensors[i] -> getType();
    if (byteType == SENSOR_TYPE_ULTRASONIC) {
      if (sensors[i] -> isReady()) {
        sensors[i] -> reset();
      }
    }
  }


  //   while(true){
  //      boolean allDone = true;
  //      for(int i = 0; i < SENSOR_COUNT; i++){
  //         byte byteType = sensors[i] -> getType();
  //         if(byteType == SENSOR_TYPE_ULTRASONIC){
  //            if(sensors[i] -> isReady()){
  //            }
  //            else{
  //               allDone = false;
  //               sensors[i] -> iteration(0, 0);
  //            }
  //         }
  //      }
  //
  //      //Finished
  //      if(allDone) break;
  //   }
}





void printSensorDebug() {
  Serial.print("\nL:[steps=" + String(leftMotor.stepsCnt)
               + " limit=" + String(leftMotor.stepsLimit)
               + " path=" + String(leftMotor.stepsPath)
               + " distance=" + String(leftMotor.stepsDistance)
               + "]");
  Serial.print("\nR:[stepsR=" + String(rightMotor.stepsCnt)
               + " limit=" + String(rightMotor.stepsLimit)
               + " path=" + String(rightMotor.stepsPath)
               + " distance=" + String(rightMotor.stepsDistance)
               + "]");
}



//void update_speed_using_PID(byte leftPower, byte rightPower){
//
//
//float power_proportion;
//      
//  
//}

byte bytearrayData[20];
byte byteDataTail = 0;
byte command = 0;

unsigned long lastReceivedCommandTime = millis();

void loop() {
  if (millis() - lastReceivedCommandTime > CONNECTION_LOST_TIME_INTERVAL) {
    leftMotor.stop();
    rightMotor.stop();
  }
    float e = 0; //невязка
    unsigned int stepsPathDelta = 0;
    if (leftMotor.PIDpath > (unsigned int)((float)rightMotor.PIDpath)*speed_proportion ){
       stepsPathDelta = leftMotor.PIDpath - (unsigned int)((float)rightMotor.PIDpath)*speed_proportion;
    }else{
           stepsPathDelta = (unsigned int)((float)rightMotor.PIDpath)*speed_proportion -  leftMotor.PIDpath;
    }
  if (/* ( stepsPathDelta >1 ) && */(leftMotor.stepsPath > 0) && (rightMotor.stepsPath > 0) && (globalLeftMotorSpeed > 0) && (globalRightMotorSpeed > 0)&&(flag==0) )
  {          
       // steps_proportion =  steps_proportion.div(Fixed::fromInt(leftMotor.stepsPath),Fixed::fromInt(rightMotor.stepsPath));
           //   e = steps_proportion - speed_proportion; //вычисляем невязку
          e= ((float)rightMotor.PIDpath)*speed_proportion - (float)leftMotor.PIDpath;// 
                 update_power_using_PID(leftMotor.PIDspeed,rightMotor.PIDspeed,e);

}
else
{
  if(flag==2)
{
  leftMotor.stop();
  rightMotor.stop();
}
}

  if (Serial.available()) {

    byte b = Serial.read();
    lastReceivedCommandTime = millis();



    if (commandState == COMMAND_STATE_WAITING_COMMAND) {
      switch (b) {
        case ' ': {
            Serial.print(F("ROBBO-"));
            if (MODEL_ID < 10000) {
              Serial.write('0');
            }
            if (MODEL_ID < 1000) {
              Serial.write('0');
            }
            if (MODEL_ID < 100) {
              Serial.write('0');
            }
            if (MODEL_ID < 10) {
              Serial.write('0');
            }
            Serial.print(MODEL_ID);



            Serial.write('-');
            Serial.print(F(FIRMWARE_VERSION));


            Serial.write('-');
            Serial.print(chararrModel);


            Serial.print('-');
            for (int f = strlen(chararrVersion); f < 5; f++) {
              Serial.write('0');
            }
            Serial.print(chararrVersion);




            Serial.print('-');
            for (int f = strlen(chararrPart); f < 5; f++) {
              Serial.write('0');
            }
            Serial.print(chararrPart);




            Serial.print('-');
            for (int f = strlen(chararrSerial); f < 20; f++) {
              Serial.write('0');
            }
            Serial.print(chararrSerial);

            break;
          }
        case '!': {
            Serial.print(chararrSerialRaw);

            break;
          }
        case 'a': {
            command = b;
            commandState = COMMAND_STATE_WAITING_CRC;
            break;
          }
        case 'b': {
            command = b;
            commandState = COMMAND_STATE_WAITING_CRC;
            break;
          }
        case 'c': {
            commandState = COMMAND_STATE_WAITING_DATA;
            byteDataTail = 0;
            command = b;
            break;
          }
        case 'd': {
            commandState = COMMAND_STATE_WAITING_DATA;
            byteDataTail = 0;
            command = b;
            break;
          }
        case 'e': {
            commandState = COMMAND_STATE_WAITING_DATA;
            byteDataTail = 0;
            command = b;
            break;
          }
        case 'f': {
            commandState = COMMAND_STATE_WAITING_DATA;
            byteDataTail = 0;
            command = b;
            break;
          }
        case 'g': {
            commandState = COMMAND_STATE_WAITING_DATA;
            byteDataTail = 0;
            command = b;
            break;
          }
        case 'h': {
            commandState = COMMAND_STATE_WAITING_DATA;
            byteDataTail = 0;
            command = b;
            break;
          }
        case 'i': {
            commandState = COMMAND_STATE_WAITING_DATA;
            byteDataTail = 0;
            command = b;
            break;
          }
        case 'j': {
            commandState = COMMAND_STATE_WAITING_DATA;
            byteDataTail = 0;
            command = b;
            break;
          }
      }
    }
    else if (commandState == COMMAND_STATE_WAITING_DATA) {
      bytearrayData[byteDataTail] = b;
      byteDataTail++;

      switch (command) {
        case 'c': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
          }
        case 'd': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
          }
        case 'e': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
          }
        case 'f': {
            if (byteDataTail > 0) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
          }
        case 'g': {
            if (byteDataTail > 3) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
          }
        case 'h': {
            if (byteDataTail > 0) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
          }
        case 'i': {
            if (byteDataTail > 4) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
          }
        case 'j': {
            if (byteDataTail > 0) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
          }
      }
    }
    else if (commandState == COMMAND_STATE_WAITING_CRC) {

      if (b == '$') {
        switch (command) {
          case 'a': {
              printSensors();
              break;
            }
          case 'b': {
              printSensorDebug();
              break;
            }
          case 'c': {
              byte leftSpeed = bytearrayData[0];
              byte rightSpeed = bytearrayData[1];
              flag=0;
              byte leftDir = DIRECTION_FORWARD;
              byte rightDir = DIRECTION_FORWARD;

              if ( leftSpeed >= 64) {
                leftDir = DIRECTION_BACKWARD;
                leftSpeed -= 64;
              }
              if ( rightSpeed >= 64) {
                rightDir = DIRECTION_BACKWARD;
                rightSpeed -= 64;
              }
               speed_proportion = ((float)leftSpeed / (float) rightSpeed) * calibraterdl / calibrateldr;
               globalLeftMotorSpeed = leftSpeed;
               globalRightMotorSpeed = rightSpeed;
               globalLeftDir = leftDir;
               globalRightDir = rightDir;
               leftMotor.PIDspeed=leftSpeed;
               rightMotor.PIDspeed=rightSpeed;
              if((leftSpeed==0)&&(rightSpeed==0))//проверка на обнуление ПИД путей
              {
              leftMotor.PIDpath=0;
              rightMotor.PIDpath=0;
              }
              leftMotor.setSpeedAndDirection(leftSpeed, leftDir);///
              rightMotor.setSpeedAndDirection(rightSpeed, rightDir);//
              printSensors();
              break;
            }
          case 'd': {
              byte leftSpeed = bytearrayData[0] - '0';
              byte rightSpeed = bytearrayData[1] - '0';

              byte leftDir = DIRECTION_FORWARD;
              byte rightDir = DIRECTION_FORWARD;

              if ( leftSpeed >= 64) {
                leftDir = DIRECTION_BACKWARD;
                leftSpeed -= 64;
              }
              if ( rightSpeed >= 64) {
                rightDir = DIRECTION_BACKWARD;
                rightSpeed -= 64;
              }

              leftMotor.setSpeedAndDirection(leftSpeed, leftDir);
              rightMotor.setSpeedAndDirection(rightSpeed, rightDir);

              printSensorDebug();
              break;
            }
          case 'e': {
              byte stepsHigh = bytearrayData[0];
              byte stepsLow = bytearrayData[1];

              unsigned int iStepsLimit = stepsHigh;
              iStepsLimit = iStepsLimit << 8;
              iStepsLimit += stepsLow;

              leftMotor.enableAndSetStepsLimit(iStepsLimit);
              rightMotor.enableAndSetStepsLimit(iStepsLimit);

              printSensors();
              break;
            }
          case 'f': {
              byte steps = bytearrayData[0] - '0';

              leftMotor.enableAndSetStepsLimit(steps);
              rightMotor.enableAndSetStepsLimit(steps);

              printSensorDebug();
              break;
            }
          case 'g': {
              byte leftSpeed = bytearrayData[0];
              byte rightSpeed = bytearrayData[1];
              byte stepsHigh = bytearrayData[2];
              byte stepsLow = bytearrayData[3];
              byte leftDir = DIRECTION_FORWARD;
              byte rightDir = DIRECTION_FORWARD;
              flag=0;
              leftMotor.PIDpath=0;
              rightMotor.PIDpath=0;
              if ( leftSpeed >= 64) {
                leftDir = DIRECTION_BACKWARD;
                leftSpeed -= 64;
              }
              if ( rightSpeed >= 64) {
                rightDir = DIRECTION_BACKWARD;
                rightSpeed -= 64;
              }
              //if(leftSpeed!=rightSpeed)
              
              if(leftSpeed==rightSpeed)
              kii=0.92 -(63-leftSpeed)*0.02;
              else
              {kii=1;flag=1;}
              unsigned int iStepsLimit = stepsHigh;
              iStepsLimit = iStepsLimit << 8;
              iStepsLimit += stepsLow;
              speed_proportion = (float)leftSpeed / (float) rightSpeed*calibraterdl/calibrateldr;//соотношение скоростей и путей соответственно
              globalLeftDir = leftDir;
              globalRightDir = rightDir;
              globalLeftMotorSpeed=leftSpeed;
              globalRightMotorSpeed=rightSpeed;
              leftMotor.PIDspeed=leftSpeed;
              rightMotor.PIDspeed=rightSpeed;
              leftMotor.setSpeedAndDirection(leftSpeed, leftDir);
              rightMotor.setSpeedAndDirection(rightSpeed, rightDir);
              leftMotor.enableAndSetStepsLimit(iStepsLimit);
              rightMotor.enableAndSetStepsLimit(iStepsLimit);

              printSensors();
              break;
            }
          case 'h': {
              byte lamps = bytearrayData[0];
              if (lamps & 1) {
                digitalWrite(4, HIGH);
              }
              else {
                digitalWrite(4, LOW);
              }

              if (lamps & 2) {
                digitalWrite(7, HIGH);
              }
              else {
                digitalWrite(7, LOW);
              }


              if (lamps & 4) {
                digitalWrite(8, HIGH);
              }
              else {
                digitalWrite(8, LOW);
              }


              if (lamps & 8) {
                digitalWrite(11, HIGH);
              }
              else {
                digitalWrite(11, LOW);
              }


              if (lamps & 16) {
                digitalWrite(12, HIGH);
              }
              else {
                digitalWrite(12, LOW);
              }




              printSensors();
              break;
            }
          case 'i': {
             //IMPORTANT
             //Color sensor count be changed!
              byteActiveColorSensor = 0;
              
              for (int f = 0; f < 5; f++) {
                delete sensors[f];
                switch (bytearrayData[f]) {
                  case SENSOR_TYPE_NONE:
                  case SENSOR_TYPE_LINE:
                  case SENSOR_TYPE_LED:
                  case SENSOR_TYPE_LIGHT:
                  case SENSOR_TYPE_TOUCH:
                  case SENSOR_TYPE_PROXIMITY: {
                      switch (f) {
                        case 0: {
                            sensors[f]  = new AnalogSensor(A1, bytearrayData[f]);
                            break;
                          }
                        case 1: {
                            sensors[f]  = new AnalogSensor(A2, bytearrayData[f]);
                            break;
                          }
                        case 2: {
                            sensors[f]  = new AnalogSensor(A3, bytearrayData[f]);
                            break;
                          }
                        case 3: {
                            sensors[f]  = new AnalogSensor(A4, bytearrayData[f]);
                            break;
                          }
                        case 4: {
                            sensors[f]  = new AnalogSensor(A5, bytearrayData[f]);
                            break;
                          }
                      }
                      break;
                    }
                  case SENSOR_TYPE_ULTRASONIC: {
                      switch (f) {
                        case 0: {
                            sensors[f]  = new SonicSensor(DIGITAL_PIN_1);
                            break;
                          }
                        case 1: {
                            sensors[f]  = new SonicSensor(DIGITAL_PIN_2);
                            break;
                          }
                        case 2: {
                            sensors[f]  = new SonicSensor(DIGITAL_PIN_3);
                            break;
                          }
                        case 3: {
                            sensors[f]  = new SonicSensor(DIGITAL_PIN_4);
                            break;
                          }
                        case 4: {
                            sensors[f]  = new SonicSensor(DIGITAL_PIN_5);
                            break;
                          }
                      }
                      break;
                    }
                  case SENSOR_TYPE_COLOR: {
                      switch (f) {
                        case 0: {
                            sensors[f]  = new ColorSensor(DIGITAL_PIN_1);
                            break;
                          }
                        case 1: {
                            sensors[f]  = new ColorSensor(DIGITAL_PIN_2);
                            break;
                          }
                        case 2: {
                            sensors[f]  = new ColorSensor(DIGITAL_PIN_3);
                            break;
                          }
                        case 3: {
                            sensors[f]  = new ColorSensor(DIGITAL_PIN_4);
                            break;
                          }
                        case 4: {
                            sensors[f]  = new ColorSensor(DIGITAL_PIN_5);
                            break;
                          }
                      }
                      break;
                    }
                }
              }

              printSensors();
              break;
            }
          case 'j': {
              myservo.write(bytearrayData[0] * 5 + 500);

              printSensors();
              break;
            }
        }
      }
      commandState = COMMAND_STATE_WAITING_COMMAND;
    }
  }


  for (int i = 0; i < SENSOR_COUNT; i++) {
    byte byteType = sensors[i] -> getType();
    if (byteType == SENSOR_TYPE_ULTRASONIC) {
      if (sensors[i] -> isReady()) {
      }
      else {
        sensors[i] -> iteration(0, 0);
      }
    }
  }


#ifdef TESTMODE_COLOR_SENSOR
   pinMode(11, OUTPUT);
   if(fuck){
      digitalHigh(11);
      fuck = false;
   }
   else{
      digitalLow(11);
      fuck = true;
   }
#endif
  //   for(int f = 0; f < 5; f++){
  //      sensors[f] -> iteration();
  //   }


  //   pinMode(11, OUTPUT);
  //   if(fuck){
  //      digitalHigh(11);
  //      fuck = false;
  //   }
  //   else{
  //      digitalLow(11);
  //      fuck = true;
  //   }
}
