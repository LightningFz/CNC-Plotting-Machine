// Final version for the pet feeder

/*  Features:
    - Easy  to navigate menu
    - Overview of feed times, current time, feeding comletions,
      and feeding portion on the main screen
    - Controllable portions using  a hall sensor for feedback
    - Accurate time keeping with DS1307 chip
    -  Can manually change set time in DS1307 chip
    - Two feedings per day
    -  Manual feeding option
    - Feeding restart in case of power outtage
    -  LED indication of hall sensor and real time clock
    - Feeding times and completions  safetly store in EEPROM
    - Servo "jiggle" in the event of food getting stuck
*/

#include <JC_Button.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LiquidMenu.h>
#include <RTClib.h>
#include <Servo.h>
#include <EEPROM.h>
#include <MFRC522.h>

#include <SPI.h>
#include <RFID.h>
//  Creates servo object to control your servo
Servo myservo;

// Assigning  all my input and I/O pins
#define ENTER_BUTTON_PIN 3
#define UP_BUTTON_PIN  A0
#define DOWN_BUTTON_PIN A3
#define BACK_BUTTON_PIN A1
#define POWER_LED_PIN  4
#define MANUAL_BUTTON_PIN A2
#define hallPin 2
#define HALL_LED_PIN 6
#define SERVO_PIN 5
#define SDA_DIO 10
#define RESET_DIO 9

// Defining all the Buttons, works with the JC_Button library
Button enterBtn (ENTER_BUTTON_PIN);
Button upBtn (UP_BUTTON_PIN);
Button downBtn  (DOWN_BUTTON_PIN);
Button backBtn (BACK_BUTTON_PIN);
Button manualFeedBtn  (MANUAL_BUTTON_PIN);

/* Create an instance of the RFID library */
RFID RC522(SDA_DIO, RESET_DIO); 

// Defining LCD I2C and RTC
LiquidCrystal_I2C lcd(0x27,  16, 2);
RTC_DS1307 rtc;

//Variables used throughout the code

unsigned  long hallSensorTime;
unsigned long rotationTime = 400;
unsigned long led_previousMillis  = 0;
const long interval_delay = 1000;
unsigned long delay_interval = 2000;
int  ledState = LOW;

boolean manualFeed = false;
boolean hall_sensor_fail =  false;

unsigned long blink_previousMillis = 0;
unsigned long blink_currentMillis  = 0;
unsigned long blink_interval = 500;

unsigned long delay_currentMillis  = 0;
unsigned long delay_previousMillis = 0;

boolean blink_state = false;
int  count;
boolean feeding1_complete = false;
boolean feeding2_complete = false;
boolean  feeding1_trigger = false;
boolean feeding2_trigger = false;
boolean servoOn  = true;

//Hall sensor interrupt

volatile boolean hallSensorActivated  = false;
volatile int isr_count = 1;
void hallActiveISR()
{
  hallSensorActivated  = true;
  digitalWrite(HALL_LED_PIN, HIGH);
  isr_count = isr_count + 1;
}

/*  I use enums here to keep better track of what button is
    being pushed as opposed  to just having each button set to
    an interger value.
*/
enum {
  btnENTER,
  btnUP,
  btnDOWN,
  btnBACK,
};

/* States of the  State Machine. Same thing here with the enum
   type. It makes it easier to keep  track of what menu you are
   in or want to go to instead of giving each menu  an intreger value
*/
enum STATES {
  MAIN,
  MENU_EDIT_FEED1,
  MENU_EDIT_FEED2,
  MENU_EDIT_TIME,
  MENU_EDIT_PORTION,

  EDIT_FEED1_HOUR,
  EDIT_FEED1_MIN,

  EDIT_FEED2_HOUR,
  EDIT_FEED2_MIN,

  EDIT_HOUR,
  EDIT_MIN,

  EDIT_PORTION
};

// Holds state of the state machine
STATES state;

//User  input variables
int Hour;
int Minute;
int portion;

int feed_time1_hour;
int  feed_time1_min;
int feed_time2_hour;
int feed_time2_min;

int userInput;

//  Special character check mark
byte check_Char[8] = {
  B00000,
  B00000,
  B00001,
  B00011,
  B10110,
  B11100,
  B11000,
  B00000
};
//======================The  Setup===========================

void setup() {
  Wire.begin();
  Serial.begin(9600);
  SPI.begin(); 
  RC522.init();
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, check_Char);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
  }

  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //The buttons
  enterBtn.begin();
  upBtn.begin();
  downBtn.begin();
  backBtn.begin();
  manualFeedBtn.begin();

  //Setting up initial state of State Machine
  state = MAIN;

  //Setting up inputs and outputs
  pinMode(POWER_LED_PIN,  OUTPUT);
  pinMode(HALL_LED_PIN, OUTPUT);
  pinMode(hallPin, INPUT);

  /* Attatching interrupt to the pin that connects to the hall sensor.
     The  hall sensor I used is set to HIGH and goes LOW when it encounters
     a magnet.  Thats why its set to FALLING
  */
  attachInterrupt (digitalPinToInterrupt(hallPin),  hallActiveISR, FALLING);
  // default state of LEDs
  digitalWrite(POWER_LED_PIN,  HIGH);
  digitalWrite (HALL_LED_PIN, LOW);

  /* These functions get the  stored feed time, completed feeding,
     and portion size from EEPROM on start-up.  I did this because I get random
     power outtages here where I live.
  */
  get_feed_time1();
  get_feed_time2();
  get_completed_feedings();
  get_portion();

}

//======================The  Loop===========================
void loop() {
  changing_states();

  check_buttons();

  check_feedtime ();

  check_rtc();

  manual_feed_check  ();
if (RC522.isCard())
  {
    /* If so then get its serial number */
    RC522.readCardSerial();
    Serial.println("Card detected: ");
    Serial.print(RC522.serNum[i],DEC);
    /* Output the serial number to the UART */
    ID=RC522.serNum[0];
    if(ID == 671802391810){
      Serial.println("sjdhgaks,d");
    } 
    if(ID == 192371345276){
      Serial.println("ertab,,d");
    }
  }

}

//=============== The Functions =======================

/*  Uses the JC_Button Library to continually check if a button
   was pressed. Depending  on what button is pressed, it sets the
   variable userInput to be used in the  fucntion menu_transitions
*/
void check_buttons () {
  enterBtn.read();
  upBtn.read();
  downBtn.read();
  backBtn.read();
  manualFeedBtn.read();

  if (enterBtn.wasPressed()) {
    Serial.println ("You Pressed Enter!");
    userInput = btnENTER;
    menu_transitions(userInput);
  }
  if (upBtn.wasPressed())  {
    Serial.println ("You Pressed Up!");
    userInput = btnUP;
    menu_transitions(userInput);
  }
  if (downBtn.wasPressed()) {
    Serial.println ("You Pressed Down!");
    userInput = btnDOWN;
    menu_transitions(userInput);
  }
  if (backBtn.wasPressed())  {
    Serial.println ("You Pressed Back!");
    userInput = btnBACK;
    menu_transitions(userInput);
  }
  if (manualFeedBtn.wasPressed()) {
    Serial.println ("You Are Manually Feeding!");
    manualFeed = true;
  }
}
//=====================================================

/* This  funcion determines what is displayed, depending on what menu
    or "state"  you are in. Each menu has a function that displays the
    respective menu
*/
void  changing_states() {

  switch (state) {
    case MAIN:
      display_current_time();
      display_feeding_times();
      display_portion();
      break;

    case MENU_EDIT_FEED1:
      display_set_feed_time1_menu();
      break;

    case MENU_EDIT_FEED2:
      display_set_feed_time2_menu();
      break;

    case MENU_EDIT_TIME:
      display_set_time_menu();
      break;

    case MENU_EDIT_PORTION:
      display_set_portion_menu ();
      break;

    case EDIT_FEED1_HOUR:
      set_feed_time1();
      break;

    case  EDIT_FEED1_MIN:
      set_feed_time1();
      break;

    case EDIT_FEED2_HOUR:
      set_feed_time2();
      break;

    case EDIT_FEED2_MIN:
      set_feed_time2();
      break;

    case EDIT_HOUR:
      set_the_time();
      break;

    case EDIT_MIN:
      set_the_time();
      break;

    case EDIT_PORTION:
      set_the_portion();
      break;
  }
}
//=====================================================
/*  This is the transitional part of the state machine. This is
   what allows you  to go from one menu to another and change values
*/
void menu_transitions(int  input) {

  switch (state) {
    case MAIN:
      if (input == btnENTER)  {
        lcd.clear();
        state = MENU_EDIT_FEED1;
      }
      if  (input == btnBACK)
      {
        hall_sensor_fail = false;
      }
      break;
    //----------------------------------------------------
    case  MENU_EDIT_FEED1:
      if (input == btnBACK) {
        lcd.clear();
        state  = MAIN;
      }
      else if (input == btnENTER) {
        lcd.clear();
        state = EDIT_FEED1_HOUR;
      }
      else if (input == btnDOWN)  {
        lcd.clear();
        state = MENU_EDIT_FEED2;
      }
      break;
    //----------------------------------------------------
    case EDIT_FEED1_HOUR:
      // Need this to prevent servo going off while setting time
      servoOn  = false;
      if (input == btnUP) {
        feed_time1_hour++;
        if  (feed_time1_hour > 23) {
          feed_time1_hour = 0;
        }
      }
      else if (input == btnDOWN) {
        feed_time1_hour--;
        if (feed_time1_hour  < 0) {
          feed_time1_hour = 23;
        }
      }
      else  if (input == btnBACK) {
        lcd.clear();
        servoOn = true;
        state  = MENU_EDIT_FEED1;
      }
      else if (input == btnENTER) {
        state  = EDIT_FEED1_MIN;
      }
      break;
    //----------------------------------------------------
    case EDIT_FEED1_MIN:
      if (input == btnUP) {
        feed_time1_min++;
        if (feed_time1_min > 59) {
          feed_time1_min = 0;
        }
      }
      else if (input == btnDOWN) {
        feed_time1_min--;
        if  (feed_time1_min < 0) {
          feed_time1_min = 59;
        }
      }
      else if (input == btnBACK) {
        state = EDIT_FEED1_HOUR;
      }
      else if (input == btnENTER) {
        lcd.clear();
        lcd.setCursor(0,  0);
        lcd.print( "*Settings Saved*");
        delay(1000);
        lcd.clear();
        servoOn = true;
        write_feeding_time1 ();
        state = MAIN;
      }
      break;
    //----------------------------------------------------
    case MENU_EDIT_FEED2:
      if (input == btnUP) {
        lcd.clear();
        state = MENU_EDIT_FEED1;
      }
      else if (input == btnENTER)  {
        lcd.clear();
        state = EDIT_FEED2_HOUR;
      }
      else  if (input == btnDOWN) {
        lcd.clear();
        state = MENU_EDIT_TIME;
      }
      break;
    //----------------------------------------------------
    case EDIT_FEED2_HOUR:
      servoOn = false;
      if (input == btnUP)  {
        feed_time2_hour++;
        if (feed_time2_hour > 23) {
          feed_time2_hour  = 0;
        }
      }
      else if (input == btnDOWN) {
        feed_time2_hour--;
        if (feed_time2_hour < 0) {
          feed_time2_hour = 23;
        }
      }
      else if (input == btnBACK) {
        lcd.clear();
        servoOn  = true;
        state = MENU_EDIT_FEED2;
      }
      else if (input ==  btnENTER) {
        state = EDIT_FEED2_MIN;
      }
      break;
    //----------------------------------------------------
    case EDIT_FEED2_MIN:
      if (input == btnUP) {
        feed_time2_min++;
        if (feed_time2_min > 59) {
          feed_time2_min = 0;
        }
      }
      else if (input == btnDOWN) {
        feed_time2_min--;
        if  (feed_time2_min < 0) {
          feed_time2_min = 59;
        }
      }
      else if (input == btnBACK) {
        state = EDIT_FEED2_HOUR;
      }
      else if (input == btnENTER) {
        lcd.clear();
        lcd.setCursor(0,  0);
        lcd.print( "*Settings Saved*");
        delay(1000);
        lcd.clear();
        servoOn = true;
        write_feeding_time2 ();
        state = MAIN;
      }
      break;
    //----------------------------------------------------
    case MENU_EDIT_TIME:
      if (input == btnUP) {
        lcd.clear();
        state = MENU_EDIT_FEED2;
      }
      else if (input == btnENTER)  {
        lcd.clear();
        state = EDIT_HOUR;
      }
      else  if (input == btnDOWN) {
        lcd.clear();
        state = MENU_EDIT_PORTION;
      }
      break;
    //----------------------------------------------------
    case EDIT_HOUR:
      if (input == btnUP) {
        Hour++;
        if  (Hour > 23) {
          Hour = 0;
        }
      }
      else if (input  == btnDOWN) {
        Hour--;
        if (Hour < 0) {
          Hour =  23;
        }
      }
      else if (input == btnBACK) {
        lcd.clear();
        state = MENU_EDIT_TIME;
      }
      else if (input == btnENTER)  {
        state = EDIT_MIN;
      }
      break;
    //----------------------------------------------------
    case EDIT_MIN:
      if (input == btnUP) {
        Minute++;
        if  (Minute > 59) {
          Minute = 0;
        }
      }
      else if  (input == btnDOWN) {
        Minute--;
        if (Minute < 0) {
          Minute  = 59;
        }
      }
      else if (input == btnBACK) {
        state  = EDIT_HOUR;
      }
      else if (input == btnENTER) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print( "*Settings Saved*");
        delay(1000);
        lcd.clear();
        rtc.adjust(DateTime(0, 0, 0, Hour, Minute, 0));
        state = MAIN;
      }
      break;
    //----------------------------------------------------
    case MENU_EDIT_PORTION:
      if (input == btnUP) {
        lcd.clear();
        state = MENU_EDIT_TIME;
      }
      else if (input == btnENTER)  {
        lcd.clear();
        state = EDIT_PORTION;
      }
      break;
    //----------------------------------------------------
    case EDIT_PORTION:
      if (input == btnUP) {
        portion++;
        if (portion > 20) {
          portion = 1;
        }
      }
      else if (input == btnDOWN)  {
        portion--;
        if (portion < 1) {
          portion = 20;
        }
      }
      else if (input == btnBACK) {
        lcd.clear();
        state = MENU_EDIT_PORTION;
      }
      else if (input == btnENTER)  {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(  "*Settings Saved*");
        delay(1000);
        lcd.clear();
        write_portion();
        state = MAIN;
      }
      break;
  }
}
//=====================================================
//  This function checks the feed time against the current time

void check_feedtime  ()
{
  DateTime now = rtc.now();
  if (now.second() == 0) {
    if ((now.hour()  == feed_time1_hour) && (now.minute() == feed_time1_min))
    {
      feeding1_trigger  = true;
      if (servoOn)
      {
        if (feeding1_complete == false)
        {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print  ("Dispensing");
          lcd.setCursor(1, 1);
          lcd.print("First  Feeding");
          startFeeding();
        }
      }
    }
    else  if ((now.hour() == feed_time2_hour) && (now.minute () == feed_time2_min))
    {
      feeding2_trigger = true;
      if (servoOn)
      {
        if (  feeding2_complete == false)
        {
          lcd.clear();
          lcd.setCursor(3,  0);
          lcd.print ("Dispensing");
          lcd.setCursor(0, 1);
          lcd.print("Second Feeding");
          startFeeding();
        }
      }
    }
  }
  // Midnight Reset
  if ( (now.hour() == 0) && (now.minute()  == 0))
  {
    feeding1_complete = false;
    feeding2_complete = false;
    EEPROM.write(4, feeding1_complete);
    EEPROM.write(5, feeding2_complete);
  }
  /*If power outtage happens during a feed time, this checks to see if the
     feed time has passed and if the feeding occurred. If not, it feeds.
  */
  if ( (now.hour() >= feed_time1_hour) && (now.minute() > feed_time1_min))
  {
    if ((feeding1_complete == 0) && (feeding1_trigger == 0))
    {
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print ("Uh-Oh!");
      lcd.setCursor(2,  1);
      lcd.print("Power Outage");
      startFeeding();
    }
  }
  if ( (now.hour() >= feed_time2_hour) && (now.minute() > feed_time2_min))
  {
    if ((feeding2_complete == 0) && (feeding2_trigger == 0))
    {
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print ("Uh-Oh!");
      lcd.setCursor(2, 1);
      lcd.print("Power Outage");
      startFeeding();
    }
  }
}

//=====================================================
//  Displays the set portion menu option
void display_set_portion_menu () {
  lcd.setCursor(2,  0);
  lcd.print("Menu Options");
  lcd.setCursor(0, 1);
  lcd.print("Set  the Portion");
}
//=====================================================
//  Displays the menu where you change the current time
void set_the_time ()
{
  lcd.setCursor(2, 0);
  lcd.print("Set the Time");
  switch (state)
  {
    //----------------------------------------------------
    case EDIT_HOUR:

      if (blink_state == 0)
      {
        lcd.setCursor(5, 1);
        add_leading_zero(Hour);
      }
      else
      {
        lcd.setCursor(5, 1);
        lcd.print("  ");
      }
      lcd.print(":");
      add_leading_zero(Minute);
      break;
    //----------------------------------------------------
    case  EDIT_MIN:
      lcd.setCursor(5, 1);
      add_leading_zero(Hour);
      lcd.print(":");
      if (blink_state == 0)
      {
        lcd.setCursor(8, 1);
        add_leading_zero(Minute);
      }
      else
      {
        lcd.setCursor(8, 1);
        lcd.print("  ");
      }
      break;
  }
  blinkFunction();
}
//=====================================================
//  Displays the menu where you change the feeding portion
void set_the_portion ()
{
  lcd.setCursor (0, 0);
  lcd.print("Set the Portion");
  switch (state)
  {
    case EDIT_PORTION:
      if (blink_state == 0)
      {
        lcd.setCursor(7,  1);
        add_leading_zero(portion);
      }
      else
      {
        lcd.setCursor(7, 1);
        lcd.print("  ");
      }
  }
  blinkFunction();
}
//=====================================================
//Displays  the menu option for setting the time
void display_set_time_menu () {
  lcd.setCursor(2,  0);
  lcd.print("Menu Options");
  lcd.setCursor(2, 1);
  lcd.print("Set  the Time");
}
//=====================================================
//  Displays the menu where you change the second feeding time
void set_feed_time2  ()
{
  lcd.setCursor(0, 0);
  lcd.print("Set Feed Time 2");
  switch  (state)
  {
    //----------------------------------------------------
    case EDIT_FEED2_HOUR:

      if (blink_state == 0)
      {
        lcd.setCursor(5,  1);
        add_leading_zero(feed_time2_hour);
      }
      else
      {
        lcd.setCursor(5, 1);
        lcd.print("  ");
      }
      lcd.print(":");
      add_leading_zero(feed_time2_min);
      break;
    //----------------------------------------------------
    case EDIT_FEED2_MIN:
      lcd.setCursor(5, 1);
      add_leading_zero(feed_time2_hour);
      lcd.print(":");
      if (blink_state == 0)
      {
        lcd.setCursor(8,  1);
        add_leading_zero(feed_time2_min);
      }
      else
      {
        lcd.setCursor(8, 1);
        lcd.print("  ");
      }
      break;
  }
  blinkFunction();
}
//=====================================================
//  Displays the menu where you change the first feeding time
void set_feed_time1  ()
{
  lcd.setCursor(0, 0);
  lcd.print("Set Feed Time 1");
  switch  (state)
  {
    //----------------------------------------------------
    case EDIT_FEED1_HOUR:

      if (blink_state == 0)
      {
        lcd.setCursor(5,  1);
        add_leading_zero(feed_time1_hour);
      }
      else
      {
        lcd.setCursor(5, 1);
        lcd.print("  ");
      }
      lcd.print(":");
      add_leading_zero(feed_time1_min);
      break;
    //----------------------------------------------------
    case EDIT_FEED1_MIN:
      lcd.setCursor(5, 1);
      add_leading_zero(feed_time1_hour);
      lcd.print(":");
      if (blink_state == 0)
      {
        lcd.setCursor(8,  1);
        add_leading_zero(feed_time1_min);
      }
      else
      {
        lcd.setCursor(8, 1);
        lcd.print("  ");
      }
      break;
  }
  blinkFunction();
}
//=====================================================
//  Adds a leading zero to single digit numbers
void add_leading_zero (int num) {
  if (num < 10) {
    lcd.print("0");
  }
  lcd.print(num);
}
//=====================================================
/*  Displays the feeding time on the main menu as well as the
   check mark for visual  comfirmation of a completed feeding
*/
void display_feeding_times () {
  //Displaying first feed time
  lcd.setCursor(0, 0);
  lcd.print ("F1:");
  add_leading_zero(feed_time1_hour);
  lcd.print(":");
  add_leading_zero(feed_time1_min);
  lcd.print(" ");
  if (feeding1_complete == true)
  {
    lcd.write(0);
  }
  else
  {
    lcd.print(" ");
  }
  //Displaying second feed  time
  lcd.setCursor(0, 1);
  lcd.print("F2:");
  add_leading_zero(feed_time2_hour);
  lcd.print(":");
  add_leading_zero(feed_time2_min);
  lcd.print(" ");
  if (feeding2_complete == true)
  {
    lcd.write(0);
  }
  else
  {
    lcd.print(" ");
  }
}
//=====================================================
//  Displays the current time in the main menu
void display_current_time () {
  DateTime now = rtc.now();
  lcd.setCursor(11, 0);
  add_leading_zero(now.hour());
  lcd.print(":");
  add_leading_zero(now.minute());
}
//=====================================================
//  Displays the menu option for setting the first feed time
void display_set_feed_time1_menu  () {
  lcd.setCursor(2, 0);
  lcd.print("Menu Options");
  lcd.setCursor(0,  1);
  lcd.print("Set Feed Time 1");
}
//=====================================================
//  Displays the meny option for setting the second feed time
void display_set_feed_time2_menu  () {
  lcd.setCursor(2, 0);
  lcd.print("Menu Options");
  lcd.setCursor(0,  1);
  lcd.print("Set Feed Time 2");
}
//=====================================================
//  Displays the feeding portion in the main menu
void display_portion ()
{
  lcd.setCursor (12, 1);
  lcd.print("P:");
  add_leading_zero(portion);
}
//=====================================================
//  Starts the feeding process.

void startFeeding()
{
  // attach the servo  to the pin
  myservo.attach(SERVO_PIN);
  count = 1;
  hallSensorTime =  millis();
  // loop so that the servo runs until desired portion is reached
  while (count <= portion)
  {
    servoStart();
    if (hallSensorActivated  == true)
    {
      digitalWrite(HALL_LED_PIN,HIGH);
      count =  count + 1;
      //resetting for next interrupt
      hallSensorTime = millis();
      hallSensorActivated = false;
      digitalWrite(HALL_LED_PIN, LOW);
    }
    /* Moved the servo clockwise a bit to dislodge food stuck in the
       dispensing mechanism
    */
    if (millis() - hallSensorTime > rotationTime)
    {
      hall_sensor_fail = true;
      Serial.println("I'm in Jiggle");
      jiggle();
    }
  }
  // Keeps track of which feeding just happened  and writes it to EEPROM
  if ((feeding1_complete == false) && (feeding2_complete  == false))
  {
    feeding1_complete = true;
    EEPROM.write(4, feeding1_complete);
  }
  else if ((feeding1_complete == true) && (feeding2_complete == false))
  {
    feeding2_complete = true;
    EEPROM.write(5, feeding2_complete);
  }
  servoStop();
  digitalWrite(HALL_LED_PIN, LOW);
  /* Detaches the  servo from the pin so that it is no longer recieving a signal.
     You may have  to add a delay before this so the sensor stops when a magnet is over
     the  hall sensor. There was significant momentum left in my system that I did not need  it
  */
  myservo.detach();
  lcd.clear();
  delay_currentMillis = millis();
  while (millis() - delay_currentMillis <= delay_interval)
  {
    lcd.setCursor(2,  0);
    lcd.print ("Feeding Done");
  }
  lcd.clear();
}
//=====================================================

void  servoStart()
{
  myservo.write(180);
}
//=====================================================

void  servoStop()
{
  // this value will vary, you have to find it through trial  and error
  myservo.write(94);
}
//=====================================================
//  "jiggles" the servo in case food gets stuck

void jiggle()
{
  myservo.write(80);
  delay(30);
  myservo.write(93);
  delay(30);
  myservo.write(180);
}
//=====================================================
//  Writes the hour and minute valies set for 1st feeding to the EEPROM

void  write_feeding_time1 ()
{
  EEPROM.write(0, feed_time1_hour);
  EEPROM.write(1,  feed_time1_min);
}
//=====================================================
//  Writes the hour and minute values set for 2nd feeding to the EEPROM

void  write_feeding_time2 () {
  EEPROM.write(2, feed_time2_hour);
  EEPROM.write(3,  feed_time2_min);
}
//=====================================================
//  Writes portion value set to the EEPROM

void write_portion ()
{
  EEPROM.write(6,  portion);
}
//=====================================================
//  Reads the hour and minute values from 1st feed time from EEPROM

void get_feed_time1  ()
{
  feed_time1_hour = EEPROM.read(0);
  if (feed_time1_hour > 23) feed_time1_hour  = 0;
  feed_time1_min = EEPROM.read(1);
  if (feed_time1_min > 59) feed_time1_min  = 0;

}
//=====================================================
// Reads  the hour and minute values from 2nd feed time from EEPROM

void get_feed_time2  ()
{
  feed_time2_hour = EEPROM.read(2);
  if (feed_time2_hour > 23) feed_time2_hour  = 0;
  feed_time2_min = EEPROM.read(3);
  if (feed_time2_min > 59) feed_time2_min  = 0;
}
//=====================================================
// Reads  portion set value from EEPROM

void get_portion ()
{
  portion = EEPROM.read(6);
}
//=====================================================
//  Reads boolean value of whether or not feedings have occured from EEPROM

void  get_completed_feedings()
{
  feeding1_complete = EEPROM.read(4);
  feeding2_complete  = EEPROM.read(5);
}
//=====================================================
/*  Checks to see if the hall sensor has failed to read a magnet and
   blinks the  red LED
*/

void check_hall_sensor () {
  if (hall_sensor_fail == true)
  {
    if (blink_state == 0)
    {
      digitalWrite(HALL_LED_PIN, HIGH);
    }
    else
    {
      digitalWrite(HALL_LED_PIN, LOW);
    }
    blinkFunction();
  }
  else
  {
    digitalWrite(HALL_LED_PIN, LOW);
    hall_sensor_fail = false;
  }
}
//=====================================================
//  Checks if you push the manual feed button

void manual_feed_check () {
  if (manualFeed == true)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Manual Feeding");
    startFeeding();
    manualFeed = false;
  }
}
//=====================================================
// checks  to see if RTC is running

void check_rtc () {
  if (!rtc.isrunning())
  {
    led_blink();
  }
}
//=====================================================
/*  Blinks the red led when RTC has failed. Note: the led
   will be blinking at  a different rate than when the hall
   sensor fails
*/

void led_blink()
{
  unsigned long led_currentMillis = millis();
  if (led_currentMillis - led_previousMillis  >= interval_delay)
  {
    led_previousMillis = led_currentMillis;
    if  (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }
    digitalWrite(HALL_LED_PIN, ledState);
  }
}
//=====================================================
// Creates  the blinking effect when changing values

void blinkFunction()
{
  blink_currentMillis  = millis();

  if (blink_currentMillis - blink_previousMillis > blink_interval)
  {
    blink_previousMillis = blink_currentMillis;
    blink_state = !blink_state;
  }
}
//=====================================================
