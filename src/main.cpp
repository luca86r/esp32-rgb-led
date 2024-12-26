#include <Arduino.h>
#include <OneButton.h>
#include "Preferences.h"
#include "math.h"

#define PIN_BTN_RED    12
#define PIN_BTN_GREEN  13
#define PIN_BTN_BLUE   14
#define PIN_LED_RED    25
#define PIN_LED_GREEN  26
#define PIN_LED_BLUE   27

typedef enum { 
        RED,
        GREEN,
        BLUE
  } color;

OneButton btnRed;
OneButton btnGreen;
OneButton btnBlue;

// === Preferences ===
Preferences preferences;
#define PREF_LAST_RED "LAST_RED"
#define PREF_LAST_GREEN "LAST_GREEN"
#define PREF_LAST_BLUE "LAST_BLUE"
#define PREF_BTN_RED_PRESET_RED "BTN_R_PRESET_R"
#define PREF_BTN_RED_PRESET_GREEN "BTN_R_PRESET_G"
#define PREF_BTN_RED_PRESET_BLUE "BTN_R_PRESET_B"
#define PREF_BTN_GREEN_PRESET_RED "BTN_G_PRESET_R"
#define PREF_BTN_GREEN_PRESET_GREEN "BTN_G_PRESET_G"
#define PREF_BTN_GREEN_PRESET_BLUE "BTN_G_PRESET_B"
#define PREF_BTN_BLUE_PRESET_RED "BTN_B_PRESET_R"
#define PREF_BTN_BLUE_PRESET_GREEN "BTN_B_PRESET_G"
#define PREF_BTN_BLUE_PRESET_BLUE "BTN_B_PRESET_B"
#define SAVE_DELAY 5000
long lastSavingMillis = 0;

int red = 0;
int green = 0;
int blue = 0;

int lastSavedRed = 0;
int lastSavedGreen = 0;
int lastSavedBlue = 0;

bool redIncrease = true;
bool greenIncrease = true;
bool blueIncrease = true;

int btnRedDefault[3] = {255, 255, 255};
int btnGreenDefault[3] = {150, 150, 150};
int btnBlueDefault[3] = {50, 50, 50};

#define KELVIN_TEMP_MIN 800
#define KELVIN_TEMP_MAX 8000
#define KELVIN_TEMP_STEP 30
int lastKelvinTmp = KELVIN_TEMP_MIN;

// From OneButton library:
// This function is called from the interrupt when the signal on the PIN_INPUT has changed.
// do not use Serial in here.
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO_EVERY)
void checkTicks() {
  // include all buttons here to be checked
  btnRed.tick();
  btnGreen.tick();
  btnBlue.tick();
}

#elif defined(ESP8266)
ICACHE_RAM_ATTR void checkTicks() {
  // include all buttons here to be checked
  btnRed.tick();
  btnGreen.tick();
  btnBlue.tick();
}

#elif defined(ESP32)
void IRAM_ATTR checkTicks() {
  // include all buttons here to be checked
  btnRed.tick();
  btnGreen.tick();
  btnBlue.tick();
}
#endif

// From https://gist.github.com/paulkaplan/5184275
// From http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/

// Start with a temperature, in Kelvin, somewhere between 1000 and 40000.  (Other values may work,
//  but I can't make any promises about the quality of the algorithm's estimates above 40000 K.) 

int clamp(int x, int min, int max ) {

    if(x<min){ return min; }
    if(x>max){ return max; }

    return x;
}

void setRgbFromColorTemperature(int kelvin){

    int temp = kelvin / 100;
    int r, g, b;

    if( temp <= 66 ){ 
        r = 255; 
        g = temp;
        g = 99.4708025861 * log(g) - 161.1195681661;

        if( temp <= 19){
            b = 0;
        } else {
            b = temp-10;
            b = 138.5177312231 * log(b) - 305.0447927307;
        }

    } else {
        r = temp - 60;
        r = 329.698727446 * pow(r, -0.1332047592);
        
        g = temp - 60;
        g = 288.1221695283 * pow(g, -0.0755148492 );

        b = 255;
    }

    red = clamp(r, 0, 255);
    green = clamp(g, 0, 255);
    blue = clamp(b, 0, 255);
}

void increaseColorTemperature() {

  if (lastKelvinTmp < KELVIN_TEMP_MAX) {
    lastKelvinTmp = lastKelvinTmp + KELVIN_TEMP_STEP;
  }
  
  setRgbFromColorTemperature(lastKelvinTmp);
}

void decreaseColorTemperature() {
  
  if (lastKelvinTmp > KELVIN_TEMP_MIN) {
    lastKelvinTmp = lastKelvinTmp - KELVIN_TEMP_STEP;
  }
  
  setRgbFromColorTemperature(lastKelvinTmp);
}

void increaseDecreaseColor(color c) {
  switch (c) {
  case RED:

    if (redIncrease && red < 255) {
      red++;
    }
    else if (!redIncrease && red > 0) {
      red--;
    }
    break;
  case GREEN:

    if (greenIncrease && green < 255) {
      green++;
    }
    else if (!greenIncrease && green > 0) {
      green--;
    }
    break;
  case BLUE:

    if (blueIncrease && blue < 255) {
      blue++;
    }
    else if (!blueIncrease && blue > 0) {
      blue--;
    }
    break;
  
  default:
    break;
  }
}

void resetColor(color c) {
  switch (c) {
  case RED:
    red = 0;
    break;
  case GREEN:
    green = 0;
    break;
  case BLUE:
    blue = 0;
    break;
  
  default:
    break;
  }
}

static void handleBtnRedDuringLongPress() {

  if (btnBlue.isLongPressed()) {
    increaseColorTemperature();
    return;
  }
  
  if (btnGreen.isLongPressed()) {
    decreaseColorTemperature();
    return;
  }

  increaseDecreaseColor(RED);
}

static void handleBtnGreenDuringLongPress() {

  if (btnRed.isLongPressed()) {
    // To nothing; manage color temperature in btnRed Long press handler
    return;
  }

  increaseDecreaseColor(GREEN);
}

static void handleBtnBlueDuringLongPress() {

  if (btnRed.isLongPressed()) {
    // To nothing; manage color temperature in btnRed Long press handler
    return;
  }

  increaseDecreaseColor(BLUE);
}

static void handleBtnRedDoubleClick() {
  resetColor(RED);
}

static void handleBtnGreenDoubleClick() {
  resetColor(GREEN);
}

static void handleBtnBlueDoubleClick() {
  resetColor(BLUE);
}

static void handleBtnRedClick() {
  redIncrease = !redIncrease;
}

static void handleBtnGreenClick() {
  greenIncrease = !greenIncrease;
}

static void handleBtnBlueClick() {
  blueIncrease = !blueIncrease;
}

static void handleBtnRedMultiClick() {

  if (btnRed.getNumberClicks() == 3) {
    Serial.println("Restore preset red btn");
    red = preferences.getInt(PREF_BTN_RED_PRESET_RED);
    green = preferences.getInt(PREF_BTN_RED_PRESET_GREEN);
    blue = preferences.getInt(PREF_BTN_RED_PRESET_BLUE);
  }

  if (btnRed.getNumberClicks() == 4) {
    Serial.println("Save preset red btn");
    preferences.putInt(PREF_BTN_RED_PRESET_RED, red);
    preferences.putInt(PREF_BTN_RED_PRESET_GREEN, green);
    preferences.putInt(PREF_BTN_RED_PRESET_BLUE, blue);
  }

  if (btnRed.getNumberClicks() == 5) {
    Serial.println("Default preset red btn");
    red = btnRedDefault[0];
    green = btnRedDefault[1];
    blue = btnRedDefault[2];
  }
}

static void handleBtnGreenMultiClick() {

  if (btnGreen.getNumberClicks() == 3) {
    Serial.println("Restore preset green btn");
    red = preferences.getInt(PREF_BTN_GREEN_PRESET_RED);
    green = preferences.getInt(PREF_BTN_GREEN_PRESET_GREEN);
    blue = preferences.getInt(PREF_BTN_GREEN_PRESET_BLUE);
  }

  if (btnGreen.getNumberClicks() == 4) {
    Serial.println("Save preset green btn");
    preferences.putInt(PREF_BTN_GREEN_PRESET_RED, red);
    preferences.putInt(PREF_BTN_GREEN_PRESET_GREEN, green);
    preferences.putInt(PREF_BTN_GREEN_PRESET_BLUE, blue);
  }

  if (btnGreen.getNumberClicks() == 5) {
    Serial.println("Default preset green btn");
    red = btnGreenDefault[0];
    green = btnGreenDefault[1];
    blue = btnGreenDefault[2];
  }
}

static void handleBtnBlueMultiClick() {

  if (btnBlue.getNumberClicks() == 3) {
    Serial.println("Restore preset blue btn");
    red = preferences.getInt(PREF_BTN_BLUE_PRESET_RED);
    green = preferences.getInt(PREF_BTN_BLUE_PRESET_GREEN);
    blue = preferences.getInt(PREF_BTN_BLUE_PRESET_BLUE);
  }

  if (btnBlue.getNumberClicks() == 4) {
    Serial.println("Save preset blue btn");
    preferences.putInt(PREF_BTN_BLUE_PRESET_RED, red);
    preferences.putInt(PREF_BTN_BLUE_PRESET_GREEN, green);
    preferences.putInt(PREF_BTN_BLUE_PRESET_BLUE, blue);
  }

  if (btnBlue.getNumberClicks() == 5) {
    Serial.println("Default preset blue btn");
    red = btnBlueDefault[0];
    green = btnBlueDefault[1];
    blue = btnBlueDefault[2];
  }
}

void savePreferences() {

  if (millis() - lastSavingMillis < SAVE_DELAY) {
    return;
  }

  lastSavingMillis = millis();

  if (red != lastSavedRed) {
    Serial.println("Saving red");
    preferences.putInt(PREF_LAST_RED, red);
    lastSavedRed = red;
  }

  if (green != lastSavedGreen) {
    Serial.println("Saving green");
    preferences.putInt(PREF_LAST_GREEN, green);
    lastSavedGreen = green;
  }

  if (blue != lastSavedBlue) {
    Serial.println("Saving blue");
    preferences.putInt(PREF_LAST_BLUE, blue);
    lastSavedBlue = blue;
  }
}

void loadPreferences() {
  red = lastSavedRed = preferences.getInt(PREF_LAST_RED);
  green = lastSavedGreen = preferences.getInt(PREF_LAST_GREEN);
  blue = lastSavedBlue = preferences.getInt(PREF_LAST_BLUE);
}

void setup() {

  Serial.begin(115200);

  // Init NVS memory for preferences
  preferences.begin("esp32-rgb-led", false);
  loadPreferences();

  pinMode(PIN_LED_RED,   OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE,  OUTPUT);

  btnRed.setup(PIN_BTN_RED,   INPUT_PULLUP, true);
  btnGreen.setup(PIN_BTN_GREEN, INPUT_PULLUP, true);
  btnBlue.setup(PIN_BTN_BLUE,  INPUT_PULLUP, true);

  btnRed.attachDuringLongPress(handleBtnRedDuringLongPress);
  btnGreen.attachDuringLongPress(handleBtnGreenDuringLongPress);
  btnBlue.attachDuringLongPress(handleBtnBlueDuringLongPress);

  btnRed.attachDoubleClick(handleBtnRedDoubleClick);
  btnGreen.attachDoubleClick(handleBtnGreenDoubleClick);
  btnBlue.attachDoubleClick(handleBtnBlueDoubleClick);

  btnRed.attachClick(handleBtnRedClick);
  btnGreen.attachClick(handleBtnGreenClick);
  btnBlue.attachClick(handleBtnBlueClick);

  btnRed.attachMultiClick(handleBtnRedMultiClick);
  btnGreen.attachMultiClick(handleBtnGreenMultiClick);
  btnBlue.attachMultiClick(handleBtnBlueMultiClick);


  // setup interrupt routine
  // when not registering to the interrupt the sketch also works when the tick is called frequently.
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_RED), checkTicks, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_GREEN), checkTicks, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_BLUE), checkTicks, CHANGE);
}

void loop() {

  btnRed.tick();
  btnGreen.tick();
  btnBlue.tick();

  analogWrite(PIN_LED_RED, red);
  analogWrite(PIN_LED_GREEN, green);
  analogWrite(PIN_LED_BLUE, blue);
  
  /*
  Serial.print(red);
  Serial.print(", ");
  Serial.print(green);
  Serial.print(", ");
  Serial.print(blue);
  Serial.print(", last k");
  Serial.println(lastKelvinTmp);
  */

  savePreferences();
  
  delay(20);

}
