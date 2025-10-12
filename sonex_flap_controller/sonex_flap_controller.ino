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

// OLED screen size in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// a couple of pre-defined values that determine which way the flaps should move (or not)
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

/**************************************************************************
This is where you define your various flap notches. You can pretty much
put any integer values you want to appear on the screen. Ideally you will
do some testing to figure out what angles your flaps actually sit at for
each notch. The first and last element in this array should be 0 and whatever
full flaps are deflected to. Sonex says full flaps on the B model is 30 deg.
Because they suggest 0 for clean, 10 for takeoff, and 30 for landing, that's 
what I've put here. Feel free to add additional "notches" in there if you 
think it's necessary. If you wanted to add a notch for 20 deg, it would look 
like this:
int ANGLES[] = { 0, 10, 20, 30 };
***************************************************************************/ 
int ANGLES[] = { 0, 15, 30 };


// This defines the number of notches for later use
int NOTCH_COUNT = sizeof(ANGLES) / sizeof(ANGLES[0]);


/************************************************************************** 
These two variables are used a lot to determine where the flaps currently
are and where you want them to eventually stop.
***************************************************************************/ 
int CURRENT_NOTCH = 0;
int TARGET_NOTCH = 0;

int TITLE_MARGIN = 8;
int TITLE_HEIGHT = 20;


/***************************************************************************
 Define a public variable to define the current direction of the actuator - 
 0:DIRECTION_STOP, 1: DIRECTION_EXTEND, -1: DIRECTION_RETRACT
 This value will be used to determine if the actuator should be moving in one direction or
 the other (or stop) between button presses until it has reached the specified notch
***************************************************************************/ 
int MOTION_DIRECTION = DIRECTION_STOP;


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// define pin numbers for buttons
#define RETRACT_BUTTON 2
#define EXTEND_BUTTON 3

/***************************************************************************
Define pin numbers and speed for Pulse Width Modulation (PWM), which is used to tell 
the motor driver to move at a certain speed or stop.

In case you don't know what PWM is ... 

The first two lines below define which of the Arduino's digital pins are to be used for
PWM signalling (pins 10 and 11). They are both "digital" pins, meaning they can only be 
either on or off (nothing in between).

PWM is determined by turning each of the pins on and off at a set rate, from 0-255. 

A PWM of 0 denotes that the pin is alway set to 0V. A PWM of 255 denotes that the pin
is always set to 5V. A PWM of 127 denotes that the pin goes from 0V to 5V for half of
each PWM cycle - on a typical Arduino (such as the Nano used in the project), that cycle is 
about every 2 milliseconds. So a PWM signal of 127 means that it's about 1ms on and 1ms off. 

To slow down the actuator, you would set your PWM signal at a low number - say 10. 
To speed up the movement of the actuator, you would increase the "duty cycle" to something
higher - up to 255. 

There's a good write-up on Arduino's website:
https://docs.arduino.cc/learn/microcontrollers/analog-output/ 

The first two lines only define which pins to use for retracting and extending the flaps.
The third line defines how fast the actuator should go. I don't see any reason to set it
lower than full speed (because the Sonex flap actuator isn't super fast), so 255 it is. 
***************************************************************************/ 
#define RETRACT_PWM_PIN 10
#define EXTEND_PWM_PIN 11
#define ACTUATOR_SPEED 255


// Decide whether or not to output serial data
bool SERIAL_OUTPUT = false;

// run the setup() function. This section runs one time only, when the Arduino is powered on
void setup() {
  // set up serial output
  Serial.begin(115200);

  /*
    set up the button pins. We are initiating each button with a pin number 
    (defined by EXTEND_BUTTON or RETRACT_BUTTON defined above) 
    and specifying that we want to connect the pin to a pull-up resistor. 
    Basically, this adds a 5V signal to the pin at all times until we press a button
    and ground the 5V. When that happens, the Arduino can tell that the button is pressed.
  */ 
  pinMode(EXTEND_BUTTON, INPUT_PULLUP);
  pinMode(RETRACT_BUTTON, INPUT_PULLUP);

  /*
   Set up the display. First, configure the correct type and I2C address (0x3c)
   Next, clear it the screen (the library displays an Adafruit logo by default).
   If you clearDisplay() immediately (like we're doing here), it doesn't appear.
   Finally, set the rotation to 90 degrees - we want to use it in portrait mode
   by using display.setRotation(1) -  you could also set it upside down using 
   display.setRotation(3), but I haven't tested that - good luck if you choose to
   do so. :)
  */ 
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    if (SERIAL_OUTPUT) { Serial.println(F("SSD1306 allocation failed")); }
  }
  display.clearDisplay();
  display.setRotation(1);

  /*
   Next, use the drawTitle() function below to put a bar at the top of the screen with text that 
   says "Flaps."  
   
   Then draw lines where the notches should be and finally draw numbers next to those 
   lines - the lines and numbers will change depending on your settings in the ANGLES[] array 
   above
  */
  drawTitle();
  drawReferenceLines();
  drawReferenceValues();


  // Get the current sensor reading to determine where the flaps are currently positioned
  int SensorReading = getCurrentSensorReading(); 

  // Find the current notch - the getCurrentNotch() function goes through the list of notches 
  // to see if the flaps are set at a particular notch using the SensorReading we just grabbed
  CURRENT_NOTCH = getCurrentNotch();

  if (SERIAL_OUTPUT) { Serial.print("Beginning CURRENT_NOTCH:");Serial.println(CURRENT_NOTCH); }

}


// Run the loop() function - this function repeats over and over indefintely after the setup() function runs
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

  We assign the notch number to a variable called CURRENT_NOTCH
  *****************************************************************************/
  CURRENT_NOTCH = getCurrentNotch();


  /*****************************************************************************
  Next, look at the buttons to see if the pilot is trying to change the position
  of the flaps. There are two buttons: RETRACT_BUTTON and EXTEND_BUTTON. This
  could also be controlled with a switch. In either case, the pilot is giving input
  to either extend or retract the flaps, or neither (e.g. no button is pressed or
  the switch is in the middle position)
  *****************************************************************************/
  if (digitalRead(RETRACT_BUTTON) == LOW) {
    if (SERIAL_OUTPUT) { Serial.println("RETRACT Button pressed ..."); }
    
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
      delay(200); 

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
      if (getCurrentSensorReading() >= ANGLES[getPreviousNotch()]){
        /*****************************************************************************
        the TARGET_NOTCH variable will be used to determine if the flaps have reached
        their desired position
        ****************************************************************************/

        TARGET_NOTCH = getPreviousNotch();

      }
      delay(200); // is this necessary? Probably not
    }
  } else if (digitalRead(EXTEND_BUTTON) == LOW) {

    /****************************************************************************
    If the EXTEND_BUTTON is pressed, do the opposite of what was done if the RETRACT_BUTTON
    is pressed - e.g. stop if the MOTION_DIRECTION variable is set to either stop or 
    continue to extend to the next notch. 
    ****************************************************************************/
    
    if (SERIAL_OUTPUT) { Serial.println("EXTEND Button pressed ..."); }
    if (MOTION_DIRECTION == DIRECTION_RETRACT) {
    
      MOTION_DIRECTION = DIRECTION_STOP;
      delay(200); // Is this necessary? probably not
    
    } else {
    
      MOTION_DIRECTION = DIRECTION_EXTEND;
      if (getCurrentSensorReading() <= ANGLES[getNextNotch()]) {
        TARGET_NOTCH = getNextNotch();
      }
      delay(200); // Is this necessary? probably not
    
    }
  }



  /****************************************************************************
  Now that we know what the pilot WANTS the flaps to do, time to actually do 
  something ...

  Use the MOTION_DIRECTION variable (set above) to determine what to do
  ****************************************************************************/
  
  if (MOTION_DIRECTION == DIRECTION_RETRACT) {

    // if (SERIAL_OUTPUT) { Serial.println("MOTION_DIRECTION: DIRECTION_RETRACT"); }
    
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

    These lines would tell the motor controller to stop:
    analogWrite (RETRACT_PWM_PIN, 0);
    analogWrite (EXTEND_PWM_PIN, 0);

    These lines would tell the motor controller to retract (0 speed for extend, full
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
    
    if (getCurrentSensorReading() >= ANGLES[TARGET_NOTCH] ) {
      
      /**************************************************************************
      Looks like the flaps are currently more extended than we want, so RETRACT them
      **************************************************************************/
      
      analogWrite (EXTEND_PWM_PIN, 0);
      analogWrite (RETRACT_PWM_PIN, ACTUATOR_SPEED);

    } else {

      // otherwise, stop motion
      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      delay(100); // wait just a moment before proceeding
      
    }
  } else if (MOTION_DIRECTION == DIRECTION_EXTEND) {

    /**************************************************************************
    Looks like the flaps are more retracted than we want, so EXTEND them
    ***************************************************************************/

    // if (SERIAL_OUTPUT) { Serial.println("MOTION_DIRECTION: DIRECTION_EXTEND"); }

    if (getCurrentSensorReading() <= ANGLES[TARGET_NOTCH] ) {

      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, ACTUATOR_SPEED);

    } else {
      
      analogWrite (EXTEND_PWM_PIN, 0);
      analogWrite (RETRACT_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      delay(100); // wait a moment before proceeding

    }
  } else {

    /**************************************************************************
    If we don't want to either extend or retract the flaps, the only other option
    is to stop them. 
    ***************************************************************************/

    analogWrite (EXTEND_PWM_PIN, 0);
    analogWrite (RETRACT_PWM_PIN, 0);

  }

  // CURRENT_NOTCH = getCurrentNotch();

  /**************************************************************************
  Now, let's update the screen by drawing a little line to indicate where the flaps
  are currently extended to. We do this using a function called drawNewAngle()
  ***************************************************************************/
  drawNewAngle();
  
  if (SERIAL_OUTPUT) { 
    Serial.print("CURRENT_NOTCH:");Serial.print(CURRENT_NOTCH);
    Serial.print(" | TARGET_NOTCH:");Serial.print(TARGET_NOTCH);
    Serial.print(" | ANGLES[TARGET_NOTCH]:");Serial.println(ANGLES[TARGET_NOTCH]);
    Serial.print("cur angle:");Serial.print(getCurrentSensorReading()); Serial.print(" | target angle: "); Serial.println(ANGLES[TARGET_NOTCH]);
    delay(100);
  }
}



/**************************************************************************
Below is a list of functions that are used in the code above.
**************************************************************************/


/*
This function is called only in the setup() loop. It will draw the lines on the right
half of the screen to denote "notches" of flaps. The number of lines drawn depends on 
how many notches are defined in the ANGLES[] array. The lines remain stationary and are 
used as a reference for the moving line (thus, reference lines)
*/
void drawReferenceLines() {
  int numberOfNotches = sizeof(ANGLES) / sizeof(ANGLES[0]);
  // int screenDistanceBetweenNotches = (SCREEN_WIDTH - (TITLE_MARGIN * 2)) / numberOfNotches;

  // iterate through the defined notch settings
  for (int i = 0; i < numberOfNotches; i++) {
    if (SERIAL_OUTPUT) { Serial.print(ANGLES[i]); Serial.print(":"); }
    // drawReferenceLine(ANGLES[i], i, screenDistanceBetweenNotches);
    drawReferenceLine(ANGLES[i], i);
  }
  if (SERIAL_OUTPUT) { Serial.println(); }
}

/*
This is the function that draws each individual line - called from the function
above - once for each element in the ANGLES[] array
*/
// void drawReferenceLine(int notch, int notchNumber, int distance) {
void drawReferenceLine(int angle, int notchNumber) {  
  int yPosition = getYPosition(angle);
  // int yPosition = (notchNumber * distance) + TITLE_MARGIN;
  display.fillRect(23, yPosition + TITLE_MARGIN, 13, 5, SSD1306_WHITE);
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
  //int screenDistanceBetweenNotches = (SCREEN_WIDTH - (TITLE_MARGIN * 2)) / NOTCH_COUNT;

  // iterate through the defined notch settings
  for (int i = 0; i < NOTCH_COUNT; i++) {
    int yPosition = getYPosition(ANGLES[i] + 3);
    drawReferenceValue(ANGLES[i], i);
  }

}

/*
This is the function that writes each individual angle value (e.g. a number) - called 
from the function above - once for each element in the ANGLES[] array
*/
void drawReferenceValue(int angle, int notchNumber) {
  // int yPosition = notchNumber * distance + TITLE_MARGIN;
  int yPosition = getYPosition(angle);
  
  char textValues[3];

  // add a space before the angle if it's less than 10
  if (angle < 10) {
    sprintf(textValues, " %d", angle);
  } else {
    sprintf(textValues, "%d", angle);
  }
  
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, yPosition + 3 );
  display.cp437(true);

  display.write(textValues);
  display.display();

  if (SERIAL_OUTPUT) { Serial.println(angle); }

}

/*
this function will remove any old lines depicting the current flap angle and 
draw a new line where it needs to be
*/
void drawNewAngle() {

  int angle = getCurrentSensorReading();
  int yPosition = getYPosition(angle); //map(angle, ANGLES[0], ANGLES[NOTCH_COUNT - 1], TITLE_HEIGHT, SCREEN_WIDTH);
  // yPosition = (yPosition / 2) * 2; // rounds the position to the nearest 2 - keeps it from jumping around too much
  
  display.fillRect(0, TITLE_HEIGHT, 22, SCREEN_WIDTH, SSD1306_BLACK); 
  display.fillRect(2, yPosition + TITLE_MARGIN - 1, 18, 5, SSD1306_WHITE);
  display.display();

}


/*
This function will determine the "y" position of the lines being drawn on the screen.
It uses a combination of the angle where you want to draw it and interpolates a pixel
number from that based on the number of available pixels on the screen. 

It uses the Arduino map() function to do that:
map(value, fromLow, fromHigh, toLow, toHigh) -> interpolated value
*/
int getYPosition(int angle) {
  
  int newValue = map(angle, ANGLES[0], ANGLES[NOTCH_COUNT - 1], TITLE_HEIGHT, SCREEN_WIDTH - TITLE_HEIGHT);
  
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
  int sensorReading = analogRead(A2);
  return map( sensorReading, FULL_RETRACT_VALUE, FULL_EXTEND_VALUE, ANGLES[0], ANGLES[NOTCH_COUNT - 1] );
}



/*
returns an integer value denoting the current "notch" based on the current angle read by the 
position sensor in the flap actuator
*/
int getCurrentNotch() {
  
  int currentNotch = 0;
  int SensorReading = getCurrentSensorReading();

  // handle case where SensorReading is less than the first angle in ANGLES[] (ANGLES[0])
  if (SensorReading < ANGLES[0]) {
    currentNotch = 0;
  }

  // Iterate through the array to find the notch
  for (int i = 0; i < NOTCH_COUNT - 1; i++) {
    if (SensorReading >= ANGLES[i] && SensorReading < ANGLES[i + 1]) {
      currentNotch = i; // Found the notch
    }
  }

  // Handle cases where the sensorAngle is greater than or equal to the last element
  if (SensorReading >= ANGLES[NOTCH_COUNT - 1]) {
    currentNotch = NOTCH_COUNT - 1; // Last notch
  }

  return currentNotch;

}


/*
returns the current notch - 1
*/
int getPreviousNotch() {
  // int previousNotch = NOTCH_COUNT - 1;
  int currentNotch = getCurrentNotch();
  int previousNotch = currentNotch;
  int currentSensorReading = getCurrentSensorReading();

  // check to see if the actuator is already at the first notch (based on ANGLES[] elements) ...
  if (CURRENT_NOTCH <= 0) {
    // ... if so, set it to 0 again
    previousNotch = 0;
  } else {
    // ... otherwise, find the previous notch and return the element id
    for (int i = NOTCH_COUNT - 1; i > 0; i--) {
      if (currentSensorReading >= ANGLES[i-1]) {
        previousNotch = i - 1;
        break; // we found what we're looking for and can exit the loop
      }
    }
  }
  
  return previousNotch;
}

/*
returns the CURRENT_NOTCH + 1
*/
int getNextNotch() {
  int currentNotch = getCurrentNotch();
  int nextNotch = currentNotch;
  int currentSensorReading = getCurrentSensorReading();

  // check to see if the actuator is already at the last notch (based on ANGLES[] elements) ...
  if (CURRENT_NOTCH >= NOTCH_COUNT) {
    // ... if so, set it to the last notch again
    nextNotch = NOTCH_COUNT - 1;
  } else {
    // ... otherwise, find the next notch and return the element id
    for (int i = 0; i < NOTCH_COUNT; i++) {
      if (currentSensorReading <= ANGLES[i+1] ) {
        nextNotch = i + 1;
        break; // we found what we're looking for and can exit the loop
      }
    }
  }
  
  return nextNotch;

}



