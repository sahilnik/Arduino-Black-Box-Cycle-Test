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
const int motorinputup = A1;
const int motorinputdown = A2;

/* Relay output */
const int relaysignal = A0;

volatile static int buttonpressed;
volatile static int cyclephase = 0;
volatile static int liftmode = 5;   // 0 = Manual, 1 = Auto (cycle test), 2 = Program Mode
volatile static int liftmodenext = 0;
volatile static int32_t timecount = 0;

volatile static int uptime = 18; // 18
volatile static int newuptime = uptime;
volatile static int downtime = 10; // 10
volatile static int newdowntime = downtime;
volatile static int chargetime = 15; // 15
volatile static int newchargetime = 60*chargetime;
volatile static int restuptime = 3; // 3
volatile static int newrestuptime = restuptime;
volatile static int restdowntime = 5; // 5
volatile static int newrestdowntime = restdowntime;
volatile static int chargetoggle = 0;
volatile static int cycleresttime = 10; // 10
volatile static int newcycleresttime = 60*cycleresttime;

volatile static int modedebounce = 0;
volatile static int debouncelimit = 60;

volatile static int programcounter = 0;

void setup() {

  oled.begin();
  initTIMinterrupts();
  pinmodesetup();
  oled.selectFont(Droid_Sans_16);
  //  Serial.begin(9600);

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
  // enable timer1 compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  /* Initialize 100 Hz interrupt on Timer0 */
  TCCR0A = 0;
  TCCR0B = 0;
  /* Initialize 8 bit count register as 0 */
  TCNT0 = 0;
  /* Set TOP value to OCR0A
    OCR0A = (16,000,000 / N * f) - 1
    Therefore, 77.125 is OCR0A needed for 100 Hz frequency on Timer 0 */
  OCR0A = 155;
  /* Set to CTC (Clear timer on compare match mode) */
  TCCR0A |= (1 << WGM01);
  /* Set prescaler to 1024 */
  TCCR0B |= (1 << CS02) | (1 << CS00);
  /* enable timer0 compare interrupt */
  TIMSK0 |= (1 << OCIE0A);

  sei();
}


void pinmodesetup() {
  pinMode(upbutton, INPUT_PULLUP);
  pinMode(downbutton, INPUT_PULLUP);
  pinMode(modebutton, INPUT_PULLUP);

  pinMode(motorinputup, OUTPUT);
  pinMode(motorinputdown, OUTPUT);

  digitalWrite(motorinputup, LOW);
  digitalWrite(motorinputdown, LOW);

  pinMode(relaysignal, OUTPUT);
  digitalWrite(relaysignal, LOW);
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

      if ( (timecount >= newuptime) ) {
        timecount = 0;
        cyclephase = 1;
      }
      break;

    /* Rest at Up */
    case 1:
      cyclephase = 1;

      if ( (timecount >= newrestuptime) ) {
        timecount = 0;
        cyclephase = 2;
      }
      break;

    /* Moving Down */
    case 2:
      cyclephase = 2;

      if ( (timecount >= newdowntime) ) {
        timecount = 0;
        cyclephase = 3;
      }
      break;

    /* Down Rest */
    case 3:
      cyclephase = 3;

      if ( (timecount >= newrestdowntime) ) {

        if (chargetoggle == 0) {
          timecount = 0;
          cyclephase = 5;
          cyclecount++;
        } else if (chargetoggle == 1) {
          timecount = 0;
          cyclephase = 4;
          cyclecount++;
        }
      }
      break;

    /* Charging */
    case 4:
      cyclephase = 4;

      if ( (timecount >= (newchargetime - 5) ) ) {
        digitalWrite(relaysignal, LOW);
      } else {
        digitalWrite(relaysignal, HIGH);
      }

      if ( (timecount >= newchargetime) ) {
        timecount = 0;
        cyclephase = 0;
        digitalWrite(relaysignal, LOW);
      }
      break;

    /* Cycle rest */
    case 5:
      cyclephase = 5;
      if ( (timecount >= newcycleresttime) ) {
        timecount = 0;
        cyclephase = 0;
      }
      break;

  }

  oledprint(cyclecount, cyclephase, timecount);

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
    timecounter.println("Up Time:");
  } else if (cyclephase == 1) {
    timecounter.println("Up Rest Time:");
  } else if (cyclephase == 2) {
    timecounter.println("Down Time:");
  } else if (cyclephase == 3) {
    timecounter.println("Down Rest Time:");
  } else if (cyclephase == 4) {
    timecounter.println("Charge Time:");
  } else if (cyclephase == 5) {
    timecounter.println("Rest Time:");   
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
      modedebounce = -2 * debouncelimit;
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

  return (buttonpressed);
}

/* MAIN CONTROL LOOP */
void controlloop() {
  
  volatile static int loopcounter = 0;

  /* Manual Mode */
  if (liftmode == 0) {

    if (checkbuttonstates() == 1) {
      digitalWrite(motorinputup, HIGH);
      digitalWrite(motorinputdown, LOW);

    } else if (checkbuttonstates() == 2) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, HIGH);

    } else if (checkbuttonstates() == 3) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
      liftmode = 5;
      liftmodenext = 1;
      cycles.clear();
      timecounter.clear();
      cycles.println("AUTO MODE");

    } else if (checkbuttonstates() == 4) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
      liftmode = 5;
      liftmodenext = 2;
      cycles.clear();
      timecounter.clear();
      cycles.println("");
      cycles.println("PROGRAM MODE");
      loopcounter = 0;

    } else {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
    }

    /* Auto Mode */
  } else if (liftmode == 1) {

    if (checkbuttonstates() == 3) {
      liftmode = 5;
      liftmodenext = 0;
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
      digitalWrite(relaysignal, LOW);
      cycles.clear();
      timecounter.clear();
      modedebounce = 0;
      cycles.println("MANUAL MODE");
      loopcounter = 0;
    }

    if (cyclephase == 0) {
      digitalWrite(motorinputup, HIGH);
      digitalWrite(motorinputdown, LOW);
    } else if (cyclephase == 1) {
      digitalWrite(motorinputdown, LOW);
      digitalWrite(motorinputup, LOW);
    } else if (cyclephase == 2) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, HIGH);
    } else if (cyclephase == 3) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
    } else if (cyclephase == 4) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
    } else if (cyclephase == 5) {
      digitalWrite(motorinputup, LOW);
      digitalWrite(motorinputdown, LOW);
      digitalWrite(relaysignal, LOW);
    }

  } else if (liftmode == 2) {
    programmodeUI();
  } else if (liftmode == 5) {
    digitalWrite(motorinputup, LOW);
    digitalWrite(motorinputdown, LOW);
    digitalWrite(relaysignal, LOW);
    loopcounter++;
    if (loopcounter > 100) {
      if (liftmodenext == 1) {
        liftmode = 1;
        loopcounter = 0;
        
      } else if (liftmodenext == 0) {
        liftmode = 0;
        loopcounter = 0;
        cycles.clear();
        timecounter.clear();
        modedebounce = 0;
        cycles.println("MANUAL MODE");

      } else if (liftmodenext == 2) {
        liftmode = 2;
        loopcounter = 0;
        programcounter = 0;
      }
      modedebounce = 0;
      loopcounter = 0;
    }
  }
}


void programmodeUI() {

  volatile static int programpage = 0;
  volatile static int intropagetime = 100;
  volatile static int pauseflag = 0;
  volatile static int pausecount = 0;

  if (pauseflag == 1) {
    pausecount++;
  }
  if (pausecount >= 50) {
    pausecount = 0;
    pauseflag = 0;
  }

  digitalWrite(motorinputup, LOW);
  digitalWrite(motorinputdown, LOW);

  if (checkbuttonstates() == 4) {
    digitalWrite(motorinputup, LOW);
    digitalWrite(motorinputdown, LOW);
    uptime = uptime;
    liftmode = 5;
    liftmodenext = 0;
    cycles.clear();
    timecounter.clear();
    programpage = 0;
    cycles.println("MANUAL MODE");

  } else if (checkbuttonstates() == 3) {
    if (programpage == 0 && pauseflag == 0) {
      programpage = 1;
      pauseflag = 1;
      cycles.clear();
      timecounter.clear();
      cycles.println("UP TIME:");
      cycles.print(uptime, DEC);
      cycles.println(" Seconds");

    } else if (programpage == 1 && pauseflag == 0) {
      programpage = 2;
      pauseflag = 1;
      cycles.clear();
      cycles.println("UP STOP:");
      cycles.print(restuptime, DEC);
      cycles.println(" Seconds");

    } else if (programpage == 2 && pauseflag == 0) {
      programpage = 3;
      pauseflag = 1;
      cycles.clear();
      cycles.println("DOWN TIME:");
      cycles.print(downtime, DEC);
      cycles.println(" Seconds");

    } else if (programpage == 3 && pauseflag == 0) {
      programpage = 4;
      pauseflag = 1;
      cycles.clear();
      cycles.println("DOWN STOP:");
      cycles.print(restdowntime, DEC);
      cycles.println(" Seconds");

    } else if (programpage == 4 && pauseflag == 0) {
      programpage = 6;
      pauseflag = 1;

      cycles.clear();
      cycles.println("CHARGING?:");
      if (chargetoggle == 0) {
        cycles.println("NO");
      } else if (chargetoggle == 1) {
        cycles.print("YES");
      }
      
    } else if (programpage == 6 && pauseflag == 0) {
      if (chargetoggle == 0) {
        programpage = 7;
        pauseflag = 1;
        cycles.clear();
        cycles.println("REST TIME:");
        cycles.print(cycleresttime, DEC);
        cycles.println(" Minutes");
        
      } else if (chargetoggle == 1) {
        programpage = 5;
        pauseflag = 1;
        cycles.clear();
        cycles.println("CHARGE TIME:");
        cycles.print(chargetime, DEC);
        cycles.println(" Minutes");
      }
      
    } else if ((programpage == 7 || programpage == 5) && pauseflag == 0) {
      programpage = 0;
      liftmode = 5;
      liftmodenext = 0;
      pauseflag = 0;
    }
  }

  switch (programpage) {

    case 0:
      programcounter++;
      if (programcounter >= intropagetime) {
        programpage = 1;
        programcounter = 0;
        uptime = newuptime;
        cycles.clear();
        timecounter.clear();
        cycles.println("UP TIME:");
        cycles.print(uptime, DEC);
        cycles.println(" Seconds");
      }

      break;

    case 1:

      if (checkbuttonstates() == 1) {
        uptime++;
        newuptime = uptime;
        if (uptime <= 0) {
          uptime = 0;
        }
        cycles.clear();
        cycles.println("UP TIME:");
        cycles.print(uptime, DEC);
        cycles.println(" Seconds");

      } else if (checkbuttonstates() == 2) {
        uptime--;
        newuptime = uptime;
        if (uptime <= 0) {
          uptime = 0;
        }
        cycles.clear();
        cycles.println("UP TIME:");
        cycles.print(uptime, DEC);
        cycles.println(" Seconds");
      }

      break;

    case 2:

      if (checkbuttonstates() == 1) {
        restuptime++;
        newrestuptime = restuptime;
        if (restuptime <= 0) {
          restuptime = 0;
        }
        cycles.clear();
        cycles.println("UP STOP TIME:");
        cycles.print(restuptime, DEC);
        cycles.println(" Seconds");

      } else if (checkbuttonstates() == 2) {
        restuptime--;
        newrestuptime = restuptime;
        if (restuptime <= 0) {
          restuptime = 0;
        }
        cycles.clear();
        cycles.println("UP STOP TIME:");
        cycles.print(restuptime, DEC);
        cycles.println(" Seconds");
      }

      break;

    case 3:

      if (checkbuttonstates() == 1) {
        downtime++;
        newdowntime = downtime;
        if (downtime <= 0) {
          downtime = 0;
        }
        cycles.clear();
        cycles.println("DOWN TIME:");
        cycles.print(downtime, DEC);
        cycles.println(" Seconds");
      
      } else if (checkbuttonstates() == 2) {
        downtime--;
        newdowntime = downtime;
        if (downtime <= 0) {
          downtime = 0;
        }
        cycles.clear();
        cycles.println("DOWN TIME:");
        cycles.print(downtime, DEC);
        cycles.println(" Seconds");
      }

      break;

    case 4:

      if (checkbuttonstates() == 1) {
        restdowntime++;
        newrestdowntime = restdowntime;
        if (restdowntime <= 0) {
          restdowntime = 0;
        }
        cycles.clear();
        cycles.println("DOWN STOP TIME:");
        cycles.print(restdowntime, DEC);
        cycles.println(" Seconds");

      } else if (checkbuttonstates() == 2) {
        restdowntime--;
        newrestdowntime = restdowntime;
        if (restdowntime <= 0) {
          restdowntime = 0;
        }
        cycles.clear();
        cycles.println("DOWN STOP TIME:");
        cycles.print(restdowntime, DEC);
        cycles.println(" Seconds");
        
      }

      break;

    case 5:

      if (checkbuttonstates() == 1) {
        chargetime++;
        newchargetime = 60*chargetime;
        if (chargetime <= 0) {
          chargetime = 0;
        }
        cycles.clear();
        cycles.println("CHARGE TIME:");
        cycles.print(chargetime, DEC);
        cycles.println(" Minutes");
        
      } else if (checkbuttonstates() == 2) {
        chargetime--;
        newchargetime = 60*chargetime;
        if (chargetime <= 0) {
          chargetime = 0;
        }
        cycles.clear();
        cycles.println("CHARGE TIME:");
        cycles.print(chargetime, DEC);
        cycles.println(" Minutes");
      }

      break;

    case 6:

      if (checkbuttonstates() == 1) {
        chargetoggle = 1;
        cycles.clear();
        cycles.println("CHARGING?:");
        cycles.println("YES");
      } else if (checkbuttonstates() == 2) {
        chargetoggle = 0;
        cycles.clear();
        cycles.println("CHARGING?:");
        cycles.println("NO");
      }

      break;

    case 7:

      if (checkbuttonstates() == 1) {
        cycleresttime++;
        newcycleresttime = 60*cycleresttime;
        if (cycleresttime <= 0) {
          cycleresttime = 0;
        }
        cycles.clear();
        cycles.println("REST TIME:");
        cycles.print(cycleresttime, DEC);
        cycles.println(" Minutes");

      } else if (checkbuttonstates() == 2) {
        cycleresttime--;
        newcycleresttime = 60*cycleresttime;
        if (cycleresttime <= 0) {
          cycleresttime = 0;
        }
        cycles.clear();
        cycles.println("REST TIME:");
        cycles.print(cycleresttime, DEC);
        cycles.println(" Minutes");
      }

      break;

  }

}

ISR(TIMER1_COMPA_vect) {
  //timer1 interrupt 1Hz toggles pin 13 (LED)
  //generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)
  //Serial.println("1 Hz loop");

  if (liftmode == 1) {
    cycletest();
  }

}

ISR(TIMER0_COMPA_vect) {
  //timer0 interrupt 100Hz

  controlloop();


}

void loop() {

}
