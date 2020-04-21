#include <EEPROM.h>
#include "EEPROMAnything.h"

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

/* hardware buttons */
const int upbutton = 6;
const int downbutton = 5;
const int modebutton = 4;

/* input from lift handset PCB */
const int motorinputup = 8;
const int motorinputdown = 9;

volatile static int buttonpressed;
volatile static int cyclephase = 0;
volatile static int liftmode = 0;   // 0 = Manual, 1 = Auto (cycle test), 2 = Program Mode
volatile static int liftmodeprev = 0;
volatile static int32_t timecount = 0;

const int uptime = 2;
const int downtime = 2;
const int chargetime = 2;
const int restuptime = 2;
const int restdowntime = 2;

volatile static int modedebounce = 0;
  volatile static int debouncelimit = 60;

void setup() {

  oled.begin();
  initTIMinterrupts();
  pinmodesetup();
  oled.selectFont(Droid_Sans_16);
  Serial.begin(9600);  

}


void initTIMinterrupts() {
  
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

  /* Initialize 125 Hz interrupt on Timer0 */
  TCCR0A = 0; // set entire TCCR0A register to 0
  TCCR0B = 0; // same for TCCR0B
  TCNT0  = 0; // initialize counter value to 0
  // set compare match register for 125 Hz increments
  OCR0A = 124; // = 16000000 / (1024 * 125) - 1 (must be <256)
  // turn on CTC mode
  TCCR0B |= (1 << WGM02); 
  // Set CS02 and CS00 bits for 1024 prescaler
  TCCR0B |= (1 << CS02) | (1 << CS00); 
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);
  
  sei();
}


void pinmodesetup() {
  pinMode(upbutton,INPUT_PULLUP);
  pinMode(downbutton,INPUT_PULLUP);
  pinMode(modebutton,INPUT_PULLUP);

  pinMode(motorinputup, OUTPUT);
  pinMode(motorinputdown, OUTPUT);

  digitalWrite(motorinputup,LOW);
  digitalWrite(motorinputdown,LOW);
}


void cycletest() {

  volatile static int16_t cyclecount = 0;
  const int uptime = 2;
  const int downtime = 2;
  const int chargetime = 2;
  const int restuptime = 2;
  const int restdowntime = 2;

  switch (cyclephase) {

    /* Moving Up */
    case 0:
      cyclephase = 0;

      if ( (timecount >= uptime) ) {
        timecount = 0;
        cyclephase = 1;
      } 
      break;

    /* Rest at Up */
    case 1:
      cyclephase = 1;

      if ( (timecount >= restuptime) ) {
        timecount = 0;
        cyclephase = 2;
      } 
      break;

  /* Moving Down */
    case 2:
      cyclephase = 2;

      if ( (timecount >= downtime) ) {
        timecount = 0;
        cyclephase = 3;
      } 
      break;

    /* Down Rest */
    case 3:
      cyclephase = 3;
    
      if ( (timecount >= restdowntime) ) {
        timecount = 0;
        cyclephase = 4;
      } 
      break;

    /* Charging */
    case 4:
      cyclephase = 4;

      if ( (timecount >= chargetime) ) {
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
    timecounter.println("Down Rest Time"); 
  } else if (cyclephase == 4) {
    timecounter.println("Charge Time"); 
  }

  timecounter.println(timecount, DEC);
  
}

int checkbuttonstates() {
  volatile static int up;
  volatile static int down;
  volatile static int mode;

  volatile static int updebounce = 0;
  volatile static int downdebounce = 0;
  volatile static int programdebounce = 0;
  
  up = digitalRead(upbutton);
  down = digitalRead(downbutton);
  mode = digitalRead(modebutton);

  if (!up && down && mode) {
    updebounce++;
    if (updebounce > debouncelimit) {
      buttonpressed = 1;
    }
  } else if (!down && up && mode) {
    downdebounce++;
    if (downdebounce > debouncelimit) {
      buttonpressed = 2;
    }
  } else if (!mode && up && down) {
    modedebounce++;
    if (modedebounce > debouncelimit) {
      timecount = 0;
      cyclephase = 0;
      modedebounce = -2*debouncelimit;
      buttonpressed = 3;
      
    }

  } else if (!up && !down && mode) {
    programdebounce++;
    if (programdebounce > debouncelimit) {
      programdebounce = -debouncelimit;
      buttonpressed = 4;
    }
    
  } else {
    buttonpressed = 0;
    updebounce = 0;
    downdebounce = 0;
    modedebounce = 0;
    programdebounce = 0;
  }

  return(buttonpressed);
}

/* MAIN CONTROL LOOP */
void controlloop() {
  volatile static int loopcounter = 0;

  /* Manual Mode */
  if (liftmode == 0) {

    if (checkbuttonstates() == 1) {
      digitalWrite(motorinputup, HIGH);
      digitalWrite(motorinputdown, LOW);
      Serial.println("UP");

    } else if (checkbuttonstates() == 2) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, HIGH);
      Serial.println("Down");

    } else if (checkbuttonstates() == 3) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
      liftmode = 5;
      liftmodeprev = 0;
      cycles.clear();
      timecounter.clear();
      cycles.println("AUTO MODE");

    } else if (checkbuttonstates() == 4) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
      Serial.println("Program");

    } else {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
    }

    /* Auto Mode */    
  } else if (liftmode == 1) {

    if (checkbuttonstates() == 3) {
      liftmode = 5;
      liftmodeprev = 1;
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
      cycles.clear();
      timecounter.clear();
      modedebounce = 0;
      cycles.println("MANUAL MODE");
    }

    if (cyclephase == 0) {
        digitalWrite(motorinputup, HIGH);
        digitalWrite(motorinputdown, LOW);
    } else if (cyclephase == 1) {
        digitalWrite(motorinputdown,LOW);
        digitalWrite(motorinputup,LOW);
    } else if (cyclephase == 2) {
        digitalWrite(motorinputup,LOW);
        digitalWrite(motorinputdown,HIGH);
    } else if (cyclephase == 3 || cyclephase == 4) {
        digitalWrite(motorinputup,LOW);
        digitalWrite(motorinputdown,LOW);
    } 
    
  } else if (liftmode == 2) {
      programmodeUI();    
  } else if (liftmode == 5) {
    loopcounter++;
    if (loopcounter > 100) {
      if (liftmodeprev == 0) {
        liftmode = 1;
        Serial.println("changed to Auto");
      } else if (liftmodeprev == 1) {
        liftmode = 0;
        Serial.println("changed to Manual");
      }
      modedebounce = 0;
      loopcounter = 0;
    }
  }
}


void programmodeUI() {

  volatile static int programcounter = 0;
  volatile static int programpage = 0;
  volatile static int intropagetime = 0;
  
  digitalWrite(motorinputup,LOW);
  digitalWrite(motorinputdown,LOW);

    if (checkbuttonstates() == 4) {
      liftmode = 0;
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
    }
  
//  cycles.clear();
//  cycles.println("Cycles: ");
//  cycles.println(cyclecount, DEC);
//  timecounter.println(timecount, DEC);
  
  switch (programpage) {
  
  case 0:
    programcounter++;
    cycles.clear();
    timecounter.clear();
  
    if (programcounter >= intropagetime) {
      cycles.println("");
      cycles.println("PROGRAM MODE");
      programpage = 1;
      programcounter = 0;
    }
  
  break;
  
  case 1:
    cycles.clear();
    timecounter.clear();
    
    cycles.println("UP TIME:");
    cycles.println(uptime, DEC);

    
    
  break;
  
  
  
  }
  
}

ISR(TIMER1_COMPA_vect) {
  //timer1 interrupt 1Hz toggles pin 13 (LED)
  //generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)

  if (liftmode == 1) {
    cycletest();
  }
  
}

ISR(TIMER0_COMPA_vect) {
  //timer0 interrupt 125Hz 

  controlloop();
}

void loop() {
//  Serial.println(modedebounce);
//  Serial.println(liftmode);
}
