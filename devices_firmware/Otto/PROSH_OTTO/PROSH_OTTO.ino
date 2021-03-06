#define FIRMWARE_VERSION "00002"
#include "HC_SR04.h"
#include <EEPROM.h>
#include "Arduino.h"
#include <Servo.h>

//#define SERIAL_SPEED 115200
#define SERIAL_SPEED 38400
#define SERIAL_ADDRESS 0
#define data 2
#define clock 4
#define FAST 0
#define VERY_FAST 5
#define NORMAL 10
#define SLOW 20
#define VERY_SLOW 30
#define ECHO_INT 0

#define PIN_BLUE_0     0
#define PIN_BLUE_1     1

#define PIN_R          3
#define PIN_G          5
#define PIN_B          6

#define PIN_SONIC_ECHO 2
#define PIN_SONIC_TRIG 9

#define PIN_MATRIX_CS  10
#define PIN_MATRIX_CLK 11
#define PIN_MATRIX_DIN 12

#define PIN_BUZZER     13

#define PIN_LEFT_FOOT   14
#define PIN_RIGHT_FOOT  15
#define PIN_LEFT_LEG  16
#define PIN_RIGHT_LEG 17
#define PIN_LEFT_HAND  6
#define PIN_RIGHT_HAND 7

#define PIN_MICROPHONE  A7

#define TRIM_LEFT_LEG   0
#define TRIM_RIGHT_LEG  0
#define TRIM_LEFT_FOOT  0
#define TRIM_RIGHT_FOOT 0




char chararrSerialRaw[50];
char chararrModel[21];
char chararrVersion[21];
char chararrPart[21];
char chararrSerial[21];
int MODEL_ID;
unsigned char i;

unsigned char j;

int notes[] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047};


byte commandState;
const byte COMMAND_STATE_WAITING_COMMAND=0;
const byte COMMAND_STATE_WAITING_COMMAND_TYPE=1;
const byte COMMAND_STATE_WAITING_DATA=2;
const byte COMMAND_STATE_WAITING_CRC=3;
const byte COMMAND_STATE_EXECUTING=4;




unsigned long startMillis= millis();  // Start of sample window
///////////////////////////////////////////////////////////////////////MATRIX
unsigned char now_led[8]={0,0,0,0,0,0,0,0};

unsigned char disp1[50][8] = {
0x0,  0x7C,  0xFE,  0x82,  0x82,  0xFE,  0x7C,  0x0, //0
0x0,  0x0,  0x2,  0xFE,  0xFE,  0x42,  0x0,  0x0, //1
0x0,  0x62,  0xF2,  0x92,  0x9A,  0xCE,  0x46,  0x0,//2
0x0,  0x6C,  0xFE,  0x92,  0x92,  0xC6,  0x44,  0x0,//3
0x8,  0xFE,  0xFE,  0xC8,  0x68,  0x38,  0x18,  0x0,//4
0x0,  0x9C,  0xBE,  0xA2,  0xA2,  0xE6,  0xE4,  0x0,//5
  0x30, 0x48, 0x44, 0x22, 0x22, 0x44, 0x48, 0x30, // рисунок сердца
  0x00,0x10,0x18,0x14,0x14,0x18,0x10,0x00,         //узкая улыбка
  0x10,0x18,0x14,0x14,0x14,0x14,0x18,0x10,         //широкая улыбка
  0x00,0x10,0x28,0x44,0x44,0x28,0x10,0x00,         //буква 0
  0x0,  0x7C,  0xFE,  0x82,  0x82,  0xFE,  0x7C,  0x0, //0
0x0,  0x0,  0x2,  0xFE,  0xFE,  0x42,  0x0,  0x0, //1
0x0,  0x62,  0xF2,  0x92,  0x9A,  0xCE,  0x46,  0x0,//2
0x0,  0x6C,  0xFE,  0x92,  0x92,  0xC6,  0x44,  0x0,//3
0x8,  0xFE,  0xFE,  0xC8,  0x68,  0x38,  0x18,  0x0,//4
0x0,  0x9C,  0xBE,  0xA2,  0xA2,  0xE6,  0xE4,  0x0,//5
  0x30, 0x48, 0x44, 0x22, 0x22, 0x44, 0x48, 0x30, // рисунок сердца
  0x00,0x10,0x18,0x14,0x14,0x18,0x10,0x00,         //узкая улыбка
  0x10,0x18,0x14,0x14,0x14,0x14,0x18,0x10,         //широкая улыбка
  0x00,0x10,0x28,0x44,0x44,0x28,0x10,0x00,         //буква 0
  0x0,  0x7C,  0xFE,  0x82,  0x82,  0xFE,  0x7C,  0x0, //0
0x0,  0x0,  0x2,  0xFE,  0xFE,  0x42,  0x0,  0x0, //1
0x0,  0x62,  0xF2,  0x92,  0x9A,  0xCE,  0x46,  0x0,//2
0x0,  0x6C,  0xFE,  0x92,  0x92,  0xC6,  0x44,  0x0,//3
0x8,  0xFE,  0xFE,  0xC8,  0x68,  0x38,  0x18,  0x0,//4
0x0,  0x9C,  0xBE,  0xA2,  0xA2,  0xE6,  0xE4,  0x0,//5
  0x30, 0x48, 0x44, 0x22, 0x22, 0x44, 0x48, 0x30, // рисунок сердца
  0x00,0x10,0x18,0x14,0x14,0x18,0x10,0x00,         //узкая улыбка
  0x10,0x18,0x14,0x14,0x14,0x14,0x18,0x10,         //широкая улыбка
  0x00,0x10,0x28,0x44,0x44,0x28,0x10,0x00,         //буква 0
  0x0,  0x0,  0x2,  0xFE,  0xFE,  0x42,  0x0,  0x0, //1
0x0,  0x62,  0xF2,  0x92,  0x9A,  0xCE,  0x46,  0x0,//2
0x0,  0x6C,  0xFE,  0x92,  0x92,  0xC6,  0x44,  0x0,//3
0x8,  0xFE,  0xFE,  0xC8,  0x68,  0x38,  0x18,  0x0,//4
0x0,  0x9C,  0xBE,  0xA2,  0xA2,  0xE6,  0xE4,  0x0,//5
  0x30, 0x48, 0x44, 0x22, 0x22, 0x44, 0x48, 0x30, // рисунок сердца
  0x00,0x10,0x18,0x14,0x14,0x18,0x10,0x00,         //узкая улыбка
  0x10,0x18,0x14,0x14,0x14,0x14,0x18,0x10,         //широкая улыбка
  0x00,0x10,0x28,0x44,0x44,0x28,0x10,0x00,         //буква 0
};
void Write_Max7219_byte(unsigned char DATA) {
  unsigned char i;
  digitalWrite(PIN_MATRIX_CS, LOW);
  for (i = 8; i >= 1; i--) {
    digitalWrite(PIN_MATRIX_CLK, LOW);
    digitalWrite(PIN_MATRIX_DIN, DATA & 0x80);
    DATA = DATA << 1;
    digitalWrite(PIN_MATRIX_CLK, HIGH);
  }
}
void Write_Max7219(unsigned char address, unsigned char dat) {
  digitalWrite(PIN_MATRIX_CS, LOW);
  Write_Max7219_byte(address);
  Write_Max7219_byte(dat);
  digitalWrite(PIN_MATRIX_CS, HIGH);
}

void Init_MAX7219(void) {
  Write_Max7219(0x09, 0x00);
  Write_Max7219(0x0a, 0x03);
  Write_Max7219(0x0b, 0x07);
  Write_Max7219(0x0c, 0x01);
  Write_Max7219(0x0f, 0x00);
}
void cleaR()
{
    for (i = 1; i < 9; i++)
    Write_Max7219(i, 0x00);
}
void updatE()
{
    for (i = 1; i < 9; i++)
    Write_Max7219(i, now_led[i-1]);
}
////////////////////////////////////////////////////ENDMATRIX
////////////////////////////////////////////////////SERIALNUMBER
void parseSerialNumber(){
    EEPROM.get(SERIAL_ADDRESS, chararrSerialRaw);
    int iPointer = 0;
    while(chararrSerialRaw[iPointer] != '-'){
      iPointer++;
    }
    iPointer++;
    int iModelOffset = 0;
    while(chararrSerialRaw[iPointer] != '-'){
      chararrModel[iModelOffset] = chararrSerialRaw[iPointer];
      iModelOffset++;
      iPointer++;
    }
    iPointer++;

    int iVersionOffset = 0;
    while(chararrSerialRaw[iPointer] != '-'){
      chararrVersion[iVersionOffset] = chararrSerialRaw[iPointer];
      iVersionOffset++;
      iPointer++;
    }

    iPointer++;

    int iPartOffset = 0;
    while(chararrSerialRaw[iPointer] != '-'){
      chararrPart[iPartOffset] = chararrSerialRaw[iPointer];
      iPartOffset++;
      iPointer++;
    }

    iPointer++;


    int iSerialOffset = 0;
    while(chararrSerialRaw[iPointer] != 0 && (chararrSerialRaw[iPointer] >= '0' && chararrSerialRaw[iPointer] <='9')){
      chararrSerial[iSerialOffset] = chararrSerialRaw[iPointer];
      iSerialOffset++;
      iPointer++;
    }


    if(strcmp(chararrModel, "R") == 0
       && strcmp(chararrVersion, "1") == 0
       && (strcmp(chararrPart, "1") == 0 || strcmp(chararrPart, "2") == 0 || strcmp(chararrPart, "3") == 0 || strcmp(chararrPart, "4") == 0)){

       MODEL_ID=0;
    }
    else if(strcmp(chararrModel, "L") == 0
       && strcmp(chararrVersion, "1") == 0
       && strcmp(chararrPart, "1") == 0){

       MODEL_ID=1;
    }
    else if(strcmp(chararrModel, "L") == 0
       && strcmp(chararrVersion, "3") == 0
       && (strcmp(chararrPart, "1") == 0 || strcmp(chararrPart, "2") == 0 || strcmp(chararrPart, "3") == 0)){

       MODEL_ID=2;
    }


    else if(strcmp(chararrModel, "O") == 0              // O= Otto
         && strcmp(chararrVersion, "1") == 0           // s zavoda
        && (strcmp(chararrPart, "5") == 0 )){          // s zavoda

       MODEL_ID=5;                                     //MODEL_ID for OTTO
    }




    else{
       MODEL_ID=9999;
    }
}
///////////////////////////////////////////////////////ENDSERIALNUMBER
///////////////////////////////////////////////////////ULTRASONIC
int dist=0;
HC_SR04 sensor(PIN_SONIC_TRIG, PIN_SONIC_ECHO, ECHO_INT);
int getDist(){
  if(sensor.isFinished()){
    // Do something with the range...
    dist = sensor.getRange();
    //Serial.println("cm");
    sensor.start();
  }
  return dist;
}
////////////////////////////////////////////////////ENDULTRASONIC]
///////////////////////////////////////////////////HANDS
Servo HandL;
Servo HandR;
///////////////////////////////////////////////////ENDHANDS
void printSensors(){
    Serial.write('#');
    Serial.write(getDist());
    if(analogRead(PIN_MICROPHONE)>100)
    Serial.write(1);
    else
    Serial.write(0);
    }
Servo myservo [6];
int pos[6];
int need_pos[6];
unsigned long times[6];
void setup(){
  myservo[0].attach(14);
  myservo[1].attach(15);
  myservo[2].attach(16);
  myservo[3].attach(17);
  myservo[4].attach(7);
  myservo[5].attach(8);
    for(times[0]=0;times[0]<6;times[0]++)
  {
  myservo[times[0]].write(90);
  need_pos[times[0]]=90;
  pos[times[0]]=90;
  }
  parseSerialNumber();
  sensor.begin();
  Serial.begin(SERIAL_SPEED);
  EEPROM.get(0, chararrSerialRaw);
 // Serial.print("robot id is: ");
  //Serial.println(chararrSerialRaw);
  
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  
  pinMode(PIN_LEFT_LEG, OUTPUT); 
  pinMode(PIN_RIGHT_LEG, OUTPUT); 
  pinMode(PIN_LEFT_FOOT, OUTPUT); 
  pinMode(PIN_RIGHT_FOOT, OUTPUT);
  
  pinMode(PIN_MATRIX_CLK, OUTPUT);
  pinMode(PIN_MATRIX_CS, OUTPUT);
  pinMode(PIN_MATRIX_DIN, OUTPUT); 
  
  pinMode(PIN_MICROPHONE , INPUT ); 
  commandState=COMMAND_STATE_WAITING_COMMAND;
    

  delay(50);
  Init_MAX7219();
  cleaR();
  while(!Serial) continue;
  sensor.start();
}


byte bytearrayData[20];
byte byteDataTail=0;
byte command=0;
byte diod=1;
byte zz=0;
byte turn_speed=0;
void loop(){
  for(zz=0;zz<6;zz++)
  { 
    if((need_pos[zz]!=pos[zz])&&(millis()>times[zz]))
    {
      if(need_pos[zz]>pos[zz])
      pos[zz]++;
      else
      pos[zz]--;
      times[zz]=millis()+turn_speed;
    }
    myservo[zz].write(pos[zz]);
  }
  //Serial.println(getDist());
 // printSensors();
   //  HandL.write(90);
   //  delay(100);
     // HandR.write(0);
//delay(100);
    //Otto.jump(2,300);
    //for (k=0;k<5;k++)
    //Otto.updown(1,1000,20);//float steps=1, int T=1000, int h = 20
     //Otto.jitter(5,500, 40);
    //HandR.write(180);
    //HandL.write(180);
    ///analogWrite(3,180);
    //delay(100);
 //   Otto.crusaito(10);
//    Otto.flapping(10);
  //  Otto.walk(10,125,FORWARD);

    
    //TODO Otto.tiptoeSwing();//float steps=1, int T=900, int h=20
    //for (k=0;k<5;k++)
    //Otto.jitter(5,100, 40);//float steps=1, int T=500, int h=20
    // Otto.ascendingTurn();//float steps=1, int T=900, int h=20
    // for (k=0;k<5;k++)
    //Otto.shakeLeg ();//int steps=1, int T = 2000, int dir=RIGHT);
    //forw/back
    //TODO TODO Otto.crusaito();//float steps=1, int T=900, int h=20, int dir=FORWARD
    //TODO TODO Otto.flapping(1,1000,);//float steps=1, int T=1000, int h=20, int dir=FORWARD
    //TODO TODO Otto.walk(10,125,FORWARD);
    //left/right
    //TODO TODO Otto.turn();//float steps=4, int T=2000, int dir = LEFT
    //TODO TODO
    //for (k=0;k<5;k++)
    //Otto.turn(3,1000);
    // Otto.bend (3,1000);//int steps=1, int T=1400, int dir=LEFT
    //TODO TODO Otto.moonwalker();//float steps=1, int T=900, int h=20, int dir=LEFT
   if( Serial.available() ){
      byte b = Serial.read();
      //Serial.println(";");Serial.println(b);Serial.println(byteDataTail);Serial.println(";");
      if(commandState== COMMAND_STATE_WAITING_COMMAND){
         switch(b){
            case ' ':{
               Serial.print(F("ROBBO-"));
               if(MODEL_ID < 10000){
                  Serial.write('0');
               }
               if(MODEL_ID < 1000){
                  Serial.write('0');
               }
               if(MODEL_ID < 100){
                  Serial.write('0');
               }
               if(MODEL_ID < 10){
                  Serial.write('0');
               }
               Serial.print(MODEL_ID);
               Serial.write('-');
               Serial.print(F(FIRMWARE_VERSION));
               Serial.write('-');
               Serial.print(chararrModel);
               Serial.print('-');
               for(int f = strlen(chararrVersion); f < 5; f++){
                  Serial.write('0');
               }
               Serial.print(chararrVersion);
               Serial.print('-');
               for(int f = strlen(chararrPart); f < 5; f++){
                  Serial.write('0');
               }
               Serial.print(chararrPart);
               Serial.print('-');
               for(int f = strlen(chararrSerial); f < 20; f++){
                  Serial.write('0');
               }
               Serial.print(chararrSerial);

               break;
            }
            case 'a':{
              command = b;
              commandState = COMMAND_STATE_WAITING_CRC;
              break;
            }
            case 'b':{//staying
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'c':{//moving forv/back
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'd':{//1 led change
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'e':{//set your picture
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'f':{//set default picture
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }            
            case 'g':{//nose colour
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'h':{//play note
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'i':{//set hands
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'j':{//if needed
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
 
            case 's':{//serv state
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
           case 'y':{//serv state
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
            case 'z':{//serv state
              command = b;
              commandState = COMMAND_STATE_WAITING_DATA;
              break;
            }
         }
      }
      else if(commandState==COMMAND_STATE_WAITING_DATA){
         bytearrayData[byteDataTail] = b;
         byteDataTail++;
        // Serial.println(b);
         switch(command){
            case 'b': {
              if (byteDataTail > 3) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'c': {
            if (byteDataTail > 3) {
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
            case 'e': {
            if (byteDataTail > 7) {
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
            if (byteDataTail > 2) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'h': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'i': {
            if (byteDataTail > 1) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'j': {
            if (byteDataTail > 3) {
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
            case 'y': {
            if (byteDataTail > 2) {
              commandState = COMMAND_STATE_WAITING_CRC;
            }
            break;
            }
            case 'z': {
            if (byteDataTail > 4) {
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
         /*     case 'b':{
                int tim = 125 * bytearrayData[0];
                float steps = float(bytearrayData[1]);
                byte hshka= 2*(bytearrayData[2] >> 4);
                int chta = bytearrayData[2]%8;
                switch (chta) {
                  case 0:
                  {
                    Otto.ascendingTurn(steps,tim,hshka);
                    break;
                  }
                  case 1:
                  {
                    Otto.jitter(steps,tim,hshka);
                    break;
                  }
                  case 2:
                  {
                    Otto.tiptoeSwing(steps,tim,hshka);
                    break;
                  }
                  case 3:
                  {
                    Otto.swing(steps,tim,hshka);
                    break;
                  }
                  case 4:
                  {
                    Otto.updown(steps,tim,hshka);
                    break;
                  }
                  case 5:
                  {

                    break;
                  }
                  case 6:
                  {

                    break;
                  }
                  case 7:
                  {

                    break;
                  }

                }
                printSensors();
                break;
              }*//*
              case 'c':  {
                int tim = 125 * bytearrayData[0];
                float steps = float(bytearrayData[1]);
                int dir = bytearrayData[2]%16 >> 3;
                if(!dir)
                dir--;
                byte hshka= 2*(bytearrayData[2] >> 4);
                int chta = bytearrayData[2]%8;
                switch (chta) {
                  case 0:
                  {
                    Otto.jump(steps,tim);
                    break;
                  }
                  case 1:
                  {
                    Otto.shakeLeg (steps,tim,dir);
                    break;
                  }
                  case 2:
                  {
                    Otto.bend (steps,tim,dir);
                    break;
                  }
                  case 3:
                  {
                     Otto.walk(steps,tim,dir);
                    break;
                  }
                  case 4:
                  {
                    Otto.turn(steps,tim,dir);
                    break;
                  }
                  case 5:
                  {
                    Otto.crusaito(steps,tim,hshka,dir);
                    break;
                  }
                  case 6:
                  {
                    Otto.flapping(steps,tim,hshka,dir);
                    break;
                  }
                  case 7:
                  {
                    Otto.moonwalker(steps,tim,hshka,dir);
                    break;
                  }

                }
                printSensors();
                break;
              }    */        
              case 'd':{//1 led
              //  Serial.println(bytearrayData[0]);
                byte vkl = bitRead(bytearrayData[0],0);
                byte str = bitRead(bytearrayData[0],1)*1+bitRead(bytearrayData[0],2)*2+bitRead(bytearrayData[0],3)*4;
                byte stolb = bitRead(bytearrayData[0],4)*1+bitRead(bytearrayData[0],5)*2+bitRead(bytearrayData[0],6)*4;
               // Serial.println(vkl);
                //Serial.println(str);
                //Serial.println(stolb);
                bitWrite(now_led[7-str],stolb,vkl);
                updatE();
                  printSensors();
                  break;
               }              
              case 'e':{//set your pic
                //Serial.println("e");
                  for(i=0;i<8;i++)
                  now_led[i]=bytearrayData[i];   
                  updatE();
                  printSensors();
                  break;
               }
              case 'f':{//set default pic
                //Serial.println(bytearrayData[0]);
               // *now_led=*disp1[bytearrayData[0]];
               for(i=0;i<8;i++)
               now_led[i]=disp1[bytearrayData[0]][i];
                     updatE();
                  printSensors();
                  break;
               }
              case 'g':{//nose
               // Serial.println(bytearrayData[0]);Serial.print(" "); Serial.print(bitRead(bytearrayData[0],0)); Serial.print(" "); Serial.print(bitRead(bytearrayData[0],1));  Serial.print(" "); Serial.println(bitRead(bytearrayData[0],2));
                analogWrite(PIN_R, bytearrayData[0]);//rgb
                analogWrite(PIN_G, bytearrayData[1]);//rgb
                analogWrite(PIN_B, bytearrayData[2]);//rgb
                //analogWrite(A2, 0);//rgb
                  printSensors();
                  break;
               }
              case 'h':{//note
                int note = bytearrayData[0]+(bytearrayData[1]>>4)*16;
                int dura = bytearrayData[1]%16;
                tone(13,note,250*dura);
                  printSensors();
                  break;
               }
              case 'i':{//hands
                  HandL.write(bytearrayData[0]);
                  HandR.write(bytearrayData[1]);
                  printSensors();
                  break;
               }
              case 'j':{//reserved
                  printSensors();
                  break;
               }
              case 's': {
                byte serv_num = bytearrayData[0]%8;//last 3
                turn_speed= (bytearrayData[0]>>3)%8*5;
                need_pos[serv_num]=bytearrayData[1];
                //a[0]=bytearrayData[1];
                //a[1]=bytearrayData[2];
               // a[2]=bytearrayData[3];
              //  a[3]=bytearrayData[4];
             //   Otto._moveServos(tim,a);
                printSensors();
                break;
              }
              case 'z': {
                turn_speed= bytearrayData[4]%8*5;
                need_pos[0]=bytearrayData[0];
                need_pos[1]=bytearrayData[1];
                need_pos[2]=bytearrayData[2];
                need_pos[3]=bytearrayData[3];
                printSensors();
                break;
              }
              case 'y': {
                turn_speed= bytearrayData[2]%8*5;
                need_pos[4]=bytearrayData[0];
                need_pos[5]=bytearrayData[1];
                printSensors();
                break;
              }
            }
         }
         commandState=COMMAND_STATE_WAITING_COMMAND;
         byteDataTail=0;
      }
   }
}
//*/
