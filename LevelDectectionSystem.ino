///////////////////////////////////////////////
////////////// SMART BARISTA //////////////////
///////// LEVEL DETECTION SYSTEM //////////////
///////////////////////////////////////////////

int trigger = 1; //trigger pin
int echo = 2; // pin that waits for echo
int data; // calculated distance

void setup()
{
  Serial.begin(9600);
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT); 
}

void loop()
{
  long t = 0; //time taken from transmission
  
  // Transmitting pulse
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  
  // Waiting for pulse
  t = pulseIn(echo, HIGH);

  data = (t/2) / 10.9; // Distance calculation

  if (data <= 2)
    {
    // send signal to relay to turn off machine
    }
  
  delay(1000);
}
