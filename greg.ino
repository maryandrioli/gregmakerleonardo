/////////////////////////
// DEBUG DEFINITIONS ////
/////////////////////////
//#define DEBUG
//#define DEBUG2
//#define DEBUG3
//#define DEBUG_TIMING
//#define DEBUG_MOUSE
//#define DEBUG_TIMING2
/**/
////////////////////////
// DEFINED CONSTANTS////
////////////////////////

///////////////////////////
// NOISE CANCELLATION /////
///////////////////////////
#define SWITCH_THRESHOLD_OFFSET_PERC  18   // number between 1 and 49
                                           // larger value protects better against noise oscillations, but makes it harder to press and release
                                           // recommended values are between 2 and 20
                                           // default value is 5

#define SWITCH_THRESHOLD_CENTER_BIAS 65   // number between 1 and 99
                                          // larger value makes it easier to "release" keys, but harder to "press"
                                          // smaller value makes it easier to "press" keys, but harder to "release"
                                          // recommended values are between 30 and 70
                                          // 50 is "middle" 2.5 volt center
                                          // default value is 55
                                          // 100 = 5V (never use this high)
                                          // 0 = 0 V (never use this low
                                          

/////////////////////////
// MOUSE MOTION /////////
/////////////////////////
#define MOUSE_MOTION_UPDATE_INTERVAL  35   // how many loops to wait between 
                                           // sending mouse motion updates
                                           
#define PIXELS_PER_MOUSE_STEP         4     // a larger number will make the mouse
                                           // move faster

#define MOUSE_RAMP_SCALE              150  // Scaling factor for mouse movement ramping
                                           // Lower = more sensitive mouse movement
                                           // Higher = slower ramping of speed
                                           // 0 = Ramping off
                                            
#define MOUSE_MAX_PIXELS              10   // Max pixels per step for mouse movement

#define BUFFER_LENGTH    3     // 3 bytes gives us 24 samples
#define NUM_INPUTS       17    // 6 on the front + 12 on the back     -> 1 a menos (virou led) (-5)
//#define TARGET_LOOP_TIME 694   // (1/60 seconds) / 24 samples = 694 microseconds per sample
//#define TARGET_LOOP_TIME 758  // (1/55 seconds) / 24 samples = 758 microseconds per sample
#define TARGET_LOOP_TIME 744  // (1/56 seconds) / 24 samples = 744 microseconds per sample 

// id numbers for mouse movement inputs (used in settings.h)
#define MOUSE_MOVE_UP       -1
#define MOUSE_MOVE_DOWN     -2
#define MOUSE_MOVE_LEFT     -3
#define MOUSE_MOVE_RIGHT    -4

#if (ARDUINO > 10605)
#include <Keyboard.h>
#include <Mouse.h>
#endif
#include "settings.h"

/////////////////////////
// STRUCT ///////////////
/////////////////////////
typedef struct {
  byte pinNumber;
  int keyCode;
  byte measurementBuffer[BUFFER_LENGTH];
  boolean oldestMeasurement;
  byte bufferSum;
  boolean pressed;
  boolean prevPressed;
  boolean isMouseMotion;
  boolean isMouseLeft;
  boolean isMouseRight;
  boolean isKey;
}
GregInput;

GregInput inputs[NUM_INPUTS];

///////////////////////////////////
// VARIABLES //////////////////////
///////////////////////////////////
int bufferIndex = 0;
byte byteCounter = 0;
byte bitCounter = 0;
int mouseMovementCounter = 0; // for sending mouse movement events at a slower interval

int pressThreshold;
int releaseThreshold;
boolean inputChanged;

int mouseHoldCount[NUM_INPUTS]; // used to store mouse movement hold data

// Pin Numbers
// input pin numbers for kickstarter production board


///TESTE
int pinNumbers[NUM_INPUTS] = {
  12, 8, 13, 7, 5, 6,
  4, 3, 2, 1, 0,
  A5, A4, A1, A0, A2, A3
};

/*
int pinNumbers[NUM_INPUTS] = {
  12, 8, 13, 15, 7,
  6, 5, 3, 2, 1,
  0, 23, 22, 21, 20,
  19, 18
};
*/

// input status LED pin numbers
const int inputLED_a = 9;
const int inputLED_b = 10;
const int inputLED_c = 11;
const int outputK = 30;
const int outputM = 17;
const int outputD4 = 4;
byte ledCycleCounter = 0;

// timing
int loopTime = 0;
int prevTime = 0;
int loopCounter = 0;


///////////////////////////
// FUNCTIONS //////////////
///////////////////////////
void initializeArduino();
void initializeInputs();
void updateMeasurementBuffers();
void updateBufferSums();
void updateBufferIndex();
void updateInputStates();
void sendMouseButtonEvents();
void sendMouseMovementEvents();
void addDelay();
void cycleLEDs();
void danceLeds();
void updateOutLEDs();
void safetyReleaseAll();

//////////////////////
// SETUP /////////////
//////////////////////
void setup()
{
  initializeArduino();
  initializeInputs();
  danceLeds();
}

////////////////////
// MAIN LOOP ///////
////////////////////
void loop()
{
  updateMeasurementBuffers();
  updateBufferSums();
  updateBufferIndex();
  updateInputStates();
  sendMouseButtonEvents();
  sendMouseMovementEvents();
  cycleLEDs();
  updateOutLEDs();
  safetyReleaseAll();
  addDelay();
}

//////////////////////////
// INITIALIZE ARDUINO
//////////////////////////
void initializeArduino() {
#ifdef DEBUG
  Serial.begin(9600);  // Serial for debugging
#endif

  /* Set up input pins
    DEactivate the internal pull-ups, since we're using external resistors */
  for (int i = 0; i < NUM_INPUTS; i++)
  {
    pinMode(pinNumbers[i], INPUT);
    digitalWrite(pinNumbers[i], LOW);
  }

  pinMode(inputLED_a, INPUT);
  pinMode(inputLED_b, INPUT);
  pinMode(inputLED_c, INPUT);
  digitalWrite(inputLED_a, LOW);
  digitalWrite(inputLED_b, LOW);
  digitalWrite(inputLED_c, LOW);

  pinMode(outputK, OUTPUT);
  pinMode(outputM, OUTPUT);
  pinMode(outputD4, OUTPUT);
  digitalWrite(outputK, HIGH);
  digitalWrite(outputM, HIGH);
  digitalWrite(outputD4, HIGH);


#ifdef DEBUG
  delay(4000); // allow us time to reprogram in case things are freaking out
#endif

  Keyboard.begin();
  Mouse.begin();
}

///////////////////////////
// INITIALIZE INPUTS
///////////////////////////
void initializeInputs() {

  float thresholdPerc = SWITCH_THRESHOLD_OFFSET_PERC;
  float thresholdCenterBias = SWITCH_THRESHOLD_CENTER_BIAS / 50.0;
  float pressThresholdAmount = (BUFFER_LENGTH * 8) * (thresholdPerc / 100.0);
  float thresholdCenter = ( (BUFFER_LENGTH * 8) / 2.0 ) * (thresholdCenterBias);
  pressThreshold = int(thresholdCenter + pressThresholdAmount);
  releaseThreshold = int(thresholdCenter - pressThresholdAmount);

#ifdef DEBUG
  Serial.println(pressThreshold);
  Serial.println(releaseThreshold);
#endif

  for (int i = 0; i < NUM_INPUTS; i++) {
    inputs[i].pinNumber = pinNumbers[i];
    inputs[i].keyCode = keyCodes[i];

    for (int j = 0; j < BUFFER_LENGTH; j++) {
      inputs[i].measurementBuffer[j] = 0;
    }
    inputs[i].oldestMeasurement = 0;
    inputs[i].bufferSum = 0;

    inputs[i].pressed = false;
    inputs[i].prevPressed = false;

    inputs[i].isMouseMotion = false;
    inputs[i].isMouseLeft = false;
    inputs[i].isMouseRight = false;
    inputs[i].isKey = false;

    if (inputs[i].keyCode < 0) {
#ifdef DEBUG_MOUSE
      Serial.println("GOT IT");
#endif
      inputs[i].isMouseMotion = true;
    }
    else if ((inputs[i].keyCode == MOUSE_LEFT)) {
      inputs[i].isMouseLeft = true;
    }
    else if ((inputs[i].keyCode == MOUSE_RIGHT)) {
      inputs[i].isMouseRight = true;
    }
    else {
      inputs[i].isKey = true;
    }
#ifdef DEBUG
    Serial.println(i);
#endif

  }
}


//////////////////////////////
// UPDATE MEASUREMENT BUFFERS
//////////////////////////////
void updateMeasurementBuffers() {

  for (int i = 0; i < NUM_INPUTS; i++) {

    // store the oldest measurement, which is the one at the current index,
    // before we update it to the new one
    // we use oldest measurement in updateBufferSums
    byte currentByte = inputs[i].measurementBuffer[byteCounter];
    inputs[i].oldestMeasurement = (currentByte >> bitCounter) & 0x01;

    // make the new measurement
    boolean newMeasurement = digitalRead(inputs[i].pinNumber);

    // invert so that true means the switch is closed
    newMeasurement = !newMeasurement;

    // store it
    if (newMeasurement) {
      currentByte |= (1 << bitCounter);
    }
    else {
      currentByte &= ~(1 << bitCounter);
    }
    inputs[i].measurementBuffer[byteCounter] = currentByte;
  }
}

///////////////////////////
// UPDATE BUFFER SUMS
///////////////////////////
void updateBufferSums() {

  // the bufferSum is a running tally of the entire measurementBuffer
  // add the new measurement and subtract the old one

  for (int i = 0; i < NUM_INPUTS; i++) {
    byte currentByte = inputs[i].measurementBuffer[byteCounter];
    boolean currentMeasurement = (currentByte >> bitCounter) & 0x01;
    if (currentMeasurement) {
      inputs[i].bufferSum++;
    }
    if (inputs[i].oldestMeasurement) {
      inputs[i].bufferSum--;
    }
  }
}

///////////////////////////
// UPDATE BUFFER INDEX
///////////////////////////
void updateBufferIndex() {
  bitCounter++;
  if (bitCounter == 8) {
    bitCounter = 0;
    byteCounter++;
    if (byteCounter == BUFFER_LENGTH) {
      byteCounter = 0;
    }
  }
}

///////////////////////////
// UPDATE INPUT STATES
///////////////////////////
void updateInputStates() {
  inputChanged = false;
  for (int i = 0; i < NUM_INPUTS; i++) {
    inputs[i].prevPressed = inputs[i].pressed; // store previous pressed state (only used for mouse buttons)
    if (inputs[i].pressed) {
      if (inputs[i].bufferSum < releaseThreshold) {
        inputChanged = true;
        inputs[i].pressed = false;
        if (inputs[i].isKey) {
          Keyboard.release(inputs[i].keyCode);
        }
        if (inputs[i].isMouseMotion) {
          mouseHoldCount[i] = 0;  // input becomes released, reset mouse hold
        }
      }
      else if (inputs[i].isMouseMotion) {
        mouseHoldCount[i]++; // input remains pressed, increment mouse hold
      }
    }
    else if (!inputs[i].pressed) {
      if (inputs[i].bufferSum > pressThreshold) {  // input becomes pressed
        inputChanged = true;
        inputs[i].pressed = true;
        if (inputs[i].isKey) {
          Keyboard.press(inputs[i].keyCode);
        }
      }
    }
  }
#ifdef DEBUG3
  if (inputChanged) {
    Serial.println("change");
  }
#endif
}

/////////////////////////////
// SEND MOUSE BUTTON EVENTS
/////////////////////////////
void sendMouseButtonEvents() {
  if (inputChanged) {
    for (int i = 0; i < NUM_INPUTS; i++) {
      if (inputs[i].isMouseLeft || inputs[i].isMouseRight) {
        if (inputs[i].pressed) {
          if (inputs[i].keyCode == MOUSE_LEFT) {
            Mouse.press(MOUSE_LEFT);
          }
          if (inputs[i].keyCode == MOUSE_RIGHT) {
            Mouse.press(MOUSE_RIGHT);
          }
        }
        else if (inputs[i].prevPressed) {
          if (inputs[i].keyCode == MOUSE_LEFT) {
            Mouse.release(MOUSE_LEFT);
          }
          if (inputs[i].keyCode == MOUSE_RIGHT) {
            Mouse.release(MOUSE_RIGHT);
          }
        }
      }
    }
  }
}

//////////////////////////////
// SEND MOUSE MOVEMENT EVENTS
//////////////////////////////
void sendMouseMovementEvents() {
  byte right = 0;
  byte left = 0;
  byte down = 0;
  byte up = 0;
  byte horizmotion = 0;
  byte vertmotion = 0;

  mouseMovementCounter++;
  mouseMovementCounter %= MOUSE_MOTION_UPDATE_INTERVAL;
  if (mouseMovementCounter == 0) {
    for (int i = 0; i < NUM_INPUTS; i++) {
#ifdef DEBUG_MOUSE
      //  Serial.println(inputs[i].isMouseMotion);
#endif

      if (inputs[i].isMouseMotion) {
        if (inputs[i].pressed) {
          if (inputs[i].keyCode == MOUSE_MOVE_UP) {
            // JL Changes (x4): now update to 1 + a hold factor, constrained between 1 and mouse max movement speed
            up = constrain(1 + mouseHoldCount[i] / MOUSE_RAMP_SCALE, 1, MOUSE_MAX_PIXELS);
          }
          if (inputs[i].keyCode == MOUSE_MOVE_DOWN) {
            down = constrain(1 + mouseHoldCount[i] / MOUSE_RAMP_SCALE, 1, MOUSE_MAX_PIXELS);
          }
          if (inputs[i].keyCode == MOUSE_MOVE_LEFT) {
            left = constrain(1 + mouseHoldCount[i] / MOUSE_RAMP_SCALE, 1, MOUSE_MAX_PIXELS);
          }
          if (inputs[i].keyCode == MOUSE_MOVE_RIGHT) {
            right = constrain(1 + mouseHoldCount[i] / MOUSE_RAMP_SCALE, 1, MOUSE_MAX_PIXELS);
          }
        }
      }
    }

    // diagonal scrolling and left/right cancellation
    if (left > 0)
    {
      if (right > 0)
      {
        horizmotion = 0; // cancel horizontal motion because left and right are both pushed
      }
      else
      {
        horizmotion = -left; // left yes, right no
      }
    }
    else
    {
      if (right > 0)
      {
        horizmotion = right; // right yes, left no
      }
    }

    if (down > 0)
    {
      if (up > 0)
      {
        vertmotion = 0; // cancel vertical motion because up and down are both pushed
      }
      else
      {
        vertmotion = down; // down yes, up no
      }
    }
    else
    {
      if (up > 0)
      {
        vertmotion = -up; // up yes, down no
      }
    }
    // now move the mouse
    if ( !((horizmotion == 0) && (vertmotion == 0)) )
    {
      Mouse.move(horizmotion * PIXELS_PER_MOUSE_STEP, vertmotion * PIXELS_PER_MOUSE_STEP);
    }
  }
}

///////////////////////////
// ADD DELAY
///////////////////////////
void addDelay() {

  loopTime = micros() - prevTime;
  if (loopTime < TARGET_LOOP_TIME) {
    int wait = TARGET_LOOP_TIME - loopTime;
    delayMicroseconds(wait);
  }

  prevTime = micros();

#ifdef DEBUG_TIMING
  if (loopCounter == 0) {
    int t = micros() - prevTime;
    Serial.println(t);
  }
  loopCounter++;
  loopCounter %= 999;
#endif

}

///////////////////////////
// CYCLE LEDS
///////////////////////////
void cycleLEDs() {
  pinMode(inputLED_a, INPUT);
  pinMode(inputLED_b, INPUT);
  pinMode(inputLED_c, INPUT);
  digitalWrite(inputLED_a, LOW);
  digitalWrite(inputLED_b, LOW);
  digitalWrite(inputLED_c, LOW);

  ledCycleCounter++;
  ledCycleCounter %= 6;

  if ((ledCycleCounter == 0) && inputs[0].pressed) {
    pinMode(inputLED_a, OUTPUT);        //up
    digitalWrite(inputLED_a, LOW);
    pinMode(inputLED_b, OUTPUT);
    digitalWrite(inputLED_b, HIGH);
    pinMode(inputLED_c, INPUT);
    digitalWrite(inputLED_c, LOW);
  }
  if ((ledCycleCounter == 1) && inputs[1].pressed) {
    pinMode(inputLED_a, OUTPUT);          //down
    digitalWrite(inputLED_a, HIGH);
    pinMode(inputLED_b, OUTPUT);
    digitalWrite(inputLED_b, LOW);
    pinMode(inputLED_c, INPUT);
    digitalWrite(inputLED_c, LOW);

  }
  if ((ledCycleCounter == 2) && inputs[2].pressed) {
    pinMode(inputLED_a, OUTPUT);          //left
    digitalWrite(inputLED_a, HIGH);
    pinMode(inputLED_b, INPUT);
    digitalWrite(inputLED_b, LOW);
    pinMode(inputLED_c, OUTPUT);
    digitalWrite(inputLED_c, LOW);
  }
  if ((ledCycleCounter == 3) && inputs[3].pressed) {
    pinMode(inputLED_a, OUTPUT);           //right
    digitalWrite(inputLED_a, LOW);
    pinMode(inputLED_b, INPUT);
    digitalWrite(inputLED_b, LOW);
    pinMode(inputLED_c, OUTPUT);
    digitalWrite(inputLED_c, HIGH);
  }
  if ((ledCycleCounter == 4) && inputs[4].pressed) {
    pinMode(inputLED_a, INPUT);          //space
    digitalWrite(inputLED_a, LOW);
    pinMode(inputLED_b, OUTPUT);
    digitalWrite(inputLED_b, LOW);
    pinMode(inputLED_c, OUTPUT);
    digitalWrite(inputLED_c, HIGH);
  }
  if ((ledCycleCounter == 5) && inputs[5].pressed) {
    pinMode(inputLED_a, INPUT);         //click
    digitalWrite(inputLED_a, LOW);
    pinMode(inputLED_b, OUTPUT);
    digitalWrite(inputLED_b, HIGH);
    pinMode(inputLED_c, OUTPUT);
    digitalWrite(inputLED_c, LOW);
  }

}

///////////////////////////
// DANCE LEDS
///////////////////////////
void danceLeds()
{
  int delayTime = 50;
  int delayTime2 = 100;

  // CIRCLE
  for (int i = 0; i < 4; i++)
  {
    // UP
    pinMode(inputLED_a, OUTPUT);
    digitalWrite(inputLED_a, LOW);
    pinMode(inputLED_b, OUTPUT);
    digitalWrite(inputLED_b, HIGH);
    pinMode(inputLED_c, INPUT);
    digitalWrite(inputLED_c, LOW);
    digitalWrite(outputK, LOW);
    delay(delayTime);

    //RIGHT
    pinMode(inputLED_a, OUTPUT);
    digitalWrite(inputLED_a, LOW);
    pinMode(inputLED_b, INPUT);
    digitalWrite(inputLED_b, LOW);
    pinMode(inputLED_c, OUTPUT);
    digitalWrite(inputLED_c, HIGH);
    digitalWrite(outputK, HIGH);
    delay(delayTime);


    // DOWN
    pinMode(inputLED_a, OUTPUT);
    digitalWrite(inputLED_a, HIGH);
    pinMode(inputLED_b, OUTPUT);
    digitalWrite(inputLED_b, LOW);
    pinMode(inputLED_c, INPUT);
    digitalWrite(inputLED_c, LOW);
    digitalWrite(outputK, LOW);
    delay(delayTime);

    // LEFT
    pinMode(inputLED_a, OUTPUT);
    digitalWrite(inputLED_a, HIGH);
    pinMode(inputLED_b, INPUT);
    digitalWrite(inputLED_b, LOW);
    pinMode(inputLED_c, OUTPUT);
    digitalWrite(inputLED_c, LOW);
    digitalWrite(outputK, HIGH);
    delay(delayTime);
  }

  // WIGGLE
  for (int i = 0; i < 4; i++)
  {
    // SPACE
    pinMode(inputLED_a, INPUT);
    digitalWrite(inputLED_a, HIGH);
    pinMode(inputLED_b, OUTPUT);
    digitalWrite(inputLED_b, LOW);
    pinMode(inputLED_c, OUTPUT);
    digitalWrite(inputLED_c, HIGH);
    digitalWrite(outputD4, LOW);
    digitalWrite(outputM, HIGH);
    delay(delayTime2);

    // CLICK
    pinMode(inputLED_a, INPUT);
    digitalWrite(inputLED_a, LOW);
    pinMode(inputLED_b, OUTPUT);
    digitalWrite(inputLED_b, HIGH);
    pinMode(inputLED_c, OUTPUT);
    digitalWrite(inputLED_c, LOW);
    digitalWrite(outputD4, HIGH);
    digitalWrite(outputM, LOW);
    delay(delayTime2);
  }
}

///////////////////////////
// SAFETY RELEASE ALL
// A cada 2s, se o Arduino acredita que NADA esta pressionado,
// reenvia "release" para o host. Destrava teclas fantasmas presas
// no macOS/Windows sem afetar teclas legitimamente seguradas.
///////////////////////////
void safetyReleaseAll() {
  static unsigned long lastSafetyMs = 0;
  if (millis() - lastSafetyMs < 2000) return;
  lastSafetyMs = millis();

  for (int i = 0; i < NUM_INPUTS; i++) {
    if (inputs[i].pressed) return; // algo legitimo esta pressionado
  }
  Keyboard.releaseAll();
  Mouse.release(MOUSE_LEFT);
  Mouse.release(MOUSE_RIGHT);
}

void updateOutLEDs()
{
  boolean keyPressed = 0;
  boolean mousePressed = 0;
  boolean mouseMotion = 0;
  boolean mouseClickLeft = 0;
  boolean mouseClickRight = 0;

  for (int i = 0; i < NUM_INPUTS; i++)
  {
    if (inputs[i].pressed)
    {
      if (inputs[i].isKey)
      {
        keyPressed = 1;
#ifdef DEBUG
        Serial.print("Key ");
        Serial.print(i);
        Serial.println(" pressed");
#endif
      }
      if (inputs[i].isMouseLeft)
      {
        mouseClickLeft = 1;
      }
      if (inputs[i].isMouseRight)
      {
        mouseClickRight = 1;
      }
      if (inputs[i].isMouseMotion)
      {
        mouseMotion = 1;
      }
    }
  }

  if (mouseClickRight)
  {
    digitalWrite(outputD4, LOW);
  }
  else
  {
    digitalWrite(outputD4, HIGH);
  }

  if (mouseClickLeft)
  {
    digitalWrite(outputM, LOW);
  }
  else
  {
    digitalWrite(outputM, HIGH);
  }

  if (mouseMotion)
  {
    digitalWrite(outputK, LOW);
  }
  else
  {
    digitalWrite(outputK, HIGH);
  }
}


