#include <Wire.h>

const byte POTX_PIN = 56;
const byte POTY_PIN = 57;

const byte JS_ADDR = 0X41;

const byte JS_CTRL_REG_1 = 0x0F;
const byte JS_T_CTRL_REG = 0x2D;
const byte JS_ID_CODE_REG = 0x0C;
const byte JS_X_REG = 0x10;
const byte JS_Y_REG = 0x11;

int8_t PotXValue = 0;
int8_t PotYValue = 0;
char X = 0;
char Y = 0;

volatile byte i2c_reg = 0;

void setup()
{
  pinMode(POTX_PIN, INPUT);
  pinMode(POTY_PIN, INPUT);
  Serial.begin(57600);
  Wire.begin(JS_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void readPotValue(byte potentiometerPin, int8_t *pPreviousValue, char axe) {

  int PotValue = analogRead(potentiometerPin);
  
  if (axe == 'X'){ 
    PotValue= map(PotValue, 1023, 0, -128, 127);
  }
  else
  {
   PotValue= map(PotValue, 0, 1023, -128, 127);
  }

  if (*pPreviousValue != PotValue) {
    *pPreviousValue = PotValue;
  }
  return *pPreviousValue;
}

void loop() {
  delay(1);

  readPotValue(POTX_PIN, &PotXValue, X);
  readPotValue(POTY_PIN, &PotYValue, Y);
}

void receiveEvent(int numByte) {
  while(Wire.available()) {
    i2c_reg = Wire.read();
    //return i2c_reg;
  //Serial.print("receiveEvent i2c_reg = ");
  //Serial.println(i2c_reg, HEX);
   }
}

void requestEvent() {
  switch (i2c_reg) {
    case JS_T_CTRL_REG :
      Wire.write(JS_ID_CODE_REG);
      break;
    case JS_X_REG:
      Wire.write(PotXValue);
      break;
    case JS_Y_REG:
      Wire.write(PotYValue);
      break;
  }

  //Serial.print("requestEvent i2c_reg = ");
  //Serial.println(i2c_reg, HEX);
  /*Serial.print(PotXValue);
  Serial.print(" ");
  Serial.println(PotYValue);*/
}
