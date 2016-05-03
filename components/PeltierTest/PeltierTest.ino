/*
Adafruit Arduino - Lesson 13. DC Motor
*/
 
 
int motorPin = 9;
 
void setup() 
{ 
  pinMode(motorPin, OUTPUT);
  delay(2000);
} 
 
 
void loop() 
{ 
  analogWrite(motorPin, 255);
  delay(5000);
  analogWrite(motorPin, 0);
  delay(5000);
  if (Serial.available())
  {
    int speed = Serial.parseInt();
    Serial.println(speed);
    if (speed >= 0 && speed <= 255)
    {
      analogWrite(motorPin, speed);
    }
  }
} 
