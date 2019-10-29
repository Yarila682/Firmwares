#define FIRMWARE_VERSION "00003"
//#include <EEPROM.h>
#include "Arduino.h"
#include "IRremote.h"
#include "servotimer2.h"
#include "dht11.h"
#include "LCD_1602_RUS.h"
#define SERIAL_SPEED 38400
#define SERIAL_ADDRESS 0
#define data 2
#define clock 4
#include "HC_SR04.h"
#define NO_ONE      100
#define READ_DIG    0
#define READ_ANA    1
#define READ_PULSE  2
#define READ_ECHO   3
#define SET_DIG  4
#define SET_PWM  5
#define SONIC   6
#define SONIC2  7
#define SOUND   8
#define SERVO   9
#define PULT 10
#define VLAGA 11
#define TEMP 12
dht11 sensorr;
HC_SR04 sensor(9, 2, 0);
HC_SR04 sensor1(9, 3, 1);
byte commandState;
const byte COMMAND_STATE_WAITING_COMMAND=0;
const byte COMMAND_STATE_WAITING_COMMAND_TYPE=1;
const byte COMMAND_STATE_WAITING_DATA=2;
const byte COMMAND_STATE_WAITING_CRC=3;
const byte COMMAND_STATE_EXECUTING=4;
int inf;
byte sens[22];
ServoTimer2 myservo;
IRrecv irrecv(12);
decode_results results;
LCD_1602_RUS lcd(0x27, 16, 2);
unsigned long startMillis= millis();  // Start of sample window
byte curse=0;
byte old_sens=0;
byte pin_for_sonic=0;
byte pult_rec=99;
bool flag=0;
byte vlag_temp;
void printSensors(){
    flag=0;
    Serial.write('#');
    for(byte a=0;a<21;a++)
    {
      switch (sens[a]) {
      case READ_DIG:
      if(a>13)
      Serial.write(0);
      Serial.write((byte)digitalRead(a));
      break;
      case READ_ANA:
      inf = analogRead(a);

      Serial.write(byte(inf%256));
            if(a>13)
      Serial.write(bitRead(inf,8)+bitRead(inf,9)*2);
      break;
      case READ_PULSE:

      case READ_ECHO:

      case NO_ONE:
      case SET_DIG:
      case SET_PWM:
      case SONIC:
      Serial.write(old_sens);
      break;
      case PULT:
      Serial.write(byte(pult_rec));
      pult_rec=99;
      if(a>13)
      Serial.write(0);
      break;
      case SOUND:
      case SERVO:
      Serial.write(0);
      break;
      case VLAGA:
      if(!flag)
      {
      sensorr.read(vlag_temp);  
      flag=1;
      }
      Serial.write(sensorr.humidity);
      if(a>13)
      Serial.write(0);
      break;
      case TEMP:
      if(!flag)
      {
      sensorr.read(vlag_temp);  
      flag=1;
      }
      Serial.write(sensorr.temperature);
      if(a>13)
      Serial.write(0);
      break;
      }
    }
          Serial.write(228);
          Serial.write(2);
  }

void setup(){
  Serial.begin(SERIAL_SPEED);
   Serial.print("0");
  commandState=COMMAND_STATE_WAITING_COMMAND;
  for (byte a;a<14;a++)
  sens[a]=READ_DIG;
  for (byte a=14;a<22;a++)
  sens[a]=READ_ANA;
  }


byte bytearrayData[20];
byte byteDataTail=0;
byte command=0;

void loop(){
//     Serial.print("0");
    if(sensor.isFinished()){
    // Do something with the range...
    int a = sensor.getRange();
    if(a)
    old_sens = a;
    sensor.start();
   }
       if(sensor1.isFinished()){
    // Do something with the range...
    int a = sensor1.getRange();
    if(a)
    old_sens = a;
    sensor1.start();
   }
//   Serial.print("1");
   if( Serial.available() ){
      byte b = Serial.read();
      if(commandState== COMMAND_STATE_WAITING_COMMAND){
         switch(b){
            case ' ':{
               Serial.print("ROBBO-00006-00003-A-00000-00000-00000000000000000222");
               break;
            }
            case 'a':{
              command = b;
              commandState = COMMAND_STATE_WAITING_CRC;
              break;
            }
            case 'b':{//read dig anal sonic1 sonic2
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'c':{//set dig pwm
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'd':{//set dig pwm
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'h':{//note
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'i':{
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 's':{
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'u':{
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }     
            case 't':{
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }  
            case 'g':{
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }      
            case 'y':{
              command = b;
              commandState = COMMAND_STATE_WAITING_CRC;
              break;
            }                               
            case 'z':{
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
           }   
            }
      }
      else if(commandState==COMMAND_STATE_WAITING_DATA){
         bytearrayData[byteDataTail] = b;
         byteDataTail++;
         switch(command){
            case 'b': {
              if (byteDataTail > 0) {

              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'c': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'd': {
            if (byteDataTail > 0) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'h': {
            if (byteDataTail > 2) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'i': {
            if (byteDataTail > 0) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 's': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'u': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 't': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'g': {
            if (byteDataTail > 15) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }            
            case 'z': {
            if (byteDataTail > 0) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            } 
            }
      }
      else if(commandState==COMMAND_STATE_WAITING_CRC){

         if(b == '$'){
            switch(command){
              case 'a':{
                  printSensors();
                  break;
               }
              case 'b':{
                 byte a = bytearrayData[0]%128;
                 byte b = bitRead(bytearrayData[0],7);
                 pinMode(a,OUTPUT);
                 if(b)
                 digitalWrite(a,HIGH);
                 else
                 digitalWrite(a,LOW);
                 printSensors();
                 break;
              }
              case 'c':  {
                byte a = bytearrayData[0];
                byte b = bytearrayData[1];
                sens[a] = SERVO;
                pinMode(a, OUTPUT);
                analogWrite(a,b);
                //delay(100);
                printSensors();
                break;
              }
              case 'd':  { //pin output analog/dig _ number  ok!
                byte a = bitRead(bytearrayData[0],7);
                byte b = bytearrayData[0]%128;
                if(a==1) 
                sens[b]=READ_ANA;
                else
                sens[b]=READ_DIG;
                printSensors();
                break;
              }
              case 'h':{//note 
                sens[bytearrayData[2]]=SOUND;
                int note = bytearrayData[0]+(bytearrayData[1]>>4)*256;
                int dura = bytearrayData[1]%16;
                tone(bytearrayData[2],note,250*dura);
                 //tone(13,100,1000);
                  printSensors();
                  break;
               }
              case 's': { //servo ok!
                if(sens[bytearrayData[0]]!= SERVO)
                {
                  myservo.attach(bytearrayData[0]);
                  sens[bytearrayData[0]] = SERVO;
                }

                 myservo.write(500+bytearrayData[1]*10);
              //  myservo.deattach(bytearrayData[0]);
                printSensors();
                break;
              }
              case 'i': { //servo ok!
                IRrecv irrecv1(bytearrayData[0]);
                irrecv=irrecv1;
                irrecv.enableIRIn(); // Start the receiver
                sens[bytearrayData[0]]=PULT;
                printSensors();
                break;
              }
              case 'u': { 
                pin_for_sonic=bytearrayData[1];
                sens[bytearrayData[0]]=SONIC;
          //      HC_SR04(bytearrayData[1], bytearrayData[0], bytearrayData[0]-2);
          if(sens[2]==SONIC){
                sensor.begin();
                sensor.start();
          }
          if(sens[3]==SONIC){
                sensor1.begin();
                sensor1.start();
          }
                printSensors();
                break;
             }
              case 't': { 
                vlag_temp=bytearrayData[0];
                sens[bytearrayData[0]]=VLAGA;
                sens[bytearrayData[1]]=TEMP;
                printSensors();
                break;
             }
              case 'g': { 
//jopa          
                byte it=0;
                byte longg=0;
                char*str;
                char * ha = (char*) malloc(55);
                while((bytearrayData[it]!=127)&&(it<16))
                {

                    if(bytearrayData[it]>128)
                    {
                      switch(bytearrayData[it]-129)
                      {
                        case 0:str="\320\220";break;
                        case 1:str="\320\221";break;
                        case 2:str="\320\222";break;
                        case 3:str="\320\223";break;
                        case 4:str="\320\224";break;
                        case 5:str="\320\225";break;
                        case 6:str="\320\226";break;
                        case 7:str="\320\227";break;
                        case 8:str="\320\230";break;
                        case 9:str="\320\231";break;
                        case 10:str="\320\232";break;
                        case 11:str="\320\233";break;
                        case 12:str="\320\234";break;
                        case 13:str="\320\235";break;
                        case 14:str="\320\236";break;
                        case 15:str="\320\237";break;
                        case 16:str="\320\240";break;
                        case 17:str="\320\241";break;
                        case 18:str="\320\242";break;
                        case 19:str="\320\243";break;
                        case 20:str="\320\244";break;
                        case 21:str="\320\245";break;
                        case 22:str="\320\246";break;
                        case 23:str="\320\247";break;
                        case 24:str="\320\250";break;
                        case 25:str="\320\251";break;
                        case 26:str="\320\252";break;
                        case 27:str="\320\253";break;
                        case 28:str="\320\254";break;
                        case 29:str="\320\255";break;
                        case 30:str="\320\256";break;
                        case 31:str="\320\257";break;
                        case 32:str="\320\260";break;
                        case 33:str="\320\261";break;
                        case 34:str="\320\262";break;
                        case 35:str="\320\263";break;
                        case 36:str="\320\264";break;
                        case 37:str="\320\265";break;
                        case 38:str="\320\266";break;
                        case 39:str="\320\267";break;
                        case 40:str="\320\270";break;
                        case 41:str="\320\271";break;
                        case 42:str="\320\272";break;
                        case 43:str="\320\273";break;
                        case 44:str="\320\274";break;
                        case 45:str="\320\275";break;
                        case 46:str="\320\276";break;
                        case 47:str="\320\277";break;
                        case 48:str="\321\200";break;
                        case 49:str="\321\201";break;
                        case 50:str="\321\202";break;
                        case 51:str="\321\203";break;
                        case 52:str="\321\204";break;
                        case 53:str="\321\205";break;
                        case 54:str="\321\206";break;
                        case 55:str="\321\207";break;
                        case 56:str="\321\210";break;
                        case 57:str="\321\211";break;
                        case 58:str="\321\212";break;
                        case 59:str="\321\213";break;
                        case 60:str="\321\214";break;
                        case 61:str="\321\215";break;
                        case 62:str="\321\216";break;
                        case 63:str="\321\217";break;
                      }
                      strcat(ha,str);
                      longg++;
                    }
                    else
                    {
                     ha[longg]=bytearrayData[it];
                    }
                    longg++;
                                      it++;
                  }

                ha[longg]='\0';
   
                //ha = (char*) realloc (ha, -it * sizeof(byte));
                lcd.print(ha);
                free(ha);
                printSensors();
                break;
             }             
              case 'y': { 
              lcd.init();                      // initialize the lcd
              lcd.backlight();
                printSensors();
                break;
             }
              case 'z': { 
                curse=bytearrayData[0]%16;
                lcd.setCursor(bytearrayData[0]%16,bitRead(bytearrayData[0],7));
                //lcd.print("a");
                printSensors();
                break;
             }
           }
            byteDataTail=0;
         }
         commandState=COMMAND_STATE_WAITING_COMMAND;
      }
   }
     if (irrecv.decode(&results)) {
    Serial.println(results.value,DEC);
    if(results.value==16738455)
    {
      pult_rec=0;
    }
    if(results.value==16724175)
    {
      pult_rec=1;
    }
    if(results.value==16718055)
    {
      pult_rec=2;
    }
    if(results.value==16743045)
    {
      pult_rec=3;
    }
    if(results.value==16716015)
    {
      pult_rec=4;
    }
    if(results.value==16726215)
    {
      pult_rec=5;
    }
    if(results.value==16734885)
    {
      pult_rec=6;
    }
    if(results.value==16728765)
    {
      pult_rec=7;
    }
    if(results.value==16730805)
    {
      pult_rec=8;
    }
    if(results.value==16732845)
    {
      pult_rec=9;
    }  
        if(results.value==16753245)
    {
      pult_rec=10;
    }    
        if(results.value==16736925)
    {
      pult_rec=11;
    }    
        if(results.value==16769565)
    {
      pult_rec=12;
    }    
        if(results.value==16720605)
    {
      pult_rec=13;
    }    
        if(results.value==16712445)
    {
      pult_rec=14;
    }    
        if(results.value==16761405)
    {
      pult_rec=15;
    }    
    if(results.value==16769055)
    {
      pult_rec=16;
    }      
    if(results.value==16754775)
    {
      pult_rec=17;
    }      
    if(results.value==16748655)
    {
      pult_rec=18;
    }      
    if(results.value==16750695)
    {
      pult_rec=19;
    } 
    if(results.value==16756815)
    {
      pult_rec=20;
    } 
    irrecv.resume(); // Receive the next value
  }
}
//*/
/* 







*/
