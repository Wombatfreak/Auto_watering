int val = 0;
void setup()
{
  Serial.begin(57600); //serial port to computer
  Serial.println("Startup");
}

void loop()
{
  val = analogRead(A0);
  Serial.println(val);
  delay(1000);
}

