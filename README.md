# Sonex Flap Control
This is an experimental ameteur built (EAB) electronic project that was designed for flap movement and tracking project in my Sonex B model aircraft kit. It uses an Arduino Nano in conjunction with an BTS7960/IBT_2 H-type motor driver to move the Sonex-supplied Electric Flap Actuator (Sonex part SNB-C05-11) and displays current flap position on a small OLED screen. Feel free to use it for your own purposes AT YOUR OWN RISK. It is released under MIT open source agreement license (see the license agreement listed in the root of the project). 

<img src="https://photos.app.goo.gl/4ijbEMg8VTdL9vNT6" style="width:200px" />



# Parts/Supplies List
Here's a list of the parts I used while prototyping. You can probably find other vendors, but your mileage will likely vary. 
## Free-standing parts
### - Sonex Electric Flap Actuator
The Electric Flap Actuator requires a feedback circuit. Not all Sonex Electric Flap Actuators had the feedback circuit. To verify that yours has one, you will need to look at the two sets of wires coming out of the actuator. One of them will have a red and black wire - this is for 12-24V power. The other set of wires will have three leads - blue, white, and yellow. 
### - Arduino Nano (https://www.amazon.com/dp/B07G99NNXL)
### - BTS7960/IBT_2 H-Bridge DC Motor Driver (https://www.amazon.com/dp/B099N8XS5P)
### - .96 inch OLED I2C Display Module - 128x64 pixel (https://www.amazon.com/dp/B09T6SJBV5)
### - Marine-grade heavy duty 12V ON-OFF-ON momentary toggle switch (https://www.amazon.com/dp/B0F9P9L9G6)

## PCB-related Parts
Custom Printed Cirtuit Boards are required for this project. For the prototype, I used my ancient Canon laser printer to print a mirrored image on special paper, an iron to transfer the toner to a blank single-sided 
### - FR-4 Copper Clad PCB Laminate, Single Side, 4x2.7 inch (https://www.amazon.com/dp/B01MCVLDDZ)
### - Voltage Reducer/Buck Converter (set to 9V output) (https://www.amazon.com/dp/B08JZ5FVLC)
### - PCB Screw Terminal Blocks (https://www.amazon.com/dp/B014GMRBH2) 
### - Straight AND Right Angle 2.54mm Male Header Pins (https://www.amazon.com/dp/B0CN14TP2T)

### - 2x4 8P 2.54mm Straight Male Shrouded PCB Box Header IDC Socket (https://www.amazon.com/dp/B08DYFQKB1)
### - (pre-made) 2x4 Ribbon Cable (https://www.amazon.com/dp/B07DFBPZLJ) - these are pre-made. I preferred to make my own using the next two items listed below
### - (custom) 2x4 FC-8P 2.54mm Dual Rows IDC Sockets Female Connector for Flat Ribbon Cable (https://www.amazon.com/dp/B0BVHMTY5S)
### - (custom) Flat Ribbon Cable for 2.54mm 0.1" Spacing Connectors



# Wiring

## Power for actuator

You can wire the actuator to directly use 12/24V. To extend the flaps, you give Positive power to the red wire and Negative power to black. To retract, flip the Positive/Negative connections
  - RED:   Power (+ to extend, - to retract)
  - BLACK: Power (- to extend, + to retract)

  You can simply wire it with a switch and be done (which allows you to just hold the extend or retract switch position until it's where you want it) or you can choose to use the Arduino in conjunction with a motor driver to move the actuator to various "notches" of flaps.

  The Motor Driver will flip the polarity of the actuator wires to move the flaps in the correct direction.

## Position sensor  

The real benefit to this project is the flap position indicator. You can use the built-in position sensor to display the position of the flaps on your panel.

  - BLUE: 5V positive
  - WHITE: 5V negative/ground
  - YELLOW: 0-5V Signal wire (read by analog pin A2 on Arduino)
