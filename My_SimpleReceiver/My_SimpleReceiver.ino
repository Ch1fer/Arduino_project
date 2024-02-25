#include <Arduino.h>
#include "Adafruit_NeoPixel.h"


// This include defines the actual pin number for pins like IR_RECEIVE_PIN, IR_SEND_PIN for many different boards and architectures
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp> // include the library

#define DECODE_NEC // Includes Apple and Onkyo


#define LED_COUNT 138
#define LED_PIN A5

// Макроси для fireLineZones
// настройки зон
#define ZONE_AMOUNT 4   // количество зон
#define ZONE_K 0.08     // плавность движения зон
#define ZONE_MAX_SHIFT 10

// настройки пламени
#define HUE_START 5     // начальный цвет огня (0 красный, 80 зелёный, 140 молния, 190 розовый)
#define HUE_COEF 0.7    // коэффициент цвета огня (чем больше - тем дальше заброс по цвету)
#define SMOOTH_K 0.15   // коэффициент плавности огня
#define MIN_BRIGHT 90   // мин. яркость огня
#define MAX_BRIGHT 255  // макс. яркость огня
#define MIN_SAT 200     // мин. насыщенность
#define MAX_SAT 255     // макс. насыщенность

// для разработчиков
#include <FastLED.h>
#define ORDER_GRB       // порядок цветов ORDER_GRB / ORDER_RGB / ORDER_BRG
#define COLOR_DEBTH 3   // цветовая глубина: 1, 2, 3 (в байтах)
// на меньшем цветовом разрешении скетч будет занимать в разы меньше места,
// но уменьшится и количество оттенков и уровней яркости!

// ВНИМАНИЕ! define настройки (ORDER_GRB и COLOR_DEBTH) делаются до подключения библиотеки!
#include <microLED.h>
LEDdata leds[LED_COUNT];  // буфер ленты типа LEDdata (размер зависит от COLOR_DEBTH)
microLED stripFire(leds, LED_COUNT, LED_PIN);  // объект лента

#define ZONE_SIZE LED_COUNT / ZONE_AMOUNT
float zoneValues[ZONE_AMOUNT];
byte zoneRndValues[ZONE_AMOUNT];
float zoneShift[ZONE_AMOUNT - 1];
int8_t zoneRND[ZONE_AMOUNT - 1];

// ленивая жопа
#define FOR_i(from, to) for(int i = (from); i < (to); i++)
#define FOR_j(from, to) for(int j = (from); j < (to); j++)



int mode = 0;
int new_mode = 0;
bool power = true;

int old_brightness = 255;
int brightness = 255;

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 200;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


void setup() {
  
  strip.begin();

  Serial.begin(115200);

  startMillis = millis();

  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

  // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);
  Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));
}

void loop() {
  currentMillis = millis();
  strip.setBrightness(brightness);
  stripFire.setBrightness(brightness);
  // Serial.println(brightness);
  /*
    * Check if received data is available and if yes, try to decode it.
    * Decoded result is in the IrReceiver.decodedIRData structure.
    *
    * E.g. command is in IrReceiver.decodedIRData.command
    * address is in command is in IrReceiver.decodedIRData.address
    * and up to 32 bit raw data in IrReceiver.decodedIRData.decodedRawData
    */
  if (IrReceiver.decode()){

    /*
      * Print a short summary of received data
      */
    // IrReceiver.printIRResultShort(&Serial);
    // IrReceiver.printIRSendUsage(&Serial);
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
        Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
        // We have an unknown protocol here, print more info
        IrReceiver.printIRResultRawFormatted(&Serial, true);
    }
    Serial.println();

    /*
      * !!!Important!!! Enable receiving of the next value,
      * since receiving has stopped after the end of the current received data packet.
      */
    IrReceiver.resume(); // Enable receiving of the next value

    /*
      * Finally, check the received data and perform actions according to the received command
      */
    
    if (IrReceiver.decodedIRData.command == 0x47 && (currentMillis - startMillis >= period)){
      power = !power;
      if(!power){
        Serial.println("OFF");
      }
      else{
        Serial.print("Mode_");
        Serial.println(mode);
      }
      startMillis = millis();
    }
    else if(power){

      switch (IrReceiver.decodedIRData.command){
        
        case 0x7:

          if(brightness >= 95 && (currentMillis - startMillis >= period)){
            brightness = brightness - 32;
            startMillis = millis();
          }
          break;

        case 0x15:

          if(brightness <= 223 && (currentMillis - startMillis >= period)){
            brightness = brightness + 32;
            startMillis = millis();
          }
          break;

        case 0x16:
          new_mode = 0;
          break;

        case 0xC:
          new_mode = 1;
          break;

        case 0x18:
          new_mode = 2;
          break;

        case 0x5E:
          new_mode = 3;
          break;

        case 0x8:
          new_mode = 4;
          break;

        case 0x1C:
          new_mode = 5;
          break;

        case 0x5A:
          new_mode = 6;
          break;

        case 0x42:
          new_mode = 7;
          break;

        case 0x52:
          new_mode = 8;
          break;

        case 0x4A:
          new_mode = 9;
          break;
      }
    }
    // Serial.print(currentMillis);
    // Serial.print(" - ");
    // Serial.print(startMillis);
    // Serial.print(" = ");
    // Serial.println(currentMillis - startMillis);
  }
  if(power && (new_mode != mode || mode == 9 || old_brightness != brightness)){
      mode = new_mode;
      old_brightness = brightness;
      switch (mode){
      
        case 0:
          Serial.println("Mode_0");
          setColour(255, 0, 0); //red
          break;

        case 1:
          Serial.println("Mode_1");
          setColour(0, 255, 0); //green
          break;

        case 2:
          Serial.println("Mode_2");
          setColour(0, 0, 255); //blue
          break;

        case 3:
          Serial.println("Mode_3");
          setColour(255, 255, 0); //yellow
          break;
        
        case 4:
          Serial.println("Mode_4");
          setColour(255, 120, 0); //orange
          break;

        case 5:
          Serial.println("Mode_5");
          setColour(200, 0, 255); //purple
          break;

        case 6:
          Serial.println("Mode_6");
          break;

        case 7:
          Serial.println("Mode_7");
          break;

        case 8:
          Serial.println("Mode_8");
          break;

        case 9:
          Serial.println("Mode_9");
          fireTick();
          randomizeZones();
          break;
      }
    }
}

void clearStrip(){

  for(int i = 0; i < LED_COUNT; i++)
  {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }

  strip.show();

  return;
}

void setColour(int r, int g, int b){

  clearStrip();

  for(int i = 0; i < LED_COUNT; i++)
  {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.setBrightness(brightness);
  strip.show();
  strip.setBrightness(brightness);
  return;
}


//Функції для режиму fireLineZones
void randomizeZones() {
  static uint32_t prevTime, prevTime2;

  // задаёт направление движения зон
  if (millis() - prevTime >= 800) {
    prevTime = millis();
    FOR_i(0, ZONE_AMOUNT - 1) {
      if (abs(zoneShift[i] - zoneRND[i]) < 2)
        zoneRND[i] = random(-ZONE_MAX_SHIFT, ZONE_MAX_SHIFT);
    }
  }

  // движение зон
  if (millis() - prevTime2 >= 50) {
    prevTime2 = millis();
    FOR_i(0, ZONE_AMOUNT - 1) {
      zoneShift[i] += (zoneRND[i] - zoneShift[i]) * ZONE_K;
    }
  }
}

void fireTick() {
  static uint32_t prevTime, prevTime2;

  // задаём направление движения огня
  if (millis() - prevTime > 100) {
    prevTime = millis();
    FOR_i(0, ZONE_AMOUNT) {
      zoneRndValues[i] = random(0, 10);
    }
  }
  
  // двигаем пламя
  if (millis() - prevTime2 > 20) {
    prevTime2 = millis();
    int thisPos = 0, lastPos = 0;
    FOR_i(0, ZONE_AMOUNT) {
      zoneValues[i] = (float)zoneValues[i] * (1 - SMOOTH_K) + (float)zoneRndValues[i] * 10 * SMOOTH_K;
      //zoneValues[i] = (zoneRndValues[i] * 10 - zoneValues[i]) * SMOOTH_K;
      if (i < ZONE_AMOUNT - 1) thisPos += ZONE_SIZE + zoneShift[i];
      else thisPos = LED_COUNT;
      if (thisPos > LED_COUNT) break;

      // вывести огонь
      byte zoneHalf = (thisPos - lastPos) / 2;
      float valStep = (float)1 / zoneHalf;
      byte counter = 0;
      FOR_j(lastPos, lastPos + zoneHalf) {
        counter++;
        if (j > LED_COUNT) break;
        leds[j] = getFireColor((float)zoneValues[i] * counter * valStep);
      }
      counter = 0;
      FOR_j(lastPos + zoneHalf, thisPos) {
        counter++;
        if (j > LED_COUNT) break;
        leds[j] = getFireColor((float)zoneValues[i] * (zoneHalf - counter) * valStep);
      }

      // вывести цветные полосы
      //FOR_j(lastPos, thisPos) leds[j] = mHSV(i * 70, 255, 150);

      lastPos = thisPos;
    }
    stripFire.show();
  }
}

// возвращает цвет огня для одного пикселя
LEDdata getFireColor(int val) {
  // чем больше val, тем сильнее сдвигается цвет, падает насыщеность и растёт яркость
  return mHSV(
           HUE_START + val * HUE_COEF,                                  // H
           constrain(map(val, 20, 60, MAX_SAT, MIN_SAT), 0, 255),       // S
           constrain(map(val, 20, 60, MIN_BRIGHT, MAX_BRIGHT), 0, 255)  // V
         );
}





