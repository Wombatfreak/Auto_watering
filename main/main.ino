#include <rn2xx3.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <Servo.h>

SoftwareSerial mySerial(10, 11);
rn2xx3 myLora(mySerial);

Servo myservo;
int pos = 0;

float water_level = 0;
float humidity = 0;
float temperature = 0;
int wakeup_cnt = 0;
void setup()
{

  myservo.attach(7);
  // Set up Watchdog Timer
  MCUSR &= ~(1 << WDRF);          // Clear the reset flag
  WDTCSR |= (1 << WDCE) | (1 << WDE);  // Set up WDT for changes
  WDTCSR =  (1 << WDP3) | (1 << WDP0); // Set WDT to 8 seconds
  WDTCSR |= (1 << WDIE);          // Enable the WD interrupt (not reset)

  pinMode(13, OUTPUT);
  Serial.begin(57600); //serial port to computer
  mySerial.begin(9600);
  Serial.println("Startup");
  //initialize_radio();
  //myLora.tx("TTN Mapper on TTN Enschede node");
  delay(2000);
}

const char *appEui = "6081F9AE81CDD74F";
const char *appKey = "D290FE815DD1FD7738AE7A8289B5C56A";

void initialize_radio()
{
  //reset rn2483
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  delay(500);
  digitalWrite(12, HIGH);

  delay(100); //wait for the RN2xx3's startup message
  mySerial.flush();

  //Autobaud the rn2483 module to 9600. The default would otherwise be 57600.
  myLora.autobaud();

  //check communication with radio
  String hweui = myLora.hweui();
  while(hweui.length() != 16)
  {
    Serial.println("Communication with RN2xx3 unsuccesful. Power cycle the board.");
    Serial.println(hweui);
    delay(10000);
    hweui = myLora.hweui();
  }

  //print out the HWEUI so that we can register it via ttnctl
  Serial.println("When using OTAA, register this DevEUI: ");
  Serial.println(myLora.hweui());
  Serial.println("RN2xx3 firmware version:");
  Serial.println(myLora.sysver());

  //configure your keys and join the network
  Serial.println("Trying to join TTN");
  bool join_result = false;

  //ABP: initABP(String addr, String AppSKey, String NwkSKey);
  //join_result = myLora.initABP("02017201", "8D7FFEF938589D95AAD928C2E2E7E48F", "AE17E567AECC8787F749A62F5541D522");

  //OTAA: initOTAA(String AppEUI, String AppKey);
  join_result = myLora.initOTAA(appEui, appKey);

  while(!join_result)
  {
    Serial.println("Unable to join. Are your keys correct, and do you have TTN coverage?");
    delay(60000); //delay a minute before retry
    join_result = myLora.init();
  }
  Serial.println("Successfully joined TTN");

}

void loop()
{
  if (wakeup_cnt >= 1) { //450 = 1 hour
    Serial.println("Opening...");
    myservo.write(90);              // tell servo to go to position in variable 'pos'
    delay(3000);                       // waits 15ms for the servo to reach the position
    Serial.println("Closing...");
    myservo.write(0);              // tell servo to go to position in variable 'pos'
    delay(500);                       // waits 15ms for the servo to reach the position

    wakeup_cnt = 0;
    water_level = analogRead(A0);
    humidity = analogRead(A1);
    temperature = analogRead(A2);
    Serial.print("Water Level: ");
    Serial.println(water_level/50);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("TXing");
      //myLora.txCnf("!"); //one byte, blocking function
      //switch(myLora.txCnf("!")) //one byte, blocking function
      //{
      //  case TX_FAIL:
      //  {
      //    Serial.println("TX unsuccessful or not acknowledged");
      //    break;
      //  }
      //  case TX_SUCCESS:
      //  {
      //    Serial.println("TX successful and acknowledged");
      //    break;
      //  }
      //  case TX_WITH_RX:
      //  {
      //    String received = myLora.getRx();
      //    received = myLora.base16decode(received);
      //    Serial.print("Received downlink: " + received);
      //    break;
      //  }
      //  default:
      //  {
      //    Serial.println("Unknown response from TX function");
      //  }
      //}
  }
     wakeup_cnt = wakeup_cnt + 1;
     enterSleep();
}

ISR(WDT_vect) {
}

void enterSleep() {
    //Set sleep mode type and disable parts
    Serial.println("Entering sleep");
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    //Go to sleep
    sleep_mode();

    //On wake up
    sleep_disable();
    Serial.println("Exiting sleep");
}

