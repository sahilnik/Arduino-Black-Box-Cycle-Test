const int ToggleUP_950 = 10;
const int ToggleDOWN_950 = 9;
const int ToggleUP_Freecurve = 13;
const int ToggleDOWN_Freecurve = 12;


void setup() {
  // put your setup code here, to run once:
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
  
  pinMode(ToggleDOWN_950, OUTPUT);
  pinMode(ToggleUP_950, OUTPUT);
  pinMode(ToggleDOWN_Freecurve, OUTPUT);
  pinMode(ToggleUP_Freecurve, OUTPUT);
  
  digitalWrite(ToggleDOWN_Freecurve, LOW);
  digitalWrite(ToggleUP_Freecurve, LOW);
  digitalWrite(ToggleDOWN_950, LOW);
  digitalWrite(ToggleUP_950, LOW);
}

void cycle_test_950() {

  // volatile static int16_t cycle_count_950 = 250;
  volatile static int32_t time_count_950 = 0;
  const int Up_time_950 = 17;
  const int Down_time_950 = 38;
  const int Rest_time_950 = 518;
  const int Pause_time_950 = 21;

  time_count_950++;  // time_count is a 1 Hz counter
  // time_count = 60 = 1 minute
  // First 17 seconds move up
  if (time_count_950 < Up_time_950) {
    digitalWrite(ToggleUP_950, HIGH);
    digitalWrite(ToggleDOWN_950, LOW);
  } // Pause for 4 seconds
    else if (time_count_950 >= Up_time_950 && time_count_950 < Pause_time_950) {
    digitalWrite(ToggleDOWN_950, LOW);
    digitalWrite(ToggleUP_950, LOW);
  } // Move down for 17 seconds
    else if (time_count_950 >= Pause_time_950 && time_count_950 < Down_time_950) {
    digitalWrite(ToggleUP_950, LOW);
    digitalWrite(ToggleDOWN_950, HIGH);    
  } // Rest for 8 mins
    else if (time_count_950 >= Down_time_950 && time_count_950 < Rest_time_950) {
    digitalWrite(ToggleUP_950, LOW);
    digitalWrite(ToggleDOWN_950, LOW);    
  } // Reset master counter after 8 mins to restart
    else if (time_count_950 >= Rest_time_950) {
    time_count_950 = 0;
    // cycle_count_950++;
  }

}

void Freecurve_cycle_test() {

  // volatile static int16_t cycle_count_Freecurve = 0;
  volatile static int32_t time_count_Freecurve = 0;
  const int Up_time_Freecurve = 84;
  const int Down_time_Freecurve = 40;
  const int Rest_time_Freecurve = 685;
  const int Pause_time_Freecurve = 43;

  time_count_Freecurve++;  // time_count is a 1 Hz counter
  // time_count = 60 = 1 minute
  // First 40 seconds move down
  if (time_count_Freecurve < Down_time_Freecurve) {
    digitalWrite(ToggleUP_Freecurve, LOW);
    digitalWrite(ToggleDOWN_Freecurve, HIGH);  
  } // Pause for 2 seconds
    else if (time_count_Freecurve >= Down_time_Freecurve && time_count_Freecurve < Pause_time_Freecurve) {
    digitalWrite(ToggleUP_Freecurve, LOW);
    digitalWrite(ToggleDOWN_Freecurve, LOW); 
  } // Move up for 40 seconds
    else if (time_count_Freecurve >= Pause_time_Freecurve && time_count_Freecurve < Up_time_Freecurve) {
    digitalWrite(ToggleUP_Freecurve, HIGH);
    digitalWrite(ToggleDOWN_Freecurve, LOW);  
  } // Rest for 8 mins
    else if (time_count_Freecurve >= Up_time_Freecurve && time_count_Freecurve < Rest_time_Freecurve) {
    digitalWrite(ToggleUP_Freecurve, LOW);
    digitalWrite(ToggleDOWN_Freecurve, LOW);
  } // Reset master counter after 8 mins to restart
    else if (time_count_Freecurve >= Rest_time_Freecurve) {
    time_count_Freecurve = 0;
    // cycle_count_Freecurve++;
  }

}


ISR(TIMER1_COMPA_vect)
{
  //timer1 interrupt 1Hz toggles pin 13 (LED)
  //generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)

  cycle_test_950();
  Freecurve_cycle_test();

}

void loop() {
  // put your main code here, to run repeatedly:

}
