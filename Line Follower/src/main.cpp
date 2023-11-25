#include <Arduino.h>            //USED LIBRARIES
#include<QTRSensors.h>
#include<Wire.h>                                                        
#include<Adafruit_SSD1306.h>
#include<Adafruit_GFX.h>

QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];

int error;
int lastError;
int motorSpeed;
int btn_count = 0;
int thrdshld = 1500;
unsigned int i=0;

String path;

enum junctionType { DeadEnd, CrossJunction, TJunction, LeftTJunction, LeftJunction, RightTJunction, RightJunction, Destination};    //REQUIRED TO SWITCH STRING DATA TYPE
junctionType direction;

#define m1_in1 18            //DEFINED MOTOR PINS
#define m1_in2 5
#define m2_in1 16
#define m2_in2 17

#define strt_btn 23

#define Kp 0.1
#define Kd 0.1

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void mdrive(int m1_speed, int m2_speed){            //CONTROLLING SPEED AND DIRECTION OF MOTORS

    if(m1_speed>0){
        if(m1_speed>255){
            m1_speed = 255;
        }
        digitalWrite(m1_in1, HIGH);
        analogWrite(m1_in2, 255-m1_speed);
    }
    else{
        if(m1_speed<-255){
            m1_speed = -255;
        }
        digitalWrite(m1_in1, LOW);
        analogWrite(m1_in2, abs(m1_speed));
    }


    if(m2_speed>0){
        if(m2_speed>255){
            m2_speed = 255;
        }
        digitalWrite(m2_in1, HIGH);
        analogWrite(m2_in2, 255-m2_speed);
    }
    else{
        if(m2_speed<-255){
            m2_speed = -255;
        }
        digitalWrite(m2_in1, LOW);
        analogWrite(m2_in2, abs(m2_speed));
    }

}

void followLine(){                                                          //LINE FOLLOWING 
    uint16_t position = qtr.readLineBlack(sensorValues);
    error = position-3500;
    motorSpeed = Kp * error + Kd * (error - lastError);
    lastError = error;

    int left_speed = 155 + motorSpeed;
    int rightSpeed = 155 - motorSpeed;

    mdrive(left_speed,rightSpeed);
}

void front(){                                                                               
    mdrive(128,128);
    delay(380);
    mdrive(-50,-50);
    delay(50);
    mdrive(0,0);
    delay(100);
}

void back(){
    mdrive(-200,200);
    delay(800);
    mdrive(50,-50);
    delay(50);
    mdrive(0,0);
    delay(100);
}

void right(){
    mdrive(128,-128);
    delay(540);
    mdrive(-50,50);
    delay(50);
    mdrive(0,0);
    delay(100);
}

void left(){
    mdrive(-128,128);
    delay(540);
    mdrive(50,-50);
    delay(50);
    mdrive(0,0);
    delay(100);
}

void halt(){

    mdrive(100,100);
    delay(150);
    mdrive(-100,-100);
    delay(50);
    mdrive(0,0);
    delay(50);
}

void checkJunction(){            //THIS FUNCION GIVES THE TYPE OF THE JUNCTION

    qtr.read(sensorValues);
    if((sensorValues[0]<thrdshld)&&((sensorValues[3]<thrdshld)&&(sensorValues[4]<thrdshld))&&(sensorValues[6]<thrdshld)){

        direction = DeadEnd; 
    }

    else if(((sensorValues[0]>thrdshld)||(sensorValues[1]>thrdshld))&&((sensorValues[6]>thrdshld)||(sensorValues[7]>thrdshld))){

        front();
        qtr.read(sensorValues);

        if(((sensorValues[0]>thrdshld)||(sensorValues[1]>thrdshld))&&((sensorValues[6]>thrdshld)||(sensorValues[7]>thrdshld))){

            direction = Destination; 

        }
        else if((sensorValues[3]>thrdshld)||(sensorValues[4]>thrdshld)){

            direction = CrossJunction;

        }
        else{

            direction = TJunction;

        }

    }

    else if(((sensorValues[0]>thrdshld)||(sensorValues[1]>thrdshld))&&((sensorValues[6]<thrdshld)||(sensorValues[7]<thrdshld))){

        front();
        qtr.read(sensorValues);

        if((sensorValues[3]>thrdshld)||(sensorValues[4]>thrdshld)){

            direction = LeftTJunction;  
        }
        else{

            direction = LeftJunction;

        }


    }

    else if(((sensorValues[0]<thrdshld)||(sensorValues[1]<thrdshld))&&((sensorValues[6]>thrdshld)||(sensorValues[7]>thrdshld))){

        front();
        qtr.read(sensorValues);

        if((sensorValues[3]>thrdshld)||(sensorValues[4]>thrdshld)){

            direction = RightTJunction; 
        }
        else{

            direction = RightJunction; 


        }


    }


}


void leftHandRule(){                    //MOVE THE ROBOT ACCORDING TO THE JUNCION TYPE USING LEFT HAND RULE

    qtr.read(sensorValues);

    while(((sensorValues[0]<thrdshld)||(sensorValues[1]<thrdshld))&&((sensorValues[3]>thrdshld)||(sensorValues[4]>thrdshld))&&((sensorValues[6]<thrdshld)||(sensorValues[7]<thrdshld))){
        
        followLine();
        qtr.read(sensorValues);
    }

    halt();
    checkJunction();

    switch(direction){

        case DeadEnd:
        back();
        path += 'B';
        break;

        case CrossJunction:
        left();
        path += 'L';
        break;

        case TJunction:
        left();
        path += 'L';
        break;

        case LeftTJunction:
        left();
        path += 'L';
        break;

        case LeftJunction:
        left();
        break;

        case RightTJunction:
        path += 'S';
        break;

        case RightJunction:
        right();
        break;

        case Destination:
        halt();
        while(digitalRead(strt_btn)==HIGH){

            display.clearDisplay();
            display.display();
            display.setCursor(0,0);
            display.print(path);
            display.display();     

        }
        break;

    }



}

String ShortPath(){                //FINDS THE SHORTEST PATH 

    path.replace("LBL", "S"); 
    path.replace("LBS", "R");
    path.replace("RBL", "B");
    path.replace("SBS", "B");
    path.replace("SBL", "R");
    path.replace("LBR", "B");
    return path;
}

void turn(){
    
    if(path.charAt(i)=='L'){

        left();
        i++;

    }
    else if(path.charAt(i)=='R'){

        right();
        i++;

    }    
    else if(path.charAt(i)=='B'){

        back();
        i++;

    }
    else if(path.charAt(i)=='S'){

        i++;

    }

}

void followPath(){                    //FOLLOW THE SHORTED PATH

    qtr.read(sensorValues);

    while(((sensorValues[0]<thrdshld)||(sensorValues[1]<thrdshld))&&((sensorValues[3]>thrdshld)||(sensorValues[4]>thrdshld))&&((sensorValues[6]<thrdshld)||(sensorValues[7]<thrdshld))){
        
        followLine();
        qtr.read(sensorValues);
    }

    halt();
    checkJunction();
    
    if(direction==LeftJunction){
        left();
    }
    else if(direction==RightJunction){
        right();
    }
    else if(direction==Destination){
        while(digitalRead(strt_btn)==HIGH){
            mdrive(0,0);
            display.clearDisplay();
            display.display();
            display.setCursor(0,0);
            display.print("FINISH");
            display.display();
        }
    }
    else{
        turn();
    }


}


void setup() {

    Serial.begin(115200);

    pinMode(strt_btn, INPUT_PULLUP);
    pinMode(m1_in1,OUTPUT);
    pinMode(m1_in2,OUTPUT);
    pinMode(m2_in1,OUTPUT);
    pinMode(m2_in2,OUTPUT);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.print("Callibrating...");
    display.display();
    delay(2000);

    qtr.setTypeAnalog();
    qtr.setSensorPins((const uint8_t[]){34, 32, 33, 25, 26, 27, 14, 13}, SensorCount);

    for (uint16_t i = 0; i < 400; i++) {
    qtr.calibrate();
    }

    display.clearDisplay();
    display.display();

    while(digitalRead(strt_btn)==HIGH){                 //press start button
    display.setCursor(0,0);
    display.print("Press start button");
    display.display();
    delay(10);
    display.clearDisplay();
    }
    display.clearDisplay();
    display.display();

}

void loop(){
 
   if(digitalRead(strt_btn)==LOW){
        btn_count += 1;
        delay(500);
    }

    while(btn_count==1){

        leftHandRule();
        if(digitalRead(strt_btn)==LOW){
            btn_count += 1;
            delay(500);
        }

    }

    while(btn_count==2){

        ShortPath();
        ShortPath();
        followPath();

    }


}
