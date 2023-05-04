#include <Wire.h>
#include <BH1750.h>
#include <math.h>

BH1750 GY302;       // initialize BH1750 object
int PowerPin = 10;  // pin to go to relay
int on = 1;
float lighthrs;
float seconds;
float dli;
float time;

int msensor = A0;
int mout = PD7;
int msvalue = 0;  
int cooldown = 0;

// number of analog samples to take per reading
#define NUM_SAMPLES 200

int sum = 0;                    // sum of samples taken
unsigned char sample_count = 0; // current sample number
float voltage = 0.0;            // calculated voltagef
float ph;
// int soundPin = 9;
int acPin = PD6; // digital pin 7 - now 6
int virgrd = PB0; // digital pin 8
double val = 0 ;
int freq = 50;
double t = 0;
const double pi = 3.1415;
const double fs=1000;
int sound;

void setup() 
{
  Serial.begin(9600);
  Wire.begin();
  GY302.begin();  // initialize GY-302 module
  pinMode(PowerPin, OUTPUT);
  pinMode(msensor, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);  
  pinMode(mout, OUTPUT);
  pinMode(acPin, OUTPUT);

  seconds = 0;
  on = 1;
  lighthrs = 0;
  dli = 0;

}

void loop() 
{
  // get reading from module
  float lux = GY302.readLightLevel();
  float ppfd = lux * 0.0165 / 2;          // multiply lux by the conversion factor to get ppfd, calibrated
  time = time / 3600;
  dli += ppfd * lighthrs * 0.0036;  // dli calculation
  time = 0;
  if (dli >= 16) {
    on = 0;  // turns the light off when dli gets high enough
  }
  if (on == 1) {
    digitalWrite(PowerPin, HIGH); // if on = 1, turn on relay
  }
  else {
    digitalWrite(PowerPin, LOW); // if on = 0, turn off relay
  }

  while (sample_count < NUM_SAMPLES) {
        sum += analogRead(A2); // analog pin 2
        sample_count++;
        delay(10);
    }
  time = time + 2;
  seconds = seconds + 2;

  voltage = ((float)sum / (float)NUM_SAMPLES * 5.015) / 1024.0;

  // send voltage for display on Serial Monitor
  // voltage multiplied by 11 when using voltage divider that
  // divides by 11. 11.132 is the calibrated voltage divide
  // value
  voltage = 250 - (voltage * 11.132 * 10000);
  ph = 7 - voltage/ 57.14;
  Serial.print("ph: ");
  Serial.println(ph);
  if (ph <= 3) { // if ph is too low
    // digitalWrite(soundPin, HIGH); // alert user..
    float starttime = millis();
    // float endtime = starttime;
    while ((millis() - starttime) <=5000) {
      t = millis();
      val = 127*sin(2*pi*(freq/fs)*t)+127;
      analogWrite(acPin,val);
      analogWrite(virgrd,127);
      sum = analogRead(A2);
    }
    time = time + 5;
    seconds = seconds + 5;
  }
  else if (ph >= 9) { // if ph is too high
    // digitalWrite(soundPin, HIGH); // alert user
    float starttime = millis();
    // float endtime = starttime;
    while ((millis() - starttime) <=5000) {
      t = millis();
      val = 127*sin(2*pi*(freq/fs)*t)+127;
      analogWrite(acPin,val);
      analogWrite(virgrd,127);
      sum = analogRead(A2);
    }
    time = time + 5;
    seconds = seconds + 5;
  } 
  analogWrite(acPin,LOW);
  analogWrite(virgrd,LOW);
  
  sample_count = 0;
  sum = 0;

  msvalue = analogRead(msensor); // read value on moisture sensor
  
  if ( ( msvalue >= 459 ) && ( cooldown == 0 ) ) {// 10.5% lower bound
    digitalWrite(LED_BUILTIN, HIGH); // open solenoid
    digitalWrite(mout, HIGH); // open solenoid
    delay(6500); // open solenoid for 6.5 seconds, should get it from 10.5% to 11.5%
    seconds = seconds + 6.5;
    time = time + 6.5;
    digitalWrite(LED_BUILTIN, LOW); // once at optimal point, close solenoid
    digitalWrite(mout, LOW);
    msvalue = analogRead(msensor); // reread value on moisture sensor
    cooldown = 3;
  }
  else { // wait for soil to dry out
    digitalWrite(LED_BUILTIN, LOW); // solenoid closed
    digitalWrite(mout, LOW); // solenoid closed
    delay(5000); 
    seconds = seconds + 5;
    time = time + 5;
    if ( cooldown > 0 ) {
    cooldown--;
    }
  }

  lighthrs = seconds / 3600;
  if (lighthrs >= 24) {  // reset every day
    lighthrs = 0;
    seconds = 0;
  }  
}
