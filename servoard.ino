#include <Servo.h>

Servo horizontal;  
Servo vertical;    

int servohori = 90; 
int servovert = 45; 

int tol = 0;
const int erreur = 30;

// Pins LDR
const int ldrld = A2;
const int ldrrd = A3;
const int ldrlt = A0;
const int ldrrt = A1;

int filteredLight = 0;

void setup() {
  Serial.begin(9600);          // ESP8266
  horizontal.attach(2);
  vertical.attach(3);
  horizontal.write(servohori);
  vertical.write(servovert);

  // --- Calibration ---
  unsigned long startTime = millis();
  long sumLight = 0;
  int count = 0;

  while (millis() - startTime < 15000) {
    int lt = analogRead(ldrlt);
    int rt = analogRead(ldrrt);
    int ld = analogRead(ldrld);
    int rd = analogRead(ldrrd);

    int total = (lt + rt + ld + rd) / 4;

    for ( int i = 5 ; i < 175 ; i++ ) {
      horizontal.write(i);
      vertical.write(i); 
      delay(100); 
    }

    sumLight += total;
    count++;
    delay(5);
  }

  tol = sumLight / count;  
  delay(1000);
}

void loop() {

  int lt = analogRead(ldrlt);
  int rt = analogRead(ldrrt);
  int ld = analogRead(ldrld);
  int rd = analogRead(ldrrd);

  int avt = (lt + rt) / 2;
  int avd = (ld + rd) / 2;
  int avl = (lt + ld) / 2;
  int avr = (rt + rd) / 2;

  int dvert = avt - avd;
  int dhoriz = avl - avr;


  int totalLight = (lt + rt + ld + rd) / 4;

  const int dtime = 30;
  const int speedStep = 1;

  filteredLight = (filteredLight * 0.7) + (totalLight * 0.3);

  //  Retour faible lumière 
  if (filteredLight < tol + erreur) {

    if (servohori < 90) servohori++;
    else if (servohori > 90) servohori--;
    horizontal.write(servohori);

    if (servovert < 45) servovert++;
    else if (servovert > 45) servovert--;
    vertical.write(servovert);

    delay(15);
  }

  else {

    if (abs(dvert) > (tol / 2)) {
      if (avt > avd) servovert = min(servovert + speedStep, 180);
      else           servovert = max(servovert - speedStep, 0);
      vertical.write(servovert);
    }

    if (abs(dhoriz) > (tol / 2)) {
      if (avl > avr) servohori = max(servohori - speedStep, 0);
      else           servohori = min(servohori + speedStep, 180);
      horizontal.write(servohori);
    }
  }
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'R') { // ESP
      Serial.print("<A>,");
      Serial.print(servohori);
      Serial.print(",");
      Serial.print(servovert);
      Serial.print(",");
      Serial.print(dvert);
      Serial.print(",");
      Serial.println(dhoriz);
    }
  }
  delay(dtime);
}
