#include <Wire.h>

const byte JS_ADDR = 0x41;


// N35P112 Register definitions
//  NB- while there are other registers listed in the datasheet, these are the
//  only ones necessary for correct operation. The others are read-only or 
//  initialize to the right value and SHOULD NOT be messed with.

const byte JS_CTRL_REG_1 = 0x0F;  // 8-bit configuration register. (def: 11110000)
//  bit 7     1 = active mode, 0 = low power mode
//             EasyPoint MUST be in low power mode for INT on threshold mode!
//  bits 6:4  low power readout time base register. Wake-up interval for low
//             power mode output:
//             000 -> 20ms, 001 -> 40ms, 010 -> 80ms, 011 -> 100ms
//             100 -> 140ms, 101 -> 200ms, 110 -> 260ms, 111 -> 320ms
//  bit 3     1 = interrupt disabled, 0 = interrupt enabled
//  bit 2     0 = INT pin low on every new coordinate calculation
//            1 = INT pin low when threshold values exceeded and while
//                threshold values are exceeded
//  bit 1     write to 1 to reset device
//  bit 0     1 = conversion complete, data valid. Read only!

const byte JS_T_CTRL_REG = 0x2D;    // Scaling factor for X/Y coordinates.
//  Datasheet says to set this to 0x06 which is NOT the default power-up value.
//  No further information is provided, so make sure you write it after power on
//  or reset of the joystick!

const byte JS_ID_CODE_REG = 0x0C;   // 8-bit manufacturer ID code. Read only.
//  Reads as 0x0C- useful to test data communications with the device.

const byte JS_X_REG = 0x10;         // 8-bit register containing a signed 8-bit
                                   //  value representing the current position of
                                   //  the joystick in X.
                                   
const byte JS_Y_REG = 0x11;         // Y position register. Reading this register
                                   //  resets the status of the INT output, so it
                                   //  should be read AFTER the X value.
                                   
                           
// INT threshold registers. If the X or Y value is greater than the value in the
//  corresponding "P" register or less than the value in the "N" register, and 
//  JS_Control_Register_1 bit 2 = 1 and bit 3 = 0, and the device is set to poll
//  its read values automatically, 
const byte JS_XP_REG = 0x12;
const byte JS_XN_REG = 0x13;
const byte JS_YP_REG = 0x14;
const byte JS_YN_REG = 0x15;

// X and Y zero-state values. The EasyPointInit() function will populate these values
//  with the initial at-rest offset of the joystick. This can be used later to determine
//  the true offset from center when reading the value, as well as to calculate a
//  reasonable threshold for setting interrupt levels. The datatype "char" is used because
//  it is a signed 8-bit value, which is the same size as the X/Y values.
char XZero = 0;
char YZero = 0;

void setup() {
 Serial.begin(57600);
 Wire.begin();
 EasyPointInit();  // This is a nice, packaged function that sets up the EasyPoint.
 //EasyPointIntSetup(XZero, YZero, 25, 25); // Set up the interrupt function. Not
 EasyPointIntSetup(XZero, YZero, 15, 15);  //  needed, but nice to include! This is
                                           //  setting it to flag an interrupt if the
                                           //  reading is 25 points off center or more.
  //Serial.println("Setup completed successfully.");
  
  //Serial.print(readXAxis(), DEC); // readXAxis() returns a signed 8-bit result.
  //Serial.print(" ");
  //Serial.println(readYAxis(), DEC); // readYAxis() returns a signed 8-bit result.

}

void loop() {

}

// This function handles most of the EasyPoint specific initialization stuff-
//  registers that MUST be set to a certain value, calculation of zero-point
//  offsets, verification of connection.

void EasyPointInit()
{    
  // The datasheet tells us to initialize the T Control Register to 0x06 for this
  //  product but doesn't say why, nor why it doesn't power up to this value.
  //  Don't forget to do this!
  writeI2CReg(JS_ADDR, JS_T_CTRL_REG, 0x06);
  //readI2CReg(JS_ADDR, JS_T_CTRL_REG);
  
  // Now let's look at the X/Y offset. Ideally, the joystick would be at 0,0 at
  //  rest; practically, it could be as much as 30-50 counts positive or negative.
  char X, Y;                // char is the signed 8-bit data type in Arduino.
  char X_temp=0, Y_temp=0;
  readXAxis();  // read and discard a couple of values- the first read after boot
  readYAxis();  //  seems to be a little off on the low side.
  delay(1);
  // Now we collect four data points from each axis and calculate the average.
  //  We delay between conversions to allow a new conversion to complete- we COULD
  //  write some code to poll the conversion bit, or configure the INT pin, but
  //  let's be lazy instead.
  for (byte i = 0; i<4; i++)
  {
    X_temp += readXAxis();
    Y_temp += readYAxis();
    delay(1);
  }
  XZero = X_temp/4;
  YZero = Y_temp/4;
  //if (readI2CReg(JS_ADDR, JS_ID_CODE_REG) != 0x0C) {
    //Serial.println("EasyPoint failed to respond.");
    //}
}

// A simple function for setting the internal interrupt function to flag the
//  processor if the joystick moves more that x_delta or y_delta units from
//  the points specified by x_null and y_null. A function to set up an
//  asymmetrical dead zone is left as an exercise for the reader; the principle
//  is the same- write the appropriate registers, etc etc.

void EasyPointIntSetup(char x_null, char y_null, byte x_delta, byte y_delta)
{
char xp, xn, yp, yn;  // 8-bit signed values for the trigger points.
  xp = x_null + x_delta;  // calculate the positive trigger point for x
  writeI2CReg(JS_ADDR, JS_XP_REG, xp);  // set the positive trigger point for x
  xn = x_null - x_delta;  // calculate the negative trigger point for x
  writeI2CReg(JS_ADDR, JS_XN_REG, xn);  // set the negative trigger point for x
  yp = y_null + y_delta;  // calculate the positive trigger point for y
  writeI2CReg(JS_ADDR, JS_YP_REG, yp);  // set the positive trigger point for y
  yn = y_null - y_delta;  // calculate the negative trigger point for y
  writeI2CReg(JS_ADDR, JS_YN_REG, yn);  // set the negative trigger point for y
  // Now we need to actually tell the device we want to interrupt on values outside
  //  the deadzone, and to activate the interrupt pin. No assumptions- let's be
  //  responsible and change ONLY the bits we need. Start by reading the value
  //  of Control Register 1.
  byte temp = readI2CReg(JS_ADDR, JS_CTRL_REG_1);
  // Next, we'll clear bit 3, to ensure that the INT pin is active.
  temp = temp & 0b11110111;
  // Then, clear bit 7, to put the EasyPoint into self-timed mode (otherwise,
  //  it will only do a conversion after a read, which means you need to poll
  //  it to initiate a read, which makes the INT pin useless).
  temp = temp & 0b01111111;
  // Now, clear bits 6:4, so we can set the polling rate. See register description
  //  above for details on various values of these bits.
  temp = temp & 0b10001111;
  // Set the bits according to the desired polling rate. Here, I'll set it to
  //  poll every 200ms.
  temp = temp | 0b01010000;
  // Then, set bit 2, to tell the device to interrupt based on range and not
  //  every time a conversion is complete.
  temp = temp | 0b00000100;
      // NB- for clarity, I broke the bit sets/clears up into smaller operations. We
      //  COULD combine the bit clearing/setting operations into two lines, like this:
      //  temp = temp & 0b00000111;
      //  temp = temp | 0b01010100;
  // Finally, write the adjusted value back into Control Register 1.
  writeI2CReg(JS_ADDR, JS_CTRL_REG_1, temp);
}

// readXAxis() and readYAxis() are just nice little wrappers around the I2C read function which
//  automatically send the appropriate register addresses and convert the result from an unsigned
//  8-bit value to a signed 8-bit value.
char readXAxis() {
  return readI2CReg(JS_ADDR, JS_X_REG);
}

char readYAxis() {
  return readI2CReg(JS_ADDR, JS_Y_REG);
}

// The Wire library contains no discrete "read" or "write" functionality, so let's build our own.
//  I included an option to change the slave device's address but NOT the ability to send or
//  receive multiple bytes- generalizing that would have involved some unneccessarily ugly
//  programming for the simplicity of this application.
byte readI2CReg(byte slave_addr, byte reg_addr)
{
  // An I2C read transaction with the EasyPoint using the Wire library involves five steps:
  // Wire.beginTransmission() begins setting up a packet to transfer over I2C- the 7-bit
  //  address plus the R/W bit set to W mode.
  Wire.beginTransmission(slave_addr);
  // Wire.write() adds the 8-bit value passed to it to the packet. This can be repeated as
  //  necessary for multiple byte transfers; for the EasyPoint, the first byte transmitted
  //  is ALWAYS the address of interest.
  Wire.write(reg_addr);
  // Wire.endTransmission() transmits the packet that the previous two functions assembled.
  Wire.endTransmission();
  // Wire.requestFrom() pushes out the slave address followed by the R/W bit set to R mode.
  //  The result is that the device starts spitting out data based on (in this case) the
  //  current value in the address register. We need to clarify how many bytes we expect back;
  //  the (byte) typecast prevents an error in the compiler caused by an ambiguity in the
  //  expression of constants. It's dumb, but it works.
  Wire.requestFrom(slave_addr, (byte)1);
  // Wire.read() returns a byte read into the buffer by the Wire.requestFrom() function.
  //  Since we're only worried about reading one register at a time, we can simply return
  //  that value directly.
  return Wire.read();
}

void writeI2CReg(byte slave_addr, byte reg_addr, byte d_buff)
{
  // Writing the EasyPoint over I2C is a little easier than reading from it:
  // Wire.beginTransmission() starts assembling a packet for the device at slave_addr.
  Wire.beginTransmission(slave_addr);
  // Wire.write() adds data to the packet- the first piece of data is the address we
  //  of the register we want to write data into.
  Wire.write(reg_addr);
  // The second piece of data is the actual data we want to write.
  Wire.write(d_buff);
  // Wire.endTransmission() sends the packet. No return value is expected or needed for
  //  our purposes here.
  Wire.endTransmission();
}
