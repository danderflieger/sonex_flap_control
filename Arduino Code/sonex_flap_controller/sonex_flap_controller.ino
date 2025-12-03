/**************************************************************************
  MIT License

  Copyright (c) 2025 Dan Der Flieger

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**************************************************************************/


/**************************************************************************
  Include a few necessary libraries - the Adafruit ones need to be installed
  into the Arduino IDE before this will compile. See the copyright info 
  for the Adafruit libraries at the bottom of this file. Basically, just 
  consider the time and money expended to write a freely available library
  and think about buying their hardware (which is top-notch). 
**************************************************************************/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


/**************************************************************************
These two values represent the values recevied on the READING_PIN pin of the 
Arduino, which is connected to the yellow wire of the position sensor of the 
flap actuator. To calibrate your actuator, set the CALIBRATION_OUTPUT to 
true and find the sensor reading that appears when  you retract/extend the 
flap actuator to its limits in both directions. Use whatever value appears 
in the Serial Monitor to set the FULL_RETRACT_VALUE and FULL_EXTEND_VALUE 
settings here. Then turn off the CALIBRATION_OUTPUT by setting it to false.
No need to continue to output values to the Serial port when it's not
necessary. 
**************************************************************************/
#define FULL_RETRACT_VALUE  6
#define FULL_EXTEND_VALUE   880


/**************************************************************************
When a sensor reading is taken, because it is an analog sensor, sometimes
the number bounces around a little. For example, say the sensor is reading 
an average of 300. It will likely bounce between 299 and 301. Or maybe a 
range of 297 and 303. The value below (ANTI_FLICKER_VALUE) is used to 
attempt to round down by this many. So if it bounces between 300 and 301, 
it should report 300. 

If you see a lot of bounce, you may need to increase this number to, say, 
3 or 4 (whatever the range of the bounce is). When you do, you may need 
to adjust the two numbers above (FULL_RETRACT_VALUE and FULL_EXTEND_VALUE) 
to be divisible by whatever number you input here. Setting this to a lower 
number (2 by default) will make your reading more accurate, but there's a 
higher chance the bar indicating the current position of your flasp will 
"flicker" between two values. Setting it to a higher number reduces the 
liklihood of flicker, but may be a little less accurate. So weigh the 
importance of each when you set this up. 
**************************************************************************/
#define ANTI_FLICKER_VALUE 4


/**************************************************************************
 Decide whether or not to output serial data and/or calibration data over
 the serial port (e.g. through the Arduino IDE's Serial Monitor). The 
 SERIAL_OUTPUT switch will output lots of (possibly) useful data. 
 
 If CALIBRATION_OUTPUT is set to true, it will output readings so you can 
 adjust the two values above to match the flap acutator's readings when
 it's extended/retracted fully. 
 
 It's recommended that you set the FULL_RETRACT_VALUE to 0 and the 
 FULL_EXTEND_VALUE value to 1023 to start. Then set the CALIBRATION_OUTPUT 
 value to true and retract the actuator all the way.

 The output in the Serial Monitor will tell you what to put into the 
 FULL_RETRACT_VALUE. Then extend it all the way to figure out what the 
 FULL_EXTEND_VALUE should be set to. 
**************************************************************************/
#define SERIAL_OUTPUT       false
#define CALIBRATION_OUTPUT  true


/*************************************************************************
  These values are used to determine how long the system waits with no movement 
  before it stops trying to move the flaps. For example, when the flaps reach 
  full retraction or extention values, but haven't quite met the FULL_RETRACT_VALUE 
  or FULL_EXTEND_VALUE threshold - the Arduino will continue to send PWM values to
  the motor driver. However, there is a safety switch in the actuator that 
  cuts power to the actuator motor. So the Arduino will sit in a state where
  it's attempting to move the actuator, but it's not moving. These values will
  only allow the PWM signals to continue without movement for a certain number
  of milliseconds (default = 500 or half a second) before changing to DIRECTION_STOP.
*************************************************************************/
#define MAX_IDLE_TIME 500


/*************************************************************************
  This value can be adjusted to allow the actuator to stop slightly before
  it reaches a specific notch. It helps to ensure the actuator doesn't 
  go past the target as it slows - e.g., the Arduino will tell the 
  actuator to stop moving slightly before it reaches the target setting.
  Increase this number to stop it sooner. Decrease it to stop it later. 
  Note that this number MUST be positive - negative numbers don't work 
  properly.
*************************************************************************/
#define TARGET_TOLERANCE 5


/**************************************************************************
This is where you define your various flap notches. You can pretty much
put any integer values you want to appear on the screen - BUT THEY MUST
BE IN NUMERICAL ORDER!! 

Ideally you will do some testing to figure out what angles your flaps actually
sit at for each notch. The first and last element in this array should be 0 and 
whatever full flaps are deflected to. Sonex says full flaps on the B model is 
30 deg. Because they suggest 0 for clean, 10 for takeoff, and 30 for landing, 
that's what I've put here:

int ANGLES[] = { 0, 10, 30 };

Feel free to add additional "notches" in there if you think it's necessary. 
If you wanted to add a notch for 20 deg (for example), it would look like this:

int ANGLES[] = { 0, 10, 20, 30 };
***************************************************************************/ 
int ANGLES[] = { 0, 10, 30 };


/**************************************************************************
 This figures out the number of notches for later use - nothing to change 
 here. It will figure out how many notches you defined based on the ANGLES[] 
 values you entered above. 
**************************************************************************/
#define NOTCH_COUNT sizeof(ANGLES) / sizeof(ANGLES[0])


/**************************************************************************
  Declare another array with similar values. The difference here is that this
  array will hold actual sensor readings, not angles. Because sensor readings 
  have a larger range (e.g. 0-1023 instead of 0-30 degrees), we can start 
  and stop the actuator with better precision. I had an issue where I would 
  target, say, 20 degrees of flap, but the actuator would "coast" to 21 or
  22 degrees before it stopped. And when retracting, it would go to 19 or 18
  degrees before stopping. This is meant to fix that by triggering it to stop
  fractionally sooner, but I needed better precision than what ANGLES can 
  provide. 
**************************************************************************/
int SENSOR_READINGS[NOTCH_COUNT];

/************************************************************************** 
Declare and initialize (to 0) a few variables that are used in the program
***************************************************************************/ 
int           CURRENT_NOTCH         = 0;
int           TARGET_NOTCH          = 0;
int           LAST_SENSOR_READING   = 0;
unsigned long LAST_MOVEMENT_MILLIS  = 0;


/***************************************************************************
 a couple of pre-defined values (enums) that determine which way the flaps 
 should move (or not)
***************************************************************************/ 
#define DIRECTION_RETRACT -1
#define DIRECTION_STOP 0
#define DIRECTION_EXTEND 1


/***************************************************************************
 Define a public variable to define the current direction of the actuator - 
 0:DIRECTION_STOP, 1: DIRECTION_EXTEND, -1: DIRECTION_RETRACT
 This value will be used to determine if the actuator should be moving in one direction or
 the other (or stop) between button presses until it has reached the specified notch
***************************************************************************/ 
int MOTION_DIRECTION = DIRECTION_STOP;


/***************************************************************************
 Define some values for the OLED screen and utilize the Adafruit SSD1306 
 OLED library to configure the OLED screen
***************************************************************************/
#define TITLE_MARGIN    8
#define TITLE_HEIGHT    20
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/***************************************************************************
 define pin numbers for reading and buttons
***************************************************************************/
#define READING_PIN     A1
#define RETRACT_BUTTON  A2
#define EXTEND_BUTTON   A3


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
#define EXTEND_PWM_PIN  11
#define ACTUATOR_SPEED  255

/***************************************************************************
  run the setup() function. This section runs one time only, when the Arduino 
  is powered on
***************************************************************************/ 
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


  /**************************************************************************
   Next, use the drawTitle() function below to put a bar at the top of the 
   screen with text that says "Flaps."  
   
   Then draw lines where the notches should be and finally draw numbers next
   to those lines - the lines and numbers will change depending on your settings 
   in the ANGLES[] array above
  **************************************************************************/
  drawTitle();
  drawReferenceLines();
  drawReferenceValues();


  /**************************************************************************
    Fill the SENSOR_READINGS array using values defined in the ANGLES array - this will set
    "notches" based on sensor readings (0-1023) instead of ANGLES (0-30, for example), which
    allows for better precision when moving the actuator.

    To do this, we use a "for" loop to iterate through our ANGLES array and 
    change each value to a corresponding SENSOR_READINGS value.
  **************************************************************************/
  for (int i = 0; i<NOTCH_COUNT; i++) {
    SENSOR_READINGS[i] = map(ANGLES[i],ANGLES[0],ANGLES[NOTCH_COUNT - 1],FULL_RETRACT_VALUE, FULL_EXTEND_VALUE);
  }


  /**************************************************************************
    Get the current sensor reading to determine where the flaps are currently 
    positioned
  **************************************************************************/
  int SensorReading = getCurrentSensorReading(); 


  /**************************************************************************
    Find the current notch - the getCurrentNotch() function goes through the list of notches 
    to see if the flaps are set at a particular notch using the SensorReading we just grabbed
  **************************************************************************/
  CURRENT_NOTCH = getCurrentNotch();
  if (SERIAL_OUTPUT) { Serial.print("Beginning CURRENT_NOTCH:");Serial.println(CURRENT_NOTCH); delay(1000); }

}


/**************************************************************************
 Run the loop() function - this function repeats over and over indefintely
 after the setup() function runs
**************************************************************************/
void loop() {
  
  /*****************************************************************************
    Instantiate a couple of variables - get the value currently being published
    by the actuator (should be a number between 0 and 1023). Then figure out how
    many milliseconds the Arduino has been turned on. Should be something small 
    like 100 milliseconds since this portion of code only runs when it's first
    powered on. This will be used to detect when the actuator has been idle for 
    some amount of time so the arduino will stop trying to move it - usually 
    means it has reached its fully extended or reatracted position.
  *****************************************************************************/
  int currentSensorReading = getCurrentSensorReading();
  unsigned long currentMillis = millis();
  
  
  /*****************************************************************************
    Run the getCurrentNotch() function, which figures out which "notch" the 
    flaps are set to when powered on.

    In this context, a "notch" is an array element of the ANGLES[] array. For 
    example, suppose you have ANGLES[] defined as 0, 10, 20, 30 and you have
    defined your sensor limits (FULL_RETRACT_VALUE and FULL_EXTEND_VALUE) as 0 and 
    1023, respectively. The "notches" will be as follows (note the interpolated
    SENSOR_READINGS values that are defined using the map() funcion a few lines 
    above):

    | Notch | ANGLES  | SENSOR_READINGS |
    |-------|---------|-----------------|
    | 0     | 0       | 0               |
    | 1     | 10      | 341             |
    | 2     | 20      | 682             |
    | 3     | 30      | 1023            |

    With that information, we'll now use the getCurrentNotch() function, which
    will return the nearest notch that the flaps are currently set to, rounded
    down. For example, if the flaps currently are set to 23 degrees, the 
    getCurrentNotch() function will return the number 2 (third notch).

    We assign the notch number to a variable called CURRENT_NOTCH for later use.
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

    if (SERIAL_OUTPUT) { Serial.print("Button: Retract "); }
    
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
        will begin to retract. If the pilot holds the button for more than 200 milliseconds
        (notice the delay(200) line below), the flaps should stop and then start to retract
        to the next more retracted notch (e.g. from notch 2 to notch 1).
      *****************************************************************************/ 

      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      if (SERIAL_OUTPUT) { Serial.print("\tLine 421 "); }
      delay(MAX_IDLE_TIME); 

    } else {

      /*****************************************************************************
        if the MOTION_DIRECTION variable is NOT currently set to DIRECTION_EXTEND, 
        (e.g. either currently set to stop or retract), the flaps will be set to 
        start/continue to retract.
      *****************************************************************************/ 

      MOTION_DIRECTION = DIRECTION_RETRACT;

      if (currentSensorReading >= SENSOR_READINGS[getPreviousNotch()] + TARGET_TOLERANCE){

        /*****************************************************************************
          the TARGET_NOTCH variable will be used to determine if the flaps have reached
          their desired position
        ****************************************************************************/
        TARGET_NOTCH = getPreviousNotch();

      }
    }
  } else if (digitalRead(EXTEND_BUTTON) == LOW) {

    if (SERIAL_OUTPUT) { Serial.print("Button: Extend "); }

    /****************************************************************************
      If the EXTEND_BUTTON is pressed, do the opposite of what was done if the 
      RETRACT_BUTTON is pressed - e.g. stop if the MOTION_DIRECTION variable is 
      set to retract. Otherwise, move to (or continue moving) toward the next 
      notch. 
    ****************************************************************************/
    
    if (MOTION_DIRECTION == DIRECTION_RETRACT) {
    
      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      delay(MAX_IDLE_TIME);
    
    } else {
    
      MOTION_DIRECTION = DIRECTION_EXTEND;
      
      if (currentSensorReading <= SENSOR_READINGS[getNextNotch()]) {
        TARGET_NOTCH = getNextNotch();
      }
    
    }
  } else {
    if (SERIAL_OUTPUT) { Serial.print("Button: None "); }
  }



  /****************************************************************************
    Now that we know what the pilot WANTS the flaps to do, time to actually do 
    something ...

    Use the MOTION_DIRECTION variable (set above) to determine what to do
  ****************************************************************************/
  
  if (MOTION_DIRECTION == DIRECTION_RETRACT) {
    
    /****************************************************************************
      Ok, so the pilot asked for the flaps to be retracted, so start/continue 
      retracting them until the previous notch has been reached. 
      
      Take a look at the TARGET_NOTCH element of the ANGLES array and compare that
      to the current reading from the flap actuator. 

      Remember, the ANGLES array might look something like this:
      
      | Notch | ANGLES  | SENSOR_READINGS |
      |-------|---------|-----------------|
      | 0     | 0       | 0               |
      | 1     | 10      | 341             |
      | 2     | 20      | 682             |
      | 3     | 30      | 1023            |
      
      So if the current position of the flaps comes back as 28 (that is returned by 
      the getCurrentSensorReading() function) and the TARGET_NOTCH is currently set to
      the next lower notch (in this example, 2) - the motor controller will set the
      extend pin to 0 (not extending) and the retract pin to whatever ACTUATOR_SPEED 
      is set to. 255 would be full speed. ACTUATOR_SPEED is set near the top of this file.

      These lines would tell the motor controller to stop. Both the extend and retract PWM pins
      are set to 0:
      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, 0);

      These lines would tell the motor controller to retract (0 speed for extend, full
      speed retract):
      
      analogWrite (EXTEND_PWM_PIN, 0);
      analogWrite (RETRACT_PWM_PIN, ACTUATOR_SPEED);

      *** VERY IMPORTANT ***
      The lines below will tell the motor controller to extend (notice that the RETRACT and 
      EXTEND lines are reversed from the example directly above. You want make sure you 
      STOP the opposite direction BEFORE setting the direction to move - otherwise there 
      may be a brief moment where the actuator is trying to both extend AND retract at 
      the same time - this would be a short circuit since the motor controller is effectively
      forcing the power connection on the actuator to reverse polarity - 
      positive to negative/negative to positive). So always STOP in one direction before
      STARTING in the opposite direction:

      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, ACTUATOR_SPEED);

      OK, all that out of the way, let's tell the flaps to move or stop in the correct direction:
    ****************************************************************************/
    
    if (currentSensorReading >= SENSOR_READINGS[TARGET_NOTCH]  + TARGET_TOLERANCE) {
      
      /**************************************************************************
        Looks like the flaps are currently more extended than we want, so RETRACT 
        them
      **************************************************************************/
      analogWrite (EXTEND_PWM_PIN, 0);
      analogWrite (RETRACT_PWM_PIN, ACTUATOR_SPEED);

    } else {

      /**************************************************************************
        Looks like the flaps have retracted to where we want them, so STOP!
      **************************************************************************/
      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;
      
    }
    
  } else if (MOTION_DIRECTION == DIRECTION_EXTEND) {

    /**************************************************************************
      We've been given the signal to EXTEND the flaps
    ***************************************************************************/

    if (currentSensorReading <= SENSOR_READINGS[TARGET_NOTCH]) {

      /**************************************************************************
        Looks like the flaps are more retracted than we want, so EXTEND them
      ***************************************************************************/
      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, ACTUATOR_SPEED);

    } else {

      /**************************************************************************
        Looks like the flaps are extended to the right place, so STOP!
      ***************************************************************************/
      analogWrite (EXTEND_PWM_PIN, 0);
      analogWrite (RETRACT_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;

    }
  } else {

    /**************************************************************************
      If we don't want to either extend or retract the flaps, the only other option
      is to stop them. 
    ***************************************************************************/
    analogWrite (EXTEND_PWM_PIN, 0);
    analogWrite (RETRACT_PWM_PIN, 0);
    MOTION_DIRECTION = DIRECTION_STOP;

  }


  /**************************************************************************
    Now, let's update the screen by drawing a little line to indicate where 
    the flaps are currently extended to.  To do this, we use a function called 
    drawNewAngle() - which is defined toward the bottom of this file.
  ***************************************************************************/
  drawNewAngle();
  

  /**************************************************************************
    Output some data to the Serial Monitor if you have the SERIAL_OUTPUT
    and/or CALIBRATION_OUTPUT values set to true - otherwise don't output
  ***************************************************************************/
  if (SERIAL_OUTPUT) { 

    Serial.print(" CURRENT_NOTCH:");       Serial.print(CURRENT_NOTCH);
    Serial.print(" TARGET_NOTCH:");        Serial.print(TARGET_NOTCH);
    Serial.print(" ANGLES[TARGET_NOTCH]:");Serial.print(ANGLES[TARGET_NOTCH]);
    Serial.print(" currentSensorReading:");Serial.print(currentSensorReading); 
    Serial.print(" target angle:");        Serial.print(ANGLES[TARGET_NOTCH]);
  }


  /**************************************************************************
    Grab the current sensor reading to see actuator's position
  **************************************************************************/
  currentSensorReading = getCurrentSensorReading(); 
 

  /*************************************************************************
  Display the current Sensor Reading for calibration purposes
  /************************************************************************/
  if (CALIBRATION_OUTPUT) {
  
    Serial.print(" Sensor Reading:");Serial.println(currentSensorReading); //Serial.println(analogRead(READING_PIN));
  
  }


  /**************************************************************************
    And, finally, check to see if the actuator has reached its limit (and 
    therefore no longer  moving) for more than the allotted MAX_IDLE_TIME 
    threshold - if so, stop all PWM signals
  **************************************************************************/ 
  if (SERIAL_OUTPUT) { Serial.print (" In motion:");}
  if (currentSensorReading != LAST_SENSOR_READING) {

    /**************************************************************************
      It looks like the sensor has moved since the last time we checked, so
      reset the clock and change the LAST_SENSOR_READING to the current one
      in preparation for this same check to be done the next time around
    **************************************************************************/
    LAST_MOVEMENT_MILLIS = millis();
    LAST_SENSOR_READING = currentSensorReading;
    
    // output that the actuator IS in motion 
    if (SERIAL_OUTPUT) { Serial.print("true");}
    
  } else {
    
    // output that the actuator is NOT in motion 
    if (SERIAL_OUTPUT) { Serial.print("false");}

    /**************************************************************************
      The current and last sensor readings are the same - so check to see if 
      they've been the same for the MAX_IDLE_TIME.
    **************************************************************************/
    if (currentMillis - LAST_MOVEMENT_MILLIS > MAX_IDLE_TIME) {
      
      /**************************************************************************
        The MAX_IDLE_TIME threshold has been reached, so stop trying to move
        the actuator and set the MOTION_DIRECTION variable to 0 (DIRECTION_STOP)
      **************************************************************************/  
      
      analogWrite (RETRACT_PWM_PIN, 0);
      analogWrite (EXTEND_PWM_PIN, 0);
      MOTION_DIRECTION = DIRECTION_STOP;

      LAST_MOVEMENT_MILLIS = currentMillis;
      if (SERIAL_OUTPUT) { Serial.print("\n\n*** Time out reached ***\n\n"); }

    } 

  }


  /**************************************************************************
  Output some data to the Serial Monitor if you have the SERIAL_OUTPUT
  and/or CALIBRATION_OUTPUT values set to true - otherwise don't output
  ***************************************************************************/ 
  if (SERIAL_OUTPUT) { 
    Serial.print(" LAST_SENSOR_READING:");  Serial.print(LAST_SENSOR_READING);  Serial.print(":"); Serial.print(currentSensorReading);
    Serial.print(" LAST_MOVEMENT_MILLIS");  Serial.print(LAST_MOVEMENT_MILLIS); Serial.print(":"); Serial.print(currentMillis);
    Serial.print(" TARGET_NOTCH:");         Serial.print(TARGET_NOTCH);         
    Serial.print(" CURRENT_NOTCH:"); Serial.print(CURRENT_NOTCH);
    Serial.print(" SENSOR_READINGS:");
    for (int i=0; i<NOTCH_COUNT; i++) {
      Serial.print(SENSOR_READINGS[i]);
      Serial.print(",");
    }
    Serial.print(" ANGLES:");
    for (int i=0; i<NOTCH_COUNT; i++) {
      Serial.print(ANGLES[i]);
      Serial.print(" ");
    }
    
  }

  /**************************************************************************
    Set the LAST_SENSOR_READING to whatever the currentSensorReading is -
    if the value doesn't change for whatever time threshold you set the
    MAX_IDLE_TIME value to, it will STOP the actuator. 
  ***************************************************************************/
  LAST_SENSOR_READING = currentSensorReading;
  
  Serial.println();
}



/**************************************************************************
  Below is a list of functions that are used in the code above.
**************************************************************************/


/**************************************************************************
  This function is called only in the setup() loop. It will draw the lines on the right
  half of the screen to denote "notches" of flaps. The number of lines drawn depends on 
  how many notches are defined in the ANGLES[] array. The lines remain stationary and are 
  used as a reference for the moving line (thus, reference lines)
**************************************************************************/
void drawReferenceLines() {

  // iterate through the defined notch settings
  for (int i = 0; i < NOTCH_COUNT; i++) {
    if (SERIAL_OUTPUT) { Serial.print(ANGLES[i]); Serial.print(":"); }
    drawReferenceLine(ANGLES[i], i);
  }
  
  if (SERIAL_OUTPUT) { Serial.println(); }

}


/**************************************************************************
  This is the function that draws each individual line - called from a loop
  in the function above - once for each element in the ANGLES[] array
**************************************************************************/
void drawReferenceLine(int angle, int notchNumber) {  

  int yPosition = getYPosition(angle);
  display.fillRect(23, yPosition + TITLE_MARGIN, 13, 5, SSD1306_WHITE);
  display.display();

}


/**************************************************************************
  This function puts a small title at the top of the screen that says "FLAPS"
**************************************************************************/
void drawTitle() {

  char textValues[] = "FLAPS";

  display.setTextSize(2);

  // add a white background rectangle
  display.fillRect(0, 0, SCREEN_HEIGHT, 17, SSD1306_WHITE);
  
  // write on the white rectangle with black text (and outlined with white)
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(3, 1);
  display.cp437(true);
  display.write(textValues);
  display.display();

}

/**************************************************************************
  This function writes text to the right of each reference line to show the pilot what each
  notch of flaps should represent (e.g. in angles) - defined in the ANGLES[] array
**************************************************************************/
void drawReferenceValues() {

  // iterate through the defined notch settings
  for (int i = 0; i < NOTCH_COUNT; i++) {
    int yPosition = getYPosition(ANGLES[i] + 3);
    drawReferenceValue(ANGLES[i], i);
  }

}

/**************************************************************************
  This is the function that writes each individual angle value (e.g. a number) - called 
  from the function above - once for each element in the ANGLES[] array
**************************************************************************/
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

}

/**************************************************************************
  this function will remove any old lines depicting the current flap angle
  and draw a new line where it needs to be - this is the moving line
  indicator
**************************************************************************/
void drawNewAngle() {

  int angle = getCurrentAngle();
  int yPosition = getYPosition(angle);
  
  display.fillRect(0, TITLE_HEIGHT + TITLE_MARGIN, 22, SCREEN_WIDTH, SSD1306_BLACK); 
  display.fillRect(2, yPosition + TITLE_MARGIN, 18, 5, SSD1306_WHITE);
  display.display();

}


/**************************************************************************
  This function will determine the "y" position of the lines being drawn on 
  the screen. It uses a combination of the angle where you want to draw it 
  and interpolates a pixel number from that based on the number of available
  pixels on the screen. 

  It uses the Arduino map() function to do that:
  map(input value, fromLow, fromHigh, toLow, toHigh) -> interpolated value
**************************************************************************/
int getYPosition(int angle) {
  
  int newValue = map(angle, ANGLES[0], ANGLES[NOTCH_COUNT - 1], TITLE_HEIGHT, SCREEN_WIDTH - TITLE_HEIGHT);
  
  return newValue;
  
}

/**************************************************************************
  reads the READING_PIN (A0 by default) on the Arduino (connected to the 
  position output wire on the actuator) to get a value from 0-1023. 

  The sensor wires on the Sonex actuator work like this:
  - +5V on the BLUE wire
  - GND on the WHITE wire
  - voltage output (between 0 and 5V on the YELLOW wire, depending on the 
    position of the actuator - 0 = fully retracted, 5V = fully extended)

  The yellow wire is connected to the Analog 0 (A0) pin on the Arduino. 
  The Arduino converts that 0-5V voltage to a value between 0 and 1023, 
  respectively, on that pin. Using the map() function mentioned above, we
  can interpolate between fully retracted and fully extended values and
  output what those values are in comparison to values in the ANGLES[] array. 
**************************************************************************/
int getCurrentSensorReading() {
  int sensorReading = analogRead(READING_PIN);

  // round down to the nearest 2 to keep the screen from flickering
  sensorReading = sensorReading - (sensorReading % ANTI_FLICKER_VALUE);
  
  return sensorReading;
}


/**************************************************************************
  Converts the current sensor reading to an angle as defined in the 
  ANGLES array. It gets the sensor reading with the function above
  and interpolates that value to an "angle" based on the lowest and highest
  values in the ANGLES array. 
**************************************************************************/
int getCurrentAngle() {
  int angle = map(getCurrentSensorReading(), FULL_RETRACT_VALUE, FULL_EXTEND_VALUE, ANGLES[0], ANGLES[NOTCH_COUNT - 1] );
  return angle;
}



/**************************************************************************
  Returns an integer value denoting the current "notch" based on the current 
  angle read by the position sensor in the flap actuator
**************************************************************************/
int getCurrentNotch() {
  
  int currentNotch = 0;
  int currentSensorReading = getCurrentSensorReading();

  
  if (currentSensorReading < SENSOR_READINGS[0]) {

    /**************************************************************************
      handle case where SensorReading is less than the first angle in ANGLES[]
      (ANGLES[0]). If the current notch is a negative number, set it to 0 so
      we don't have a buffer overrun
    **************************************************************************/
    currentNotch = 0;
  
  } else if (currentSensorReading >= SENSOR_READINGS[NOTCH_COUNT - 1]) {
    
    /**************************************************************************
      Handle cases where the sensorAngle is greater than or equal to the last
      element - If the ANGLES array only has 3 notches, and you are somehow 
      reading notch 4, just set it to whatever the highest notch number is
      to avoid a buffer overrun
    **************************************************************************/
    currentNotch = NOTCH_COUNT - 1; // Last notch

  } else {

    /**************************************************************************
      Looks like the current notch is within the boundaries of the ANGLES array,
      so let's iterate through the array to figure out what notch the 
      actuator is currently sitting in. 
    **************************************************************************/
    for (int i = 0; i < NOTCH_COUNT - 1; i++) {
      // if (currentSensorReading >= ANGLES[i] && currentSensorReading < ANGLES[i + 1]) {
      if (currentSensorReading >= SENSOR_READINGS[i] && currentSensorReading < SENSOR_READINGS[i + 1]) {
        currentNotch = i; // Found the notch
      }
    }
  }
  
  return currentNotch;

}


/**************************************************************************
  returns the current notch - 1
**************************************************************************/
int getPreviousNotch() {
  
  int currentNotch = getCurrentNotch();
  int previousNotch = currentNotch;
  int currentSensorReading = getCurrentSensorReading();

  
  if (currentNotch <= 0) {

    /**************************************************************************
      check to see if the actuator is already at the first notch (based on 
      ANGLES[] elements) ... if so, set it to 0 again
    **************************************************************************/
    previousNotch = 0;

  }  else if (currentSensorReading >= SENSOR_READINGS[NOTCH_COUNT - 1]) {

    /**************************************************************************
     Handle cases where the sensorAngle is greater than or equal to the last 
     element ... if so, use the highest notch, minus 1
    **************************************************************************/
    previousNotch = (NOTCH_COUNT - 1) - 1; 

  } else {

    /**************************************************************************
      Looks like were within bounds, so iterate through the various notches to
      find the PREVIOUS one
    **************************************************************************/
    for (int i = NOTCH_COUNT - 1; i >= 0; i--) {
      
      // if (currentSensorReading  <= SENSOR_READINGS[i] && currentSensorReading >= SENSOR_READINGS[i-1]) {
      if (currentSensorReading >= SENSOR_READINGS[i-1] + TARGET_TOLERANCE) {
        previousNotch = i - 1;

        break; // we found what we're looking for and can exit the loop
      }

    }

  }
  
  return previousNotch;
}

/**************************************************************************
  returns the CURRENT_NOTCH + 1
**************************************************************************/
int getNextNotch() {
  int currentNotch = getCurrentNotch();
  int nextNotch = currentNotch;
  int currentSensorReading = getCurrentSensorReading();

  if (currentNotch >= NOTCH_COUNT) {

    /**************************************************************************
     Handle cases where the sensorAngle is greater than or equal to the last 
     element ... if so, use the highest notch 
    **************************************************************************/
    nextNotch = NOTCH_COUNT - 1;

  } else if(currentSensorReading <= SENSOR_READINGS[0]) {

    /**************************************************************************
      check to see if the actuator is already at the first notch (based on 
      ANGLES[] elements) ... if so, use the 2nd notch (element 1)
    **************************************************************************/
    nextNotch = 1; 
    
  } else {
    
    /**************************************************************************
      Looks like were within bounds, so iterate through the various notches to
      find the NEXT one
    **************************************************************************/
    for (int i = 0; i < NOTCH_COUNT; i++) {
      
      // if (currentSensorReading >= SENSOR_READINGS[i] - TARGET_TOLERANCE && currentSensorReading < SENSOR_READINGS[i+1]) {
      if ( currentSensorReading <= SENSOR_READINGS[i+1] - TARGET_TOLERANCE) {
        nextNotch = i + 1;
        break; // we found what we're looking for and can exit the loop
      }

    }

  }

  return nextNotch;

}

/**************************************************************************
 This code uses a driver written by Adafruit for the SSD1306 OLED screen. 
 Here is the text required for use with that driver:
 
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/