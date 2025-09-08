# Sonex Flap Control
This is an Arduino project to move flaps and display current flap angle on small OLED screen. 

My Sonex kit came with a Flap Actuator with a feedback circuit (e.g. it has 5 wires - red, black, blue, white, and yellow). This project assumes you wish to have better control over the flaps. It requires an H-type motor driver and a small OLED screen as well as two buttons or a single ON-OFF-ON three setting spring switch to select your preferred flap setting.

## Wires:

### Power for actuator: You can wire the actuator to directly use 12/24V. To extend the flaps, you give Positive power to the red wire and Negative power to black. To retract, flip the Positive/Negative connections
  RED:   Power (+ to extend, - to retract)
  BLACK: Power (- to extend, + to retract)

  You can simply wire it with a switch and be done (which allows you to just hold the extend or retract switch position until it's where you want it) or you can choose to use the Arduino in conjunction with a motor driver to move the actuator to various "notches" of flaps.

### Position sensor  
  BLUE: 5V positive
  WHITE: 5V negative/ground
  YELLOW: 0-5V Signal wire (read by analog pin A2 on Arduino)
