#include <FTOLED.h>
#include <fonts/Arial14.h>
#include <fonts/Arial_Black_16.h>
#include <fonts/Droid_Sans_12.h>
#include <fonts/Droid_Sans_16.h>
#include <fonts/Droid_Sans_24.h>
#include <fonts/Droid_Sans_36.h>
#include <fonts/Droid_Sans_64.h>
#include <fonts/Droid_Sans_96.h>
#include <fonts/SystemFont5x7.h>

/* OLED DISPLAY PARAMETERS */
const byte pin_cs = 7;
const byte pin_dnc = 2;
const byte pin_reset = 3;
OLED oled(pin_cs, pin_dnc, pin_reset);
OLED_TextBox cycles(oled, 0, 64, 128, 64);
OLED_TextBox timecounter(oled, 0, 0, 128, 64);

const int ToggleUP_950 = 10;
const int ToggleDOWN_950 = 9;
const int ToggleUP_Freecurve = 13;
const int ToggleDOWN_Freecurve = 12;

void setup() {

  oled.begin();
  initTIM1interrupt();
  pinmodesetup();
  oled.selectFont(Droid_Sans_16);

}

void cycletest() {

  volatile static int16_t cyclecount = 0;
  volatile static int32_t timecount = 0;
  const int uptime = 5;
  const int downtime = 5;
  const int chargetime = 5;
  const int restuptime = 5;
  volatile static int cyclephase = 0;

  checkbuttonstates();

  switch (cyclephase) {

    /* Moving Up */
    case 0:
      digitalWrite(ToggleUP_950, HIGH);
      digitalWrite(ToggleDOWN_950, LOW);
      cyclephase = 0;

      if ( (timecount == uptime) ) {
        timecount = 0;
        cyclephase = 1;
      } 
      break;

    /* Rest at Up */
    case 1:
      digitalWrite(ToggleDOWN_950, LOW);
      digitalWrite(ToggleUP_950, LOW);
      cyclephase = 1;

      if ( (timecount == restuptime) ) {
        timecount = 0;
        cyclephase = 2;
      } 
      break;

  /* Moving Down */
    case 2:
      digitalWrite(ToggleUP_950, LOW);
      digitalWrite(ToggleDOWN_950, HIGH);
      cyclephase = 2;

      if ( (timecount == downtime) ) {
        timecount = 0;
        cyclephase = 3;
      } 
      break;

    /* Charging */
    case 3:
      digitalWrite(ToggleUP_950, LOW);
      digitalWrite(ToggleDOWN_950, LOW);
      cyclephase = 3;

      if ( (timecount == chargetime) ) {
        timecount = 0;
        cyclephase = 0;
        cyclecount++;
      } 
      break;
  }

  oledprint(cyclecount,cyclephase,timecount);

  timecount++;  /* time_count is a 1 Hz counter
                   time_count = 60 = 1 minute */

}

void initTIM1interrupt() {
  /* Initialize 1 Hz interrupt on Timer1 */
  cli();
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

void pinmodesetup() {
  pinMode(ToggleDOWN_950, OUTPUT);
  pinMode(ToggleUP_950, OUTPUT);
  pinMode(ToggleDOWN_Freecurve, OUTPUT);
  pinMode(ToggleUP_Freecurve, OUTPUT);

  digitalWrite(ToggleDOWN_Freecurve, LOW);
  digitalWrite(ToggleUP_Freecurve, LOW);
  digitalWrite(ToggleDOWN_950, LOW);
  digitalWrite(ToggleUP_950, LOW);
}

/* Input cylecount, cyclephase and timing count. No output */
void oledprint(int cyclecount, int cyclephase, int timecount) {
  
  /* Print cycle count and timing to OLED display */
  cycles.clear();
  cycles.println("Cycles: ");
  cycles.println(cyclecount, DEC);

  timecounter.clear();
  if (cyclephase == 0) {
    timecounter.println("Up Time"); 
  } else if (cyclephase == 1) {
    timecounter.println("Up Rest Time"); 
  } else if (cyclephase == 2) {
    timecounter.println("Down Time"); 
  } else if (cyclephase == 3) {
    timecounter.println("Charge Time"); 
  }

  timecounter.println(timecount, DEC);
  
}

void checkbuttonstates() {
  
}


ISR(TIMER1_COMPA_vect) {
  //timer1 interrupt 1Hz toggles pin 13 (LED)
  //generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)

  cycletest();

}

void loop() {
  // put your main code here, to run repeatedly:

}
