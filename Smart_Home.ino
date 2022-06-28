#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <Servo.h> // servo library 
#include <EEPROM.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;
SoftwareSerial Blue = SoftwareSerial(0, 1);
Servo myservo; // servo name
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

byte ledBrightness = 255;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;

int BUZZ = 10;
String data = " ";
int opened = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(0, INPUT);  //RX
  pinMode(1, OUTPUT); //TX
  pinMode(6, OUTPUT); //Led
  pinMode(7, OUTPUT); //Led
  pinMode(8, OUTPUT); //Led
  pinMode(9, OUTPUT); //servo
  pinMode(10, OUTPUT); //buzzer
  pinMode(13, OUTPUT); //Led
  pinMode(14, OUTPUT); //temp sensor
  pinMode(18, OUTPUT); //heart rate sensor SCL
  pinMode(19, OUTPUT);//heart rate sensor SDA
  myservo.attach(9); //attaches the servo to digital pin 9
  myservo.write(0);

  Wire.begin();
  Serial.begin(115200); //heart rate sensor
  Blue.begin(9600); //Set the baud rate as 9600
  lcd.begin(16, 2); //initialize the lcd for 16 chars 2 lines
  lcd.clear();
  noTone(10);

}

void loop() {
  // put your main code here, to run repeatedly:
  Blue.listen();
  while (Blue.available() > 0)
  {
    data = Blue.readString();
    //Door Opening
    if (data == "O") {
      if (opened == 0) {
        digitalWrite(6, LOW);
        digitalWrite(7, LOW);
        digitalWrite(8, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Right Password");
        delay(1000);
        lcd.clear();
        for (int x = 0; x <= 180; x += 1) // Rotates the servo to the unlocked position
        {
          myservo.write(x);
          delay(10);
        }
        opened = 1;
        delay(1000);
        lcd.setCursor(0, 0);
        lcd.print("Opened using");
        lcd.setCursor(0, 1);
        lcd.print("the App");
        delay(1000);
        lcd.clear();
      }
    }
    else if (data == "W")
    {
      if (digitalRead(6) == LOW && digitalRead(7) == LOW && digitalRead(8) == LOW)
      {
        digitalWrite(6, HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong Password");
        delay(1000);
        lcd.clear();
      }
      else if (digitalRead(6) == HIGH && digitalRead(7) == LOW && digitalRead(8) == LOW)
      {
        digitalWrite(7, HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong Password");
        delay(1000);
        lcd.clear();
      }
      else if (digitalRead(6) == HIGH && digitalRead(7) == HIGH && digitalRead(8) == LOW)
      {
        digitalWrite(8, HIGH);
        lcd.print("Wrong Password");
        delay(1000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("The Door is Locked");
        digitalWrite(6, LOW);
        digitalWrite(7, LOW);
        digitalWrite(8, LOW);
        delay(3000);
        lcd.clear();
      }
    }
    else if (data == "C")
    {
      if (opened == 1) {
        for (int x = 180; x >= 0; x -= 1) // Rotates the servo to the unlocked position
        {
          myservo.write(x);
          delay(10);
        }
        delay(2000);
        lcd.setCursor(0, 0);
        lcd.print("Closed");
        delay(1000);
        lcd.clear();
        opened = 0;
      }
    }
    //Lightning
    else if (data == "N")
    {
      digitalWrite(13, HIGH);
    }
    else if (data == "F")
    {
      digitalWrite(13, LOW);
    }

    //Temperature
    else if (data == "T")
    {
      //int temp = analogRead(A0);
      //Convert digital data into analog by multiplying by 5000 and dividing by 1024
      //float voltage = temp * (5.0 / 1024.0);
      // Convert the voltage into the temperature in degree Celsius:
      //float tempC = voltage * 100;
      int tempC = 51;
      delay(1000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temperature= ");
      lcd.print(tempC);
      lcd.print(" C");
      delay(500);
      if (tempC > 50)
        {
        tone(BUZZ, 450);
        delay(500);
        noTone(BUZZ);
        delay(500);
        tone(BUZZ, 450);
        delay(500);
        noTone(BUZZ);
        delay(500);
        tone(BUZZ, 450);
        delay(500);
        noTone(BUZZ);
        for(int x = 0; x <= 180; x += 1)
        {
          myservo.write(x);
          delay(10);
        }
        opened = 1;
        }
      delay(1000);
      lcd.clear();

    }
    //Heart Rate
    else if (data == "H")
    {
      if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("MAX30102 was not found. Please check wiring/power. ");
        while (1);
      }
      particleSensor.setup(); //Configure sensor with default settings
      particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
      particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

      Serial.println("Initializing...");
      while(Serial.available() > 0) {
        
        long irValue = particleSensor.getIR();//Reading the IR value it will permit us to know if there's a finger on the sensor or not
                                           //Also detecting a heartbeat
        if(irValue > 7000){   //If a finger is detected 
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("BPM = ");
          lcd.print(beatAvg);
          
          if (checkForBeat(irValue) == true) {   //If a heart beat is detected
            //We sensed a beat!
            tone(10,1000);                                        //And tone the buzzer for a 100ms you can reduce it it will be better
            delay(100);
            noTone(10); 
            long delta = millis() - lastBeat; //Measure duration between two beats
            lastBeat = millis();
            beatsPerMinute = 60 / (delta / 1000.0); //Calculating the BPM
            if (beatsPerMinute < 255 && beatsPerMinute > 20) { //To calculate the average we strore some values (4) then do some math to calculate the average
              rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
              rateSpot %= RATE_SIZE; //Wrap variable
              //Take average of readings
              beatAvg = 0;
              for (byte x = 0 ; x < RATE_SIZE ; x++)
              {
                beatAvg += rates[x];
              }
              beatAvg /= RATE_SIZE;
            }
          }
        }
        
        //delay(500);
        Serial.print("IR = ");
        Serial.print(irValue);
        Serial.print(", BPM = ");
        Serial.print(beatsPerMinute);
        Serial.print(", Avg BPM = ");
        Serial.print(beatAvg);
        
        if (irValue < 7000){       //If no finger is detected it inform the user and put the average BPM to 0 or it will be stored for the next measure
          beatAvg=0;    
          noTone(10);
          lcd.clear();
          lcd.setCursor(0,0);
        }
       }
       //Serial.println();
    }
  }
}
