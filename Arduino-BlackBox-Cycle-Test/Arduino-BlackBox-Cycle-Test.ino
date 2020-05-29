/* hardware buttons */
const int upbutton = 8;
const int downbutton = 7;

/* Software buttons */
const int modebutton;

/* Relay output */
const int relayup = 5;
const int relaydown = 4;

volatile static int buttonpressed;
volatile static int cyclephase = 0;
volatile static int liftmode = 5;   // 0 = Manual, 1 = Auto (cycle test), 2 = Program Mode
volatile static int liftmodenext = 0;
volatile static int32_t timecount = 0;

volatile static int uptime = 27; // 18
volatile static int downtime = 15; // 10
volatile static int restuptime = 2; // 3
volatile static int restdowntime = 2; // 5
volatile static int cycleresttime = 420; // 10

volatile static int modedebounce = 0;
volatile static int debouncelimit = 60;

void setup() {
  
  initTIMinterrupts();
  pinmodesetup();
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
  
  pinMode(relayup, OUTPUT);
  digitalWrite(relayup, LOW);
  pinMode(relaydown, OUTPUT);
  digitalWrite(relaydown, LOW);
}

void cycletest() {

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
        cyclephase = 5;
      }
      break;

    /* Cycle rest */
    case 5:
      cyclephase = 5;
      if ( (timecount >= cycleresttime) ) {
        timecount = 0;
        cyclephase = 0;
      }
      break;
  }

  timecount++;  /* time_count is a 1 Hz counter
                   time_count = 60 = 1 minute */
}

int checkbuttonstates() {
  volatile static int up;
  volatile static int down;

  volatile static int updebounce = 0;
  volatile static int downdebounce = 0;
  volatile static int programdebounce = 0;

  up = digitalRead(upbutton);
  down = digitalRead(downbutton);

  if (!up && down) {
    updebounce++;
    if (updebounce > debouncelimit) {
      buttonpressed = 1;
      
    }
  } else if (!down && up) {
    downdebounce++;
    if (downdebounce > debouncelimit) {
      buttonpressed = 2;
      
    }
  } else if (!up && !down) {
    modedebounce++;
    if (modedebounce > debouncelimit) {
      timecount = 0;
      cyclephase = 0;
      modedebounce = -2 * debouncelimit;
      buttonpressed = 3;

    }

  } else {
    buttonpressed = 0;
    updebounce = 0;
    downdebounce = 0;
    modedebounce = 0;
  }

  return (buttonpressed);
}

/* MAIN CONTROL LOOP */
void controlloop() {

  volatile static int loopcounter = 0;

  /* Manual Mode */
  if (liftmode == 0) {

    if (checkbuttonstates() == 1) {
      digitalWrite(relayup, HIGH);
      digitalWrite(relaydown, LOW);

    } else if (checkbuttonstates() == 2) {
      digitalWrite(relayup, LOW);
      digitalWrite(relaydown, HIGH);

    } else if (checkbuttonstates() == 3) {
      digitalWrite(relayup, LOW);
      digitalWrite(relaydown, LOW);
      liftmode = 5;
      liftmodenext = 1;

    } else {
      digitalWrite(relayup, LOW);
      digitalWrite(relaydown, LOW);
    }

    /* Auto Mode */
  } else if (liftmode == 1) {

    if (checkbuttonstates() == 3) {
      liftmode = 5;
      liftmodenext = 0;
      digitalWrite(relayup, LOW);
      digitalWrite(relaydown, LOW);
      modedebounce = 0;
      loopcounter = 0;
    }

    if (cyclephase == 0) {
      digitalWrite( relayup, HIGH);
      digitalWrite( relaydown, LOW);
    } else if (cyclephase == 1) {
      digitalWrite( relaydown, LOW);
      digitalWrite( relayup, LOW);
    } else if (cyclephase == 2) {
      digitalWrite( relayup, LOW);
      digitalWrite( relaydown, HIGH);
    } else if (cyclephase == 3) {
      digitalWrite( relayup, LOW);
      digitalWrite( relaydown, LOW);
    } else if (cyclephase == 5) {
      digitalWrite( relayup, LOW);
      digitalWrite( relaydown, LOW);
    }

  } else if (liftmode == 5) {
    digitalWrite( relayup, LOW);
    digitalWrite( relaydown, LOW);

    loopcounter++;
    if (loopcounter > 100) {
      if (liftmodenext == 1) {
        liftmode = 1;
        loopcounter = 0;
        
      } else if (liftmodenext == 0) {
        liftmode = 0;
        loopcounter = 0;
        modedebounce = 0;
      }
      
      modedebounce = 0;
      loopcounter = 0;
    }
  }
}

ISR(TIMER1_COMPA_vect) {
  //timer1 interrupt 1Hz toggles pin 13 (LED)
  //generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)
  //Serial.println("1 Hz loop");
  volatile static int counter = 0;
  
  if (liftmode == 1) {
    cycletest();
  }

}

ISR(TIMER0_COMPA_vect) {
  //timer0 interrupt 100Hz

  controlloop();
//  Serial.println(cyclephase);
//  Serial.println("cycle phase = ", cyclephase);

}

void loop() {

}
