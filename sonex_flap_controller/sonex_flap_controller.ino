/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x64 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DIRECTION_RETRACT -1
#define DIRECTION_STOP 0
#define DIRECTION_EXTEND 1

// List of flap notches
int ANGLES[] = { 0, 10, 20 }; //, 30, 40 };
int NOTCH_COUNT = sizeof(ANGLES) / sizeof(ANGLES[0]);
int CURRENT_NOTCH = 0;
int TARGET_NOTCH = 0;

int TITLE_MARGIN = 8;
int TITLE_HEIGHT = 20;
int ACTUATOR_SPEED = 255;

// Define a public direction - 0:DIRECTION_STOP, 1: DIRECTION_EXTEND, -1: DIRECTION_RETRACT
// This value will be used to determine if the actuator should be moving in one direction or
// the other (or stop)
int MOTION_DIRECTION = DIRECTION_STOP;


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define EXTEND_BUTTON 3
#define RETRACT_BUTTON 2

#define EXTEND_PWM 11
#define RETRACT_PWM 10


void setup() {
  // set up serial output
  Serial.begin(115200);
  //Serial.println("Serial started ...");

  // set up the button pins
  pinMode(EXTEND_BUTTON, INPUT_PULLUP);
  pinMode(RETRACT_BUTTON, INPUT_PULLUP);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }


  // Clear the buffer
  display.clearDisplay();
  display.setRotation(1);

  drawTitle();

  drawReferenceLines();
  drawReferenceValues();


  // Get the current sensor reading to determine where the flaps are currently positioned
  int SensorReading = getCurrentSensorReading(); //map( analogRead(A2), 10, 870, ANGLES[0], ANGLES[NOTCH_COUNT - 1] );

  // Find the current notch - go through the list of notches to see if the reading
  // falls above it and below the next one
  CURRENT_NOTCH = getCurrentNotch();

  Serial.print("Beginning CURRENT_NOTCH:");Serial.println(CURRENT_NOTCH);

  // while (SensorReading < ANGLES[CURRENT_NOTCH] || CURRENT_NOTCH >= NOTCH_COUNT) {
  //   CURRENT_NOTCH++;
  // }



}

void loop() {

  CURRENT_NOTCH = getCurrentNotch();
  if (digitalRead(RETRACT_BUTTON) == LOW) {
    Serial.println("RETRACT Button pressed ...");
    if (MOTION_DIRECTION == DIRECTION_EXTEND) {
      MOTION_DIRECTION = DIRECTION_STOP;
      delay(100);
    } else {
      MOTION_DIRECTION = DIRECTION_RETRACT;
      if (getCurrentSensorReading() >= ANGLES[CURRENT_NOTCH] && CURRENT_NOTCH != 0){
        TARGET_NOTCH = setPreviousNotch();
      }
      delay(100);
    }
  } else if (digitalRead(EXTEND_BUTTON) == LOW) {
    Serial.println("EXTEND Button pressed ...");
    if (MOTION_DIRECTION == DIRECTION_RETRACT) {
      MOTION_DIRECTION = DIRECTION_STOP;
      delay(100);
    } else {
      MOTION_DIRECTION = DIRECTION_EXTEND;
      if (getCurrentSensorReading() <= ANGLES[CURRENT_NOTCH] && CURRENT_NOTCH < NOTCH_COUNT) {
        TARGET_NOTCH = setNextNotch();
      }
      delay(100);
    }
  }

  Serial.print("CURRENT_NOTCH:");Serial.println(CURRENT_NOTCH);
  Serial.print("TARGET_NOTCH:");Serial.println(TARGET_NOTCH);
  Serial.print("ANGLES[TARGET_NOTCH]:");Serial.println(ANGLES[TARGET_NOTCH]);

  if (MOTION_DIRECTION == DIRECTION_RETRACT) {

    Serial.println("MOTION_DIRECTION: DIRECTION_RETRACT");    

    if (ANGLES[TARGET_NOTCH] >= getCurrentSensorReading()) {
      analogWrite (EXTEND_PWM, 0);
      analogWrite (RETRACT_PWM, ACTUATOR_SPEED);
    } else {

      analogWrite (RETRACT_PWM, 0);
      analogWrite (EXTEND_PWM, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      Serial.println("MOTION_DIRECTION: DIRECTION_STOP (if statement ln 152)");

      CURRENT_NOTCH = getCurrentNotch();
    }
  } else if (MOTION_DIRECTION == DIRECTION_EXTEND) {

    Serial.println("MOTION_DIRECTION: DIRECTION_EXTEND");

    if (ANGLES[TARGET_NOTCH] <= getCurrentSensorReading()) {
      analogWrite (RETRACT_PWM, 0);
      analogWrite (EXTEND_PWM, ACTUATOR_SPEED);
    } else {
      
      analogWrite (EXTEND_PWM, 0);
      analogWrite (RETRACT_PWM, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      Serial.println("MOTION_DIRECTION: DIRECTION_STOP (if statement ln 167)");
      
      CURRENT_NOTCH = getCurrentNotch();
    }
  } 
  // if (digitalRead(RETRACT_BUTTON) == LOW) {
  //   Serial.println("Retract Button Pressed");
  //   // analogWrite(RETRACT_PWM, ACTUATOR_SPEED);
  //   retractFlapsOneNotch();
  // } else if (digitalRead(EXTEND_BUTTON) == LOW) {
  //   Serial.println("Extend Button Pressed");   
  //   // analogWrite(EXTEND_PWM, ACTUATOR_SPEED);
  //   extendFlapsOneNotch();
  // } else {
  //   analogWrite(RETRACT_PWM, 0);
  //   analogWrite(EXTEND_PWM, 0);
  // }
  
  // int SensorReading = getTestAngleData();
  
  
  // int SensorReading = getCurrentSensorReading(); // map( analogRead(A2), 10, 870, ANGLES[0], ANGLES[NOTCH_COUNT - 1] );
  // drawNewAngle(SensorReading);
  
  drawNewAngle();
  
  // Serial.print("loop() sensorReading: ");Serial.println(SensorReading, DEC);
  delay(500);
  
}

void drawReferenceLines() {
  int numberOfNotches = sizeof(ANGLES) / sizeof(ANGLES[0]);
  int screenDistanceBetweenNotches = (SCREEN_WIDTH - (TITLE_MARGIN * 2)) / numberOfNotches;

  // iterate through the defined notch settings
  for (int i = 0; i < numberOfNotches; i++) {
    Serial.print(ANGLES[i]); Serial.print(":");
    drawReferenceLine(ANGLES[i], i, screenDistanceBetweenNotches);
  }
  Serial.println();
}

void drawReferenceLine(int notch, int notchNumber, int distance) {
  int yPosition = (notchNumber * distance) + TITLE_MARGIN;
  display.fillRect(23, yPosition + TITLE_MARGIN + 11, 13, 5, SSD1306_WHITE);
  // display.fillRect(23, getYPosition(angle)+TITLE_MARGIN, 13, 5, SSD1306_WHITE);
  display.display();

}

void drawTitle() {

  char textValues[] = "FLAPS";

  display.setTextSize(2);
  display.fillRect(0, 0, SCREEN_HEIGHT, 16, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(3, 1);
  display.cp437(true);
  display.write(textValues);
  display.display();

}

void drawReferenceValues() {
  // int numberOfNotches = sizeof(ANGLES) / sizeof(ANGLES[0]);
  int screenDistanceBetweenNotches = (SCREEN_WIDTH - (TITLE_MARGIN * 2)) / NOTCH_COUNT;

  // iterate through the defined notch settings
  for (int i = 0; i < NOTCH_COUNT; i++) {
    drawReferenceValue(ANGLES[i], i, screenDistanceBetweenNotches);
  }

}

void drawReferenceValue(int angle, int notchNumber, int distance) {
  int yPosition = notchNumber * distance + TITLE_MARGIN;
  
  char textValues[3];
  if (angle < 10) {
    sprintf(textValues, " %d", angle);
  } else {
    sprintf(textValues, "%d", angle);
  }
  

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, yPosition + TITLE_MARGIN + 6 );
  display.cp437(true);

  display.write(textValues);
  display.display();

  Serial.println(angle);

}

void drawNewAngle() {
  int angle = getCurrentSensorReading();
  int yPosition = map(angle, ANGLES[0], ANGLES[NOTCH_COUNT - 1], TITLE_HEIGHT, SCREEN_WIDTH);
  yPosition = (yPosition / 2) * 2;
  display.fillRect(0, TITLE_HEIGHT, 22, SCREEN_WIDTH, SSD1306_BLACK); 
  display.fillRect(2, yPosition + TITLE_MARGIN - 1, 18, 5, SSD1306_WHITE);
  display.display();

}

// void drawNewAngle(int angle) {
//   // angle = (angle / 2) * 2;
//   // int numberOfNotches = sizeof(ANGLES) / sizeof(ANGLES[0]);
//   int screenDistanceBetweenNotches = (SCREEN_WIDTH - (TITLE_MARGIN * 2)) / NOTCH_COUNT;
  
//   int yPosition = map(angle, ANGLES[0], ANGLES[NOTCH_COUNT - 1], TITLE_HEIGHT, SCREEN_WIDTH - TITLE_HEIGHT);
//   yPosition = (yPosition / 2) * 2;
//   // Serial.print("new angle yPosition: "); Serial.println(yPosition);
//   display.fillRect(0, TITLE_HEIGHT, 22, SCREEN_WIDTH, SSD1306_BLACK);
//   display.fillRect(2, yPosition + TITLE_MARGIN -1, 18, 5, SSD1306_WHITE);
//   display.display();

// }



int getYPosition(int angle) {
  // int newValue = map(angle, 0, ANGLE3 + 5, 20, SCREEN_WIDTH);
  // int arraySize = sizeof(ANGLES) / sizeof(ANGLES[0]);
  int newValue = map(angle, ANGLES[0], ANGLES[NOTCH_COUNT - 1], TITLE_HEIGHT, SCREEN_WIDTH);
  Serial.print("angle:");Serial.print(angle); Serial.print(" yPosition: "); Serial.println(newValue);
  return newValue;
  
}

int getCurrentSensorReading() {
  return map( analogRead(A2), 10, 870, ANGLES[0], ANGLES[NOTCH_COUNT - 1] );
}

int getCurrentNotch() {
  int currentNotch = 0;
  int SensorReading = getCurrentSensorReading();
  for(int i = 0; i < NOTCH_COUNT; i++) {
    if (ANGLES[i] >= SensorReading) {
      currentNotch = i;
      break;
    }
  }
  return currentNotch;
}

void retractFlapsOneNotch() {

  int SensorReading = getCurrentSensorReading();
  if (CURRENT_NOTCH > 0) {
    while (ANGLES[CURRENT_NOTCH - 1] <= SensorReading) {
      SensorReading = getCurrentSensorReading();
      if (digitalRead(EXTEND_BUTTON) == LOW) {
        analogWrite(RETRACT_PWM, 0);
        analogWrite(EXTEND_PWM, 0);
        delay(200);
        break;
      } else {
        analogWrite(RETRACT_PWM, ACTUATOR_SPEED);
        //drawNewAngle(SensorReading);
        drawNewAngle();
      }
    }
    CURRENT_NOTCH = CURRENT_NOTCH -1;
  } else {
    // need to retract to 0 in case partial travel has been done without increasing the current notch
    if (getCurrentSensorReading() >= ANGLES[0]){
      while (getCurrentSensorReading() >= ANGLES[0] ){
        analogWrite(RETRACT_PWM, ACTUATOR_SPEED);
        // drawNewAngle(SensorReading);
        drawNewAngle();
        // delay(100);
      }
    }
  }
  
}

void extendFlapsOneNotch() {
  
  int SensorReading = getCurrentSensorReading();
  if (CURRENT_NOTCH < NOTCH_COUNT - 1) {
    while (ANGLES[CURRENT_NOTCH + 1] >= SensorReading) {
      SensorReading = getCurrentSensorReading();
      if (digitalRead(RETRACT_BUTTON) == LOW) {
        analogWrite(RETRACT_PWM, 0);
        analogWrite(EXTEND_PWM, 0);
        delay(200);
        break;
      } else {
        analogWrite(EXTEND_PWM, ACTUATOR_SPEED);
        // drawNewAngle(SensorReading);
        drawNewAngle();
      }
      
    }
    CURRENT_NOTCH = CURRENT_NOTCH + 1;
  } else {
    if (getCurrentSensorReading() <= ANGLES[NOTCH_COUNT - 1]){
      while (getCurrentSensorReading() <= ANGLES[NOTCH_COUNT - 1] ){
        analogWrite(EXTEND_PWM, ACTUATOR_SPEED);
        // drawNewAngle(SensorReading);
        drawNewAngle();
        // delay(100);
      }
    }
  }
}

void stopFlapMotion() {
  analogWrite(RETRACT_PWM, 0);
  analogWrite(EXTEND_PWM, 0);

}

int setPreviousNotch() {
  int previousNotch = NOTCH_COUNT - 1;
  int currentSensorReading = getCurrentSensorReading();
  if (CURRENT_NOTCH <= 0) {
    previousNotch = 0;
  } else {
    for (int i = NOTCH_COUNT - 1; i >= 0; i--) {
      if (ANGLES[i] > currentSensorReading) {
        previousNotch = i;
      }
    }
  }
  
  return previousNotch;
}

int setNextNotch() {

  int nextNotch = 0;
  int currentSensorReading = getCurrentSensorReading();
  if (CURRENT_NOTCH > NOTCH_COUNT) {
    nextNotch = NOTCH_COUNT - 1;
  } else {
    for (int i = 0; i < NOTCH_COUNT; i++) {
      if (ANGLES[i] < currentSensorReading) {
        nextNotch = i;
      }
    }
  }
  
  return nextNotch;

}



