#include <rn2xx3.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <Servo.h>
#include <DHT11.h>

SoftwareSerial mySerial(10, 11);
rn2xx3 myLora(mySerial);
DHT11 dht11(4);

Servo myservo;
int pos = 0;

float water_level = 0;
float light_level = 0;
float humidity = 0;
float temperature = 0;
uint8_t tx_cnt = 0;
uint8_t seq_old = 0;
int wakeup_cnt = 0;
unsigned long startTime;

void setup()
{

  myservo.attach(7);
  myservo.write(0); 
  myservo.detach();
  // Set up Watchdog Timer
  MCUSR &= ~(1 << WDRF);          // Clear the reset flag
  WDTCSR |= (1 << WDCE) | (1 << WDE);  // Set up WDT for changes
  WDTCSR =  (1 << WDP3) | (1 << WDP0); // Set WDT to 8 seconds
  WDTCSR |= (1 << WDIE);          // Enable the WD interrupt (not reset)

  pinMode(13, OUTPUT);
  Serial.begin(9600); //serial port to computer
  mySerial.begin(9600);
  Serial.println("Startup");
  initialize_radio();
  myLora.tx("TTN Mapper on TTN Enschede node");
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
  Serial.println("Line: 99");
  if (wakeup_cnt >= 2) { //450 = 1 hour
    myLora.autobaud();
    Serial.println("Line: 101");
    wakeup_cnt = 0;
    Serial.println("Line: 103");
    water_level = analogRead(A0);
    Serial.println("Line: 105");
    light_level = analogRead(A1);
    Serial.println("Line: 107");
    delay(50);
    humidity = dht11.readHumidity();
    Serial.println("Line: 109");
    delay(50);
    temperature = dht11.readTemperature();
    Serial.print("TXing");
    uint8_t txBuffer[] = {water_level, humidity, temperature, light_level, tx_cnt};
    myLora.txBytes(txBuffer,sizeof(txBuffer));
    Serial.print("Bytes: ");
    Serial.println(sizeof(txBuffer));
    delay(50);
    String data = myLora.getRx();

    uint8_t numbers[3]; // Array to store the numbers

    // Splitting the string and converting to integer
    numbers[0] = (uint8_t)strtol(data.substring(0, 2).c_str(), NULL, 16);
    numbers[1] = (uint8_t)strtol(data.substring(2, 4).c_str(), NULL, 16);
    numbers[2] = (uint8_t)strtol(data.substring(4, 6).c_str(), NULL, 16);

    // Printing the numbers (optional, for verification)
    Serial.print("Number 1: ");
    Serial.println(numbers[0]);
    Serial.print("Number 2: ");
    Serial.println(numbers[1]);
    Serial.println(tx_cnt);
    Serial.print("Number 3: ");
    Serial.println(numbers[2]);

    //char data_buf[data.length()];
    //data.toCharArray(data_buf,data.length());
    //Serial.println(data);
    //Serial.println("Data: ");
    //Serial.println(data_buf[0]);
    //int byte1 = int1*16+int2;
    //Serial.println(data_buf[1]);
    //Serial.println(data_buf[2]);
    //Serial.println(data_buf[3]);
    //Serial.println(byte1);
//(humidity < 38 && temperature > 0) || 
    if ((numbers[0] == 1 || numbers[0] == 2) && numbers[2] !=  seq_old){
      myservo.attach(7);
      Serial.println("Opening...");
      myservo.write(90);              // tell servo to go to position in variable 'pos'
      unsigned long water_time = numbers[1]*1000;
      delay(water_time);                       // waits 15ms for the servo to reach the position
      Serial.println("Closing...");
      myservo.write(0);              // tell servo to go to position in variable 'pos'
      delay(90);                       // waits 15ms for the servo to reach the position
      myservo.detach();
    }
    seq_old = numbers[2];
    tx_cnt = tx_cnt + 1; 
    myLora.sleep(8000);
  }
    Serial.println("Line: 158");
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
