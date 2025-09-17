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

/**************************************************************************
These two values represent the values recevied on the A2 pin of the Arduino,
which is connected to the yellow wire of the position sensor of the flap
actuator. 
**************************************************************************/
#define FULL_RETRACT_VALUE 10
#define FULL_EXTEND_VALUE 870

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

#define EXTEND_PWM_PIN 11
#define RETRACT_PWM_PIN 10


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

  /*****************************************************************************
  First - run the getCurrentNotch() function. This will figure out which 
  "notch" the flaps are set to when powered on.
  In this context, a "notch" is  the array element of the ANGLES[] array. 
  For example, suppose you have ANGLES[] defined as 0, 10, 20, 30 - the "notches" 
  will be:

  Notch | Angle 
  -------------
  0     | 0
  1     | 10
  2     | 20
  3     | 30

  the getCurrentNotch() function will return the nearest notch (rounded down) that
  the flaps are currently set to. For example, if the flaps are set to 23 degrees,
  the getCurrentNotch() function will return the number 2.

  Assigne the notch number and assign it to the CURRENT_NOTCH variable:
  *****************************************************************************/

  CURRENT_NOTCH = getCurrentNotch();

  /*****************************************************************************
  Next, look at the buttons to see if the pilot is trying to change the position
  of the flaps. There are two buttons: RETRACT_BUTTON and EXTEND_BUTTON. This
  could also be controlled with a switch. In either case, the pilot is giving input
  to either extend or retract the flaps, or neither (e.g. not button is pressed or
  the switch is in the middle position)
  *****************************************************************************/
  if (digitalRead(RETRACT_BUTTON) == LOW) {
    Serial.println("RETRACT Button pressed ...");
    
    /*****************************************************************************
    If the RETRACT_BUTTON is pressed, first check to see if the extend command was
    previously given. That directive is stored in a variable called MOTION_DIRECTION.
    There are three pre-defined states that the MOTION_DIRECTION can be in:
    - DIRECTION_RETRACT (-1)
    - DIRECTION_STOP    (0)
    - DIRECTION_EXTEND  (1)
    *****************************************************************************/
    
    if (MOTION_DIRECTION == DIRECTION_EXTEND) {
      /*****************************************************************************
      if the MOTION_DIRECTION variable is currently set to DIRECTION_EXTEND, but the 
      pilot presses the RETRACT_BUTTON, the flaps will stop moving. Otherwise, they 
      will begin to retract. If the pilot holds the button for more than 100 milliseconds
      (notice the delay(100) line below), the flaps should stop and then start to retract
      to the next more retracted notch (e.g. from notch 2 to notch 1).
      *****************************************************************************/ 

      MOTION_DIRECTION = DIRECTION_STOP;
      delay(100); // is this necessary? Probably not

    } else {
      /*****************************************************************************
      if the MOTION_DIRECTION variable is NOT currently set to DIRECTION_EXTEND, 
      (e.g. either currently set to stop or retract), the flaps will be set to 
      start/continue to retract.
      *****************************************************************************/ 

      MOTION_DIRECTION = DIRECTION_RETRACT;

      /*****************************************************************************
      Get the current notch and determine whether or not to move the flaps one more
      notch. 
      ****************************************************************************/
      if (getCurrentSensorReading() >= ANGLES[CURRENT_NOTCH] && CURRENT_NOTCH != 0){
        /*****************************************************************************
        the TARGET_NOTCH variable will be used to determine if the flaps have reached
        their desired position
        ****************************************************************************/

        TARGET_NOTCH = setPreviousNotch();

      }
      delay(100); // is this necessary? Probably not
    }
  } else if (digitalRead(EXTEND_BUTTON) == LOW) {

    /****************************************************************************
    If the EXTEND_BUTTON is pressed, do the opposite of what was done if the RETRACT_BUTTON
    is pressed - e.g. stop if the MOTION_DIRECTION variable is set to either stop or 
    continue to extend to the next notch. 
    ****************************************************************************/
    
    Serial.println("EXTEND Button pressed ...");
    if (MOTION_DIRECTION == DIRECTION_RETRACT) {
      MOTION_DIRECTION = DIRECTION_STOP;
      delay(100); // Is this necessary? probably not
    } else {
      MOTION_DIRECTION = DIRECTION_EXTEND;
      if (getCurrentSensorReading() <= ANGLES[CURRENT_NOTCH] && CURRENT_NOTCH < NOTCH_COUNT) {
        TARGET_NOTCH = setNextNotch();
      }
      delay(100); // Is this necessary? probably not
    }
  }

  Serial.print("CURRENT_NOTCH:");Serial.println(CURRENT_NOTCH);
  Serial.print("TARGET_NOTCH:");Serial.println(TARGET_NOTCH);
  Serial.print("ANGLES[TARGET_NOTCH]:");Serial.println(ANGLES[TARGET_NOTCH]);

  /****************************************************************************
  Now that we know what the pilot WANTS the flaps to do, time to actually do 
  something ...

  Use the MOTION_DIRECTION variable (set above) to determine what to do
  ****************************************************************************/
  
  if (MOTION_DIRECTION == DIRECTION_RETRACT) {

    Serial.println("MOTION_DIRECTION: DIRECTION_RETRACT");    
    
    /****************************************************************************
    Start/continue retracting if that's what the pilot wants to do. 
    
    Take a look at the TARGET_NOTCH element of the ANGLES array and compare that
    to the current reading from the flap actuator. 

    Remember, the ANGLES array might look something like this:
      Notch | Angle 
    -------------
    0     | 0
    1     | 10
    2     | 20
    3     | 30

    So if the current position of the flaps comes back as 28 (that is returned by 
    the getCurrentSensorReading() function) and the TARGET_NOTCH is currently set to
    the next lower notch (in this example, 2) - the motor controller will set the
    extend pin to 0 (not extending) and the retract pin to whatever ACTUATOR_SPEED 
    is set to. 255 would be full speed. This is set near the top of this file.

    These lines will tell the motor controller to stop:
    analogWrite (RETRACT_PWM_PIN, 0);
    analogWrite (EXTEND_PWM_PIN, 0);

    These lines will tell the motor controller to retract (0 speed for extend, full
    speed retract):
    analogWrite (EXTEND_PWM_PIN, 0);
    analogWrite (RETRACT_PWM_PIN, ACTUATOR_SPEED);

    *** VERY IMPORTANT ***
    The lines below will tell the motor controller to extend (notice that the RETRACT and 
    EXTEND lines are reversed from above. You want to STOP the opposite direction BEFORE setting
    the direction to move - otherwise there may be a brief moment where the actuator is 
    trying to both extend AND retract at the same time - this would be a short circuit since
    we're effectively forcing the power connection to the actuator to reverse - positive to negative/
    negative to positive):
    analogWrite (RETRACT_PWM_PIN, 0);
    analogWrite (EXTEND_PWM_PIN, ACTUATOR_SPEED);

    OK, all that out of the way, let's tell the flaps to move or stop in the correct direction:
    ****************************************************************************/
    
    if (ANGLES[TARGET_NOTCH] >= getCurrentSensorReading()) {
      analogWrite (EXTEND_PWM_PIN, 0);
      analogWrite (RETRACT_PWM_PIN, ACTUATOR_SPEED);
    } else {

      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      Serial.println("MOTION_DIRECTION: DIRECTION_STOP (if statement ln 152)");

      CURRENT_NOTCH = getCurrentNotch();
    }
  } else if (MOTION_DIRECTION == DIRECTION_EXTEND) {

    /**************************************************************************
    Looks like we want to EXTEND the flaps
    ***************************************************************************/

    Serial.println("MOTION_DIRECTION: DIRECTION_EXTEND");

    if (ANGLES[TARGET_NOTCH] <= getCurrentSensorReading()) {
      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, ACTUATOR_SPEED);
    } else {
      
      analogWrite (EXTEND_PWM_PIN, 0);
      analogWrite (RETRACT_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      Serial.println("MOTION_DIRECTION: DIRECTION_STOP (if statement ln 167)");
      
      CURRENT_NOTCH = getCurrentNotch();
    }
  } else {

    /**************************************************************************
    If we don't want to either extend or retract the flaps, the only other option
    is to stop them. 
    ***************************************************************************/

    analogWrite (EXTEND_PWM_PIN, 0);
    analogWrite (RETRACT_PWM_PIN, 0);

  }

  // if (digitalRead(RETRACT_BUTTON) == LOW) {
  //   Serial.println("Retract Button Pressed");
  //   // analogWrite(RETRACT_PWM_PIN, ACTUATOR_SPEED);
  //   retractFlapsOneNotch();
  // } else if (digitalRead(EXTEND_BUTTON) == LOW) {
  //   Serial.println("Extend Button Pressed");   
  //   // analogWrite(EXTEND_PWM_PIN, ACTUATOR_SPEED);
  //   extendFlapsOneNotch();
  // } else {
  //   analogWrite(RETRACT_PWM_PIN, 0);
  //   analogWrite(EXTEND_PWM_PIN, 0);
  // }
  
  // int SensorReading = getTestAngleData();
  
  
  // int SensorReading = getCurrentSensorReading(); // map( analogRead(A2), FULL_RETRACT_VALUE, FULL_EXTEND_VALUE, ANGLES[0], ANGLES[NOTCH_COUNT - 1] );
  // drawNewAngle(SensorReading);
  
  /**************************************************************************
  Now, let's update the screen by drawing a little line to indicate where the flaps
  are currently extended to. 
  ***************************************************************************/
  drawNewAngle();
  
  // Serial.print("loop() sensorReading: ");Serial.println(SensorReading, DEC);
  // delay(500); 
  
}

/*
This function is called in the setup() loop. It will draw the lines on the right
side of the screen to denote "notches" of flaps. They remain stationary
*/
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

/*
This is the function that draws each individual line - called from the function
above - once for each element in the ANGLES[] array
*/
void drawReferenceLine(int notch, int notchNumber, int distance) {
  int yPosition = (notchNumber * distance) + TITLE_MARGIN;
  display.fillRect(23, yPosition + TITLE_MARGIN + 11, 13, 5, SSD1306_WHITE);
  // display.fillRect(23, getYPosition(angle)+TITLE_MARGIN, 13, 5, SSD1306_WHITE);
  display.display();

}

/*
This function puts a small title at the top of the screen that says "FLAPS"
*/
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

/*
This function writes text to the right of each reference line to show the pilot what each
notch of flaps should represent (e.g. in angles) - defined in the ANGLES[] array
*/
void drawReferenceValues() {
  // int numberOfNotches = sizeof(ANGLES) / sizeof(ANGLES[0]);
  int screenDistanceBetweenNotches = (SCREEN_WIDTH - (TITLE_MARGIN * 2)) / NOTCH_COUNT;

  // iterate through the defined notch settings
  for (int i = 0; i < NOTCH_COUNT; i++) {
    drawReferenceValue(ANGLES[i], i, screenDistanceBetweenNotches);
  }

}

/*
This is the function that writes each individual angle value - called from the function
above - once for each element in the ANGLES[] array
*/
void drawReferenceValue(int angle, int notchNumber, int distance) {
  int yPosition = notchNumber * distance + TITLE_MARGIN;
  
  char textValues[3];

  // add a space before the angle if it's less than 10
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

/*
this function will remove any old lines depicting the current flap angle and 
draw a new line where it needs to be
*/
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


/*
This function will determine the "y" position of the lines being drawn on the screen.
It uses a combination of the angle where you want to draw it and interpolates a pixel
number from that based on the number of available pixels on the screen. 

It uses the Arduino map() function to do that:
map(value, fromLow, fromHigh, toLow, toHigh) -> interpolated value
*/
int getYPosition(int angle) {
  
  int newValue = map(angle, ANGLES[0], ANGLES[NOTCH_COUNT - 1], TITLE_HEIGHT, SCREEN_WIDTH);
  Serial.print("angle:");Serial.print(angle); Serial.print(" yPosition: "); Serial.println(newValue);
  return newValue;
  
}

/*
reads the A2 pin on the Arduino (connected to the position output wire on the actuator) to get a value from
0-1023. 

The sensor wires on the actuator work like this:
- +5V on the BLUE wire
- GND on the WHITE wire
- voltage output between 0 and 5V on the YELLOW wire. 

The yellow wire is connected to the Analog 2 (A2) pin on the arduino. The arduino converts
that 0-5V voltage to a value between 0 and 1023, respectively, on that pin. Using the map() 
function, we can interpolate between fully retracted and fully extended values and output
what those values are based on values in the ANGLES[] array. 
*/
int getCurrentSensorReading() {
  return map( analogRead(A2), FULL_RETRACT_VALUE, FULL_EXTEND_VALUE, ANGLES[0], ANGLES[NOTCH_COUNT - 1] );
}

/*
returns an integer value denoting the current "notch" based on the current angle read by the 
position sensor in the flap actuator
*/
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


// void retractFlapsOneNotch() {

//   int SensorReading = getCurrentSensorReading();
//   if (CURRENT_NOTCH > 0) {
//     while (ANGLES[CURRENT_NOTCH - 1] <= SensorReading) {
//       SensorReading = getCurrentSensorReading();
//       if (digitalRead(EXTEND_BUTTON) == LOW) {
//         analogWrite(RETRACT_PWM_PIN, 0);
//         analogWrite(EXTEND_PWM_PIN, 0);
//         delay(200);
//         break;
//       } else {
//         analogWrite(RETRACT_PWM_PIN, ACTUATOR_SPEED);
//         //drawNewAngle(SensorReading);
//         drawNewAngle();
//       }
//     }
//     CURRENT_NOTCH = CURRENT_NOTCH -1;
//   } else {
//     // need to retract to 0 in case partial travel has been done without increasing the current notch
//     if (getCurrentSensorReading() >= ANGLES[0]){
//       while (getCurrentSensorReading() >= ANGLES[0] ){
//         analogWrite(RETRACT_PWM_PIN, ACTUATOR_SPEED);
//         // drawNewAngle(SensorReading);
//         drawNewAngle();
//         // delay(100);
//       }
//     }
//   }
  
// }

// void extendFlapsOneNotch() {
  
//   int SensorReading = getCurrentSensorReading();
//   if (CURRENT_NOTCH < NOTCH_COUNT - 1) {
//     while (ANGLES[CURRENT_NOTCH + 1] >= SensorReading) {
//       SensorReading = getCurrentSensorReading();
//       if (digitalRead(RETRACT_BUTTON) == LOW) {
//         analogWrite(RETRACT_PWM_PIN, 0);
//         analogWrite(EXTEND_PWM_PIN, 0);
//         delay(200);
//         break;
//       } else {
//         analogWrite(EXTEND_PWM_PIN, ACTUATOR_SPEED);
//         // drawNewAngle(SensorReading);
//         drawNewAngle();
//       }
      
//     }
//     CURRENT_NOTCH = CURRENT_NOTCH + 1;
//   } else {
//     if (getCurrentSensorReading() <= ANGLES[NOTCH_COUNT - 1]){
//       while (getCurrentSensorReading() <= ANGLES[NOTCH_COUNT - 1] ){
//         analogWrite(EXTEND_PWM_PIN, ACTUATOR_SPEED);
//         // drawNewAngle(SensorReading);
//         drawNewAngle();
//         // delay(100);
//       }
//     }
//   }
// }

// void stopFlapMotion() {
//   analogWrite(RETRACT_PWM_PIN, 0);
//   analogWrite(EXTEND_PWM_PIN, 0);

// }

/*
returns the current notch - 1
*/
int setPreviousNotch() {
  int previousNotch = NOTCH_COUNT - 1;
  int currentSensorReading = getCurrentSensorReading();

  // check to see if the actuator is already at the first notch (based on ANGLES[] elements) ...
  if (CURRENT_NOTCH <= 0) {
    // ... if so, set it to 0 again
    previousNotch = 0;
  } else {
    // ... otherwise, find the previous notch and return the element id
    for (int i = NOTCH_COUNT - 1; i >= 0; i--) {
      if (ANGLES[i] > currentSensorReading) {
        previousNotch = i;
        break; // we found what we're looking for and can exit the loop
      }
    }
  }
  
  return previousNotch;
}

/*
returns the CURRENT_NOTCH + 1
*/
int setNextNotch() {

  int nextNotch = 0;
  int currentSensorReading = getCurrentSensorReading();

  // check to see if the actuator is already at the last notch (based on ANGLES[] elements) ...
  if (CURRENT_NOTCH > NOTCH_COUNT) {
    // ... if so, set it to the last notch again
    nextNotch = NOTCH_COUNT - 1;
  } else {
    // ... otherwise, find the next notch and return the element id
    for (int i = 0; i < NOTCH_COUNT; i++) {
      if (ANGLES[i] < currentSensorReading) {
        nextNotch = i;
        break; // we found what we're looking for and can exit the loop
      }
    }
  }
  
  return nextNotch;

}



