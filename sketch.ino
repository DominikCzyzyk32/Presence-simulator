#include <FastLED.h>
#define NUM_LEDS 77
#define LEDPin 8
CRGB leds[NUM_LEDS];

#define numberOfLedsInICR 24
int ledsToBeON[numberOfLedsInICR] = {12, 15, 16, 18, 19, 20, 23, 25, 29, 31, 34, 36, 40, 41, 42, 45, 47, 51, 52, 56, 59, 60, 62, 64};

#define numberOfLamps 2
#define LDRPin A5
#define PIRPin 12
int relayPin[numberOfLamps]={2, 3};
#define threshold 200
#define hysteresis 150
int restroomRelayPin = 4;


#define TVSessionsNumber 3
int TVPin = 8;
int TVLatency[TVSessionsNumber] = {10, 40, 80};
int TVDuration[TVSessionsNumber] = {10, 10, 10};

int alarmPin = 7;

int eveningLatency[numberOfLamps];
int eveningDuration[numberOfLamps];
int morningDuration[numberOfLamps];
int morningAnticipation[numberOfLamps];
int restroomLatency;
int restroomDuration;
int nightMinDuration = 90;
int nightMaxDuration = 180;
int nightDuration = 120; 

int dayMinDuration = 90;
int dayMaxDuration = 180;
int dayDuration = 120; 
  
    
enum {night, day} nightDay;


class chronometer
{
  unsigned long lastTime, actualTime;
  public :
  unsigned long elapsed;
  void init(); 
  void now(); 
  chronometer();
};

void chronometer::init()
{
  actualTime = millis();
  lastTime = actualTime;
}

void chronometer::now()
{
  actualTime = millis();
  if (actualTime < lastTime) 
    elapsed = (0xFFFFFFFFul - lastTime + actualTime);
  else
    elapsed = (actualTime - lastTime);
}

chronometer::chronometer()
{
  actualTime = millis();
  lastTime = actualTime;
}

chronometer lampChrono;
chronometer restroomChrono;
chronometer TVChrono;


void nightDayDetection(void){
  // Wchodimy do IFa jeżeli na zewnątrz jest jasno. 
  // Ustawiamy wsztstkie wejścia do przerzutników na LOW.
  // JEŻELI dotychczas była noc, to zmieniamy porę dnia na dzień i mierzymy jak długo trwała poprzednia noc.
  // -------------------------------------
  if(analogRead(LDRPin) <= threshold)
  {
    for(int i = 0; i < numberOfLamps; i++)
      digitalWrite(relayPin[i], LOW);
    
    if(nightDay == night)
    {
      Serial.println("NIGHT -> DAY");
      nightDay = day;
      lampChrono.now();
      restroomChrono.now();
      TVChrono.init();
      if(lampChrono.elapsed> ((unsigned long) nightMinDuration * 1000ul) && lampChrono.elapsed < ((unsigned long) nightMaxDuration * 1000ul))
         nightDuration = (int) (lampChrono.elapsed / 1000ul);
      
      int TVLatencyMin[TVSessionsNumber] = {0, 50, 90};
      int TVLatencyMax[TVSessionsNumber] = {5, 55, 95};
      int TVDurationMin[TVSessionsNumber] = {5, 3, 8};
      int TVDurationMax[TVSessionsNumber] = {20, 15, 25};

      for(int i = 0; i < 3; i++){
        TVLatency[i] = random(TVLatencyMin[i], TVLatencyMax[i]);
        TVDuration[i] = random(TVDurationMin[i], TVDurationMax[i]);
      }
    }
  }
  // -------------------------------------

  // Wchodzimy do ELSE IFa jeżeli na zewnątrz jest ciemno.
  // JEŻELI dotychczas był dzień, to 
  //    - zmieniamy go na noc 
  //    - wywołujemy lampChrono.init() - zaapamiętuję momen rozpoczęcia się nocy w polu 'latTime' klasy 'chronometr'
  //    - DLA KAŻDEJ DIODY 
  //           wybieramy z użyciem funkcji 'random()' 
  //                    jak długo ma świecić dioda wieczorem (eveningDuration)
  //                    jak długo ma świecić dioda rano (morningDuration)
  //           wybieramy z użyciem funkcji 'random()' 
  //                    jak dużo czasu ma upłynąć od rozpoczęcia nocy do zaświecenia diody (eveningLatency) 
  //                    jak dużo czasu ma upłynąć od zgaśnięcia diody do zakończenia nocy (morningAnticipation) 
  // -------------------------------------
  else if(analogRead(LDRPin) > threshold + hysteresis) 
  {
    if(nightDay == day)
    {
      Serial.println("DAY -> NIGHT");
      nightDay = night;
      lampChrono.init();
      restroomChrono.init();
      TVChrono.now();

      if(TVChrono.elapsed> ((unsigned long) dayMinDuration * 1000ul) && TVChrono.elapsed < ((unsigned long) dayMaxDuration * 1000ul))
         dayDuration = (int) (TVChrono.elapsed / 1000ul);

      for (int relayNumber=0; relayNumber<numberOfLamps; relayNumber++)
      {
        eveningDuration[relayNumber] = random(2, 20);
        eveningLatency[relayNumber] = random(15);
        morningDuration[relayNumber] = random(2, 20);
        morningAnticipation[relayNumber] = random(15);
      }

      restroomLatency = random(20, nightDuration - 20);
      restroomDuration = random(4, 7);
    }
  }
  // -------------------------------------
}


void lampManagement(void){
  int limit1, limit2, limit3, limit4, elapsed_seconds;
  lampChrono.now();
  for(int relayNumber = 0; relayNumber < numberOfLamps; relayNumber++)
  {
    limit1 = eveningLatency[relayNumber];
    limit2 = limit1 + eveningDuration[relayNumber];
    limit3 = nightDuration - morningAnticipation[relayNumber]- morningDuration[relayNumber];
    limit4 = limit3 + morningDuration[relayNumber];
    elapsed_seconds = (int) (lampChrono.elapsed / 1000);
    if((elapsed_seconds >= limit1 && elapsed_seconds < limit2)|| (elapsed_seconds >= limit3 && elapsed_seconds < limit4))
        digitalWrite(relayPin[relayNumber], HIGH);
      else
        digitalWrite(relayPin[relayNumber], LOW);
  }
}


void alarmManagement(void){
  if(digitalRead(PIRPin))
  {
    digitalWrite(alarmPin, HIGH);
    tone(alarmPin, 60, 2000);
  }
  else
  {
    digitalWrite(alarmPin, LOW);
    noTone(alarmPin);
  } 
}


void restroomLampManagement(void){
  int elapsed_seconds;
  restroomChrono.now();
  elapsed_seconds = (int) (restroomChrono.elapsed / 1000);
  if((elapsed_seconds >= restroomLatency && elapsed_seconds < restroomLatency + restroomDuration))
    digitalWrite(restroomRelayPin, HIGH);
  else
    digitalWrite(restroomRelayPin, LOW);
}


void playTV(void){
  for(int i = 0; i < numberOfLedsInICR; i++){
    leds[ledsToBeON[i]] = CRGB::Red;
  }
  FastLED.show();
  delay(500);
    for(int i = 0; i < numberOfLedsInICR; i++){
    leds[ledsToBeON[i]] = CRGB::Black;
  }
  FastLED.show();
  delay(500);
}


void TVManagement(void){
  TVChrono.now();
  int elapsed_seconds = (int) (TVChrono.elapsed / 1000);

  for(int i = 0; i < 3; i++){
    if((elapsed_seconds >= TVLatency[i] && elapsed_seconds < TVLatency[i] + TVDuration[i]))
      playTV();
  }
}

void setInitialValueForNightDay(void)
{
  while(millis() < 4000)
  {
    if(analogRead(LDRPin) <= threshold)
      nightDay = day;
    else if(analogRead(LDRPin) > threshold + hysteresis)
      nightDay = night;
  }
  if (nightDay == night)
    Serial.println("NIGHT");
  else if (nightDay == day)
    Serial.println("DAY");
}




void setup()
{
  for (int i=0; i<numberOfLamps; i++)
  {
    pinMode(relayPin[i], OUTPUT);
    digitalWrite(relayPin[i], LOW);
  }
  pinMode(alarmPin, OUTPUT);
  digitalWrite(alarmPin, LOW);

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  pinMode(12, INPUT);
  Serial.begin(9600);

  FastLED.addLeds<NEOPIXEL, LEDPin>(leds, NUM_LEDS);

  setInitialValueForNightDay();
}

void loop()
{ 
  nightDayDetection();

  if(nightDay == night){
    lampManagement();
    restroomLampManagement();
  }
  if(nightDay == day){
    TVManagement();
  }
  
  alarmManagement();
}
