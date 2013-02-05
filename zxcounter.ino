// ZX Counter sketch for DIYGeigerCounter Kit
#include <stdio.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

#define VERSION           1.2      // version of this software
#define RATIO             175.43   // CPM to uSv converion ratio

#define PERIOD_1S         1000
#define PERIOD_5S         5000
#define PERIOD_10S        10000

#define BUTTON_WAIT       2000     // wait until button pressed
#define REFRESH_PERIOD    1000     // display refresh period
#define BUTTON_PERIOD     500      // button check period
#define DEBOUNCE_PERIOD   50       // button debounce period

#define BLINK_DELAY       REFRESH_PERIOD / 2

#define COUNTS_5S_LEN     5        // 5 sec stats array length (5 data points per second)
#define COUNTS_10S_LEN    10       // 10 sec stats array lenght (10 data points per second) 
#define COUNTS_30S_LEN    30       // 30 sec stats array lenght (10 data points per second) 
#define COUNTS_1M_LEN     60       // 1 min stats array lenght (60 data points per second) 
#define COUNTS_5M_LEN     60       // 5 min stats array lenght (60 data points per 5 seconds) 
#define COUNTS_10M_LEN    60       // 10 min stats array lenght (60 data points per 10 seconds) 

#define CPM_LIMIT_1S      7500     // min CPM to display 1 sec stats
#define CPM_LIMIT_5S      1500     // min CPM to display 5 sec stats
#define CPM_LIMIT_10S     300      // min CPM to display 10 sec stats     
#define CPM_LIMIT_30S     60       // min CPM to displat 30 sec stats

#define MAX_TIME          8640000  // limit time to 100 days

#define MODE_BUTTON_PIN   10       // button to toggle display mode
#define INT_BUTTON_PIN    11       // button to togle interval

#define ALARM_PIN         15       // Outputs HIGH when alarm triggered
#define ALARM_ADDR        8        // adress of alarm setting in EEPROM
#define ALARM_USV         5.       // default uSv
#define ALARM_MAX         100.     // max uSv can be set

#define MODE_AUTO_STATS   0
#define MODE_ALL_STATS    1
#define MODE_MAX          2
#define MODE_DOSE         3

#define MODE_1S_STATS     4
#define MODE_5S_STATS     5
#define MODE_10S_STATS    6
#define MODE_30S_STATS    7
#define MODE_1M_STATS     8
#define MODE_5M_STATS     9
#define MODE_10M_STATS    10

// counts within 1 sec, 5 sec and 10 sec
volatile unsigned long count_1s;
volatile unsigned long count_5s;
volatile unsigned long count_10s;

// default startup mode
byte mode = MODE_AUTO_STATS;

// alarm setting
float alarm_usv = 0;
boolean alarm_enabled = false;

// total counts
unsigned long total;

// current CPS
unsigned long counts_1s;

// 5 sec, 10 sec, 30 sec, 1 min, 5 min, 10 min data arrays
unsigned long counts_5s[COUNTS_5S_LEN];
unsigned long counts_10s[COUNTS_10S_LEN];
unsigned long counts_30s[COUNTS_30S_LEN];
unsigned long counts_1m[COUNTS_1M_LEN];
unsigned long counts_5m[COUNTS_5M_LEN];
unsigned long counts_10m[COUNTS_10M_LEN];

// current time
unsigned long time;

// max CPM count value
unsigned long max_cpm;

// max CPM count time
unsigned long max_time;

// last 1 sec, 5 sec, 10 sec check times
unsigned long count_1s_time;
unsigned long count_5s_time;
unsigned long count_10s_time;

// last display refresh time
unsigned long last_refresh;

// last button check time
unsigned long last_button_time;

// counts array pointers
byte counts_5s_index;
byte counts_10s_index;
byte counts_30s_index;
byte counts_1m_index;
byte counts_5m_index;
byte counts_10m_index;

// data ready flags
boolean counts_5s_ready;
boolean counts_10s_ready;
boolean counts_30s_ready;
boolean counts_1m_ready;
boolean counts_5m_ready; 
boolean counts_10m_ready;

// custom characters used for analog bar
byte bar_0[8] = {0x00, 0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00}; // blank
byte bar_1[8] = {0x10, 0x10, 0x18, 0x18, 0x18, 0x10, 0x10, 0x00}; // 1 bar
byte bar_2[8] = {0x18, 0x18, 0x1c, 0x1c, 0x1c, 0x18, 0x18, 0x00}; // 2 bars
byte bar_3[8] = {0x1C, 0x1C, 0x1e, 0x1e, 0x1e, 0x1C, 0x1C, 0x00}; // 3 bars
byte bar_4[8] = {0x1E, 0x1E, 0x1f, 0x1f, 0x1f, 0x1E, 0x1E, 0x00}; // 4 bars
byte bar_5[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00}; // 5 bars

// instantiate the library and pass pins for (RS, Enable, D4, D5, D6, D7)
// default layout for the Geiger board 
LiquidCrystal lcd(3, 4, 5, 6, 7, 8);

void setup() {
  // init serial
  Serial.begin(9600);
  
  // geiger event on pin 2 triggers interrupt
  attachInterrupt(0, click, FALLING);
  
  // setup buttons
  pinMode(MODE_BUTTON_PIN, INPUT);
  pinMode(INT_BUTTON_PIN, INPUT);
  digitalWrite(MODE_BUTTON_PIN, HIGH);
  digitalWrite(INT_BUTTON_PIN, HIGH);
  
  // setup alarm PIN
  pinMode(ALARM_PIN, OUTPUT); 
  
  // init 16x2 display
  lcd.begin(16,2);
  
  // load 6 custom chars
  lcd.createChar(0, bar_0);
  lcd.createChar(1, bar_1);
  lcd.createChar(2, bar_2);
  lcd.createChar(3, bar_3);
  lcd.createChar(4, bar_4);
  lcd.createChar(5, bar_5);  
  
  // read EEPROM settings
  readEEPROM();
  
  // print software version
  clearDisplay();
  lcd.setCursor(3, 0);
  lcd.print("ZX Counter");
  lcd.setCursor(2, 1);
  lcd.print("Version ");
  lcd.print(VERSION);
  delay(1500);

  // print VCC
  clearDisplay();
  long vcc = readVCC();
  lcd.print("VCC: ");
  lcd.print(vcc/1000., 2);
  lcd.print("V");

  // print avail RAM
  lcd.setCursor(0, 1);
  lcd.print("RAM: ");
  lcd.print(getAvailRAM());
  delay(1500);

  // alarm setting
  alarmSetting();
  
  // print scale
  printScale();
  
  // reset counts
  resetCounts();
}

void loop() {
  // collect stats data
  collectData();  

  // check button state each BUTTON_PERIOD milliseconds
  if (millis() >= last_button_time + BUTTON_PERIOD) {
    // update last button check time
    last_button_time = millis();
    
    if (readButton(INT_BUTTON_PIN) == LOW) {
      if (mode < MODE_1S_STATS || mode > MODE_10M_STATS) {
        mode = MODE_1S_STATS;

        // redraw display and scale
        printScale();      
      } else {
        // cycle alt displays
        mode++;
        
        if (mode > MODE_10M_STATS) {
          mode = MODE_1S_STATS;
        }
      }
    }
    
    // if mode button was pressed
    if (readButton(MODE_BUTTON_PIN) == LOW) {
      if (mode < MODE_AUTO_STATS || mode > MODE_DOSE) {
        mode = MODE_AUTO_STATS;
      } else {
        // cycle modes
        mode++;

        if (mode > MODE_DOSE) {
          mode = 0;
        }
      }

      // redraw display and scale
      printScale();      
    }
  }    
    
  // refresh display each REFRESH_PERIOD milliseconds
  if (millis() >= last_refresh + REFRESH_PERIOD) {
    // update last refresh time
    last_refresh = millis();
    
    // check if alarm must be turned on
    checkAlarm();

    switch (mode) {
      case MODE_AUTO_STATS:
        displayAutoStats();
        break;

      case MODE_ALL_STATS:
        // display stats within all time
        displayAllStats();
        break;

      case MODE_MAX:
        // display max CPM and dose
        displayMax();
        break;

      case MODE_DOSE:
        // display total count and accumulated Sv
        displayDose();
        break;

      case MODE_1S_STATS:
        // display 1 sec stats
        displayStats(counts_1s, true, 1, false);
        break;

      case MODE_5S_STATS:
        // display 5 sec stats
        displayStats(get5sCPS(), counts_5s_ready, 5, false);
        break;

      case MODE_10S_STATS:
        // display 10 sec stats
        displayStats(get10sCPS(), counts_10s_ready, 10, false);
        break;

      case MODE_30S_STATS:
        // display 30 sec stats
        displayStats(get30sCPS(), counts_30s_ready, 30, false);
        break;

      case MODE_1M_STATS:
        // display 1 min stats
        displayStats(get1mCPS(), counts_1m_ready, 1, true);
        break;

      case MODE_5M_STATS:
        // display 5 min stats
        displayStats(get5mCPS(), counts_5m_ready, 5, true);
        break;

      case MODE_10M_STATS:
        // display 10 min stats
        displayStats(get10mCPS(), counts_10m_ready, 10, true);
        break;
    }
  }
}

// displays auto stats
void displayAutoStats() {
  byte period;
  boolean ready;
  float cps_5s, cpm_5s, avg_cps, avg_cpm, avg_usv;
    
  // calculate average CPS within last 5 sec
  cps_5s = get5sCPS();
  
  // calculate average CPM within last 5 sec 
  cpm_5s = cps_5s * 60.;
    
  // auto scale
  if (cpm_5s > CPM_LIMIT_1S) {
    period = 1;
    avg_cps = counts_1s; // current CPS
    ready = true;
  } else if (cpm_5s > CPM_LIMIT_5S) {
    period = 5;
    avg_cps = cps_5s; // average CPS within 5 sec
    ready = counts_5s_ready;
  } else if (cpm_5s > CPM_LIMIT_10S) {
    period = 10;
    avg_cps = get10sCPS(); // average CPS within 10 sec
    ready = counts_10s_ready;
  } else if (cpm_5s > CPM_LIMIT_30S) {
    period = 30;
    avg_cps = get30sCPS(); // average CPS within 30 sec
    ready = counts_30s_ready;
  } else {
    period = 60;
    avg_cps = get1mCPS(); // average CPS within 1 min
    ready = counts_1m_ready;
  }
      
  // convert CPS to CPM
  avg_cpm = avg_cps * 60.;
  
  // convert CPM to uSv
  avg_usv = avg_cpm / RATIO;
    
  // update CPM
  lcd.setCursor(4, 0);
  lcd.print("      "); // erase 6 chars after "CPM "
  lcd.setCursor(4, 0);
  printCPM(cpm_5s); // max 6 chars

  lcd.setCursor(11, 0);
  lcd.print("     "); // erase 5 chars
  lcd.setCursor(11, 0);  
  
  if (alarm_enabled) {
    lcd.print("ALARM");
  } else {
    printBar(counts_1s * 60. / RATIO, alarm_usv ? alarm_usv : ALARM_USV, 5); // max 5 chars
  }
  
  // update Sv dimension
  lcd.setCursor(0, 1);
  printSv(avg_usv); // 1 char before "Sv"
  
  // update dose
  lcd.setCursor(6, 1);
  lcd.print("      "); // erase 6 chars after "uSv/h "
  lcd.setCursor(6, 1);
  
  // blink while data is not ready
  if (!ready) {
    delay(BLINK_DELAY);
  }

  printDose(avg_usv, 2); // max 6 chars

  // update period
  lcd.setCursor(13, 1);
  printPeriod(period, false); // 3 chars
}

// displays stats within period
void displayStats(float cps, boolean ready, byte period, boolean minutes) {
  // convert CPS to CPM
  float cpm = cps * 60.;
  
  // convert CPM to uSv;
  float usv = cpm / RATIO;

  // update CPM
  lcd.setCursor(4, 0);
  lcd.print("      "); // erase 6 chars after "CPM "
  lcd.setCursor(4, 0);
  printCPM(cpm); // max 6 chars
  
  // update time
  lcd.setCursor(11, 0);
  printTime(time); // 5 chars
  
  // update Sv dimenstion
  lcd.setCursor(0, 1);
  printSv(usv); // 1 char before "Sv"
  
  // update dose
  lcd.setCursor(6, 1);
  lcd.print("      "); // erase 6 chars after "uSv/h "
  lcd.setCursor(6, 1);
  
  // blink while data is not ready
  if (!ready) {
    delay(BLINK_DELAY);
  }
  
  printDose(usv, 2); // max 6 chars
  
  lcd.setCursor(13, 1);
  printPeriod(period, minutes); // 3 chars
}

// displays stats within all time
void displayAllStats() {
  // calculate CPM
  float cpm = total / (time / 60.);
  
  // convert CPM to uSv;
  float usv = cpm / RATIO;

  // update CPM
  lcd.setCursor(4, 0);
  lcd.print("      "); // erase 6 chars after "CPM "
  lcd.setCursor(4, 0);
  printCPM(cpm); // max 6 chars
  
  // update time
  lcd.setCursor(11, 0);
  printTime(time); // 5 chars
  
  // update Sv dimenstion
  lcd.setCursor(0, 1);
  printSv(usv); // 1 char before "Sv"
  
  // update dose
  lcd.setCursor(6, 1);
  lcd.print("      "); // erase 6 chars after "uSv/h "
  lcd.setCursor(6, 1);
  printDose(usv, 2); // max 6 chars
}

// displays max count and max Sv
void displayMax() {
  // convert CPM to uSv
  float max_usv = max_cpm / RATIO;

  // update CPM
  lcd.setCursor(4, 0);
  lcd.print("      "); // erase 6 chars after "CPM "
  lcd.setCursor(4, 0);
  printCPM(max_cpm); // max 6 chars
  
  // update time
  lcd.setCursor(11, 0);
  printTime(max_time); // 5 chars
  
  // update Sv dimension
  lcd.setCursor(0, 1);
  printSv(max_usv); // 1 char before "Sv"
  
  // update dose
  lcd.setCursor(6, 1);
  lcd.print("      "); // erase 6 chars after "uSv/h "
  lcd.setCursor(6, 1);
  printDose(max_usv, 2); // max 6 chars
}

// displays total count and accumulated dose
void displayDose() {
  float avg_cpm, avg_usv, usv = 0;
  
  if (time) {
    // calculate average CPM
    avg_cpm = total / (time / 60.);
    
    // calculate average uSv/h
    avg_usv = avg_cpm / RATIO;
    
    // calulate accumulated uSv
    usv = avg_usv * (time / 3600.);
  }

  // update total count
  lcd.setCursor(4, 0);
  lcd.print("      "); // erase 6 chars after "CNT "
  lcd.setCursor(4, 0);
  printCPM(total); // max 6 chars
  
  // update time
  lcd.setCursor(11, 0);
  printTime(time); // 5 chars
  
  // update Sv dimension
  lcd.setCursor(0, 1);
  printSv(usv); // 1 char before "Sv"
  
  // update dose
  lcd.setCursor(4, 1);
  lcd.print("       "); // erase 7 chars after "uSv "
  lcd.setCursor(4, 1);
  printDose(usv, 3); // max 7  chars
}

// prints auto scaled time
void printTime(unsigned long sec) {
  char str[6];
  word days = sec / 86400;
  word hours = (sec % 86400) / 3600;
  word minutes = ((sec % 86400) % 3600) / 60;
  word seconds = ((sec % 86400) % 3600) % 60;
  
  if (days) {
    sprintf(str, "%02dd%02d", days, hours);
  } else if (hours) {
    sprintf(str, "%02dh%02d", hours, minutes);
  } else {
    sprintf(str, "%02d:%02d", minutes, seconds);
  }
  
  lcd.print(str);
}

// prints auto scaled CPM
void printCPM(unsigned long cpm) {
  if (cpm >= 1000000) {
    lcd.print(cpm / 1000000., 1);
    lcd.print("M");
  } else {
    lcd.print(cpm);
  }
}

// prints "u", "m" or space (before "Sv") depending on the dose
void printSv(float usv) {
  if (usv >= 500000) {
    lcd.print(" ");
  } else if (usv >= 500) {
    lcd.print("m");
  } else {
    lcd.print("u");
  }
}

// prints auto scaled dose
void printDose(float usv, byte base) {
  if (usv >= 500000) {
    lcd.print(usv / 1000000., base);
  } else if (usv >= 500) {
    lcd.print(usv / 1000., base);
  } else {
    lcd.print(usv, base);
  }
}

// prints auto scaled period
void printPeriod(byte period, boolean minutes) {
  if (period < 10) {
    lcd.print(" ");
  }
  
  if (minutes) {
    lcd.print(period);
    lcd.print("m");
  } else {
    lcd.print(period);
    lcd.print("s");
  }
}

// prints analog bar
void printBar(float value, float max, byte blocks) {
  if (value > max) {
    value = max;
  }
  
  float scaler = max / float(blocks * 5);
  float bar_value = value / scaler;
  byte full_blocks = byte(bar_value) / blocks;
  byte prtl_blocks = byte(bar_value) % blocks;

  for (byte i = 0; i < full_blocks; i++) {
    lcd.write(5);
  }
  
  lcd.write(prtl_blocks);
}

// prints scale
void printScale() {
  clearDisplay();
  switch (mode) {
    case MODE_AUTO_STATS:
    case MODE_1S_STATS:
    case MODE_5S_STATS:
    case MODE_10S_STATS:
    case MODE_30S_STATS:
    case MODE_1M_STATS:
    case MODE_5M_STATS:
    case MODE_10M_STATS:
      lcd.setCursor(0, 0);
      lcd.print("CPM ?");
      lcd.setCursor(0, 1);
      lcd.print("uSv/h ?");
      break;

    case MODE_ALL_STATS:
      lcd.setCursor(0, 0);
      lcd.print("CPM ?");
      lcd.setCursor(0, 1);
      lcd.print("uSv/h ?");
      lcd.setCursor(13, 1);
      lcd.print("ALL");
      break;

    case MODE_MAX:
      lcd.setCursor(0, 0);
      lcd.print("CPM ?");
      lcd.setCursor(0, 1);
      lcd.print("uSv/h ?");
      lcd.setCursor(13, 1);
      lcd.print("MAX");
      break;

    case MODE_DOSE:
      lcd.setCursor(0, 0);
      lcd.print("CNT ?");
      lcd.setCursor(0, 1);
      lcd.print("uSv ?");
      lcd.setCursor(12, 1);
      lcd.print("DOSE");
      break;
  }
}

// reset counts
void resetCounts() {
  counts_1s = 0;

  for (byte i = 0; i < COUNTS_5S_LEN; i++) {
    counts_5s[i] = 0;
  }

  for (byte i = 0; i < COUNTS_10S_LEN; i++) {
    counts_10s[i] = 0;
  }

  for (byte i = 0; i < COUNTS_30S_LEN; i++) {
    counts_30s[i] = 0;
  }

  for (byte i = 0; i < COUNTS_1M_LEN; i++) {
    counts_1m[i] = 0;
  }

  for (byte i = 0; i < COUNTS_5M_LEN; i++) {
    counts_5m[i] = 0;
  }

  for (byte i = 0; i < COUNTS_10M_LEN; i++) {
    counts_10m[i] = 0;
  }

  // reset data ready flags
  counts_5s_ready = counts_10s_ready = counts_30s_ready = counts_1m_ready = counts_10m_ready = false;
  
  // reset timers
  last_button_time = last_refresh = count_1s_time = count_5s_time = count_10s_time = millis();
  
  // reset counts
  time = total = max_cpm = max_time = count_1s = count_5s = count_10s = 0;
}

// check if alarm uSv reached
void checkAlarm() {
  float usv;
  
  if (alarm_usv) {
    usv = get5sCPS() * 60. / RATIO;
    
    if (usv >= alarm_usv) {
      setAlarm(true);
    } else if (alarm_enabled) {
      setAlarm(false);
    }
  } else if (alarm_enabled) {
    setAlarm(false);
  }
}

// turn alarm on or off
void setAlarm(boolean enabled) {
  if (enabled) {
    // turn on alarm (set alarm pin to Vcc) 
    digitalWrite(ALARM_PIN, HIGH);
    alarm_enabled = true;
  } else {
    // turn off alarm (set alarm pin to Gnd)
    digitalWrite(ALARM_PIN, LOW);
    alarm_enabled = false;
  }
}

// alarm setting
void alarmSetting() { 
  clearDisplay();  
  lcd.print("Set Alarm?");
  lcd.setCursor(0, 1);
  if (alarm_usv) {
    lcd.print("Now "); 
    lcd.print(alarm_usv, 2); 
    lcd.print(" uSv"); 
  } else {
    lcd.print("Now Off"); 
  }

  long time = millis();
  float new_alarm_usv = alarm_usv;
  
  while (millis() < time + BUTTON_WAIT) { 
    boolean pushed = false;
    
    if (readButton(INT_BUTTON_PIN) == LOW) { 
      pushed = true;
      if (new_alarm_usv <= 1) {
        new_alarm_usv = new_alarm_usv - 0.1;
      } else if (new_alarm_usv <= 10) {
        new_alarm_usv = new_alarm_usv - 1;
      } else {
        new_alarm_usv = new_alarm_usv - 10;
      }
      if (new_alarm_usv < 0) {
        new_alarm_usv = ALARM_MAX;
      }
    }

    if (readButton(MODE_BUTTON_PIN) == LOW) { 
      pushed = true;
      if (new_alarm_usv < 1) {
        new_alarm_usv = new_alarm_usv + 0.1;
      } else if (new_alarm_usv < 10) {
        new_alarm_usv = new_alarm_usv + 1;
      } else {
        new_alarm_usv = new_alarm_usv + 10;
      }
      if (new_alarm_usv > ALARM_MAX) {
        new_alarm_usv = 0;
      }
    }

    if (pushed) {
      lcd.setCursor(0, 1); 
      lcd.print("                ");
      lcd.setCursor(0, 1);
  
      if (new_alarm_usv) {
        lcd.print(new_alarm_usv, 2); 
        lcd.print(" uSv");
      } else {
        lcd.print("Alarm Off"); 
      }
  
      time = millis();
      delay(100);
    }
  } 
  
  if (new_alarm_usv != alarm_usv) {
    EEPROM.write(ALARM_ADDR, new_alarm_usv * 10.);
    alarm_usv = new_alarm_usv;
    lcd.setCursor(11, 1);
    lcd.print("SAVED");
    delay(1500);
  }
}

// triggers on interrupt event
void click() {
  // increment 1 sec count
  count_1s++;
  
  // increment 5 sec count
  count_5s++;
  
  // increment 10 sec count
  count_10s++;
}

// collects stats data each 1 sec, 5 sec, 10 sec
void collectData() {
  unsigned long cpm;
  
  // collect 1s, 5s, 10s, 30s, 1m every 1s
  if (millis() >= count_1s_time + PERIOD_1S) {
    // increment time
    time++;
    
    // limit time to MAX_TIME seconds
    if (time >= MAX_TIME) {
      time = 0;
      
      // also reset total count for all time stats
      total = 0;
    }
    
    // assign current CPS
    counts_1s = count_1s;

    // reset current CPS count
    count_1s = 0;
    
    // reset last 1 sec time
    count_1s_time = millis();

    // accumulate 5 sec data
    counts_5s[counts_5s_index++] = counts_1s;
    
    // accumulate 10 sec data
    counts_10s[counts_10s_index++] = counts_1s;
    
    // accumulate 30 sec data
    counts_30s[counts_30s_index++] = counts_1s;
    
    // accumulate 1 min data
    counts_1m[counts_1m_index++] = counts_1s;
    
    // increment 5 sec index and check if data is ready
    if (counts_5s_index >= COUNTS_5S_LEN) {
      counts_5s_index = 0;
      counts_5s_ready = true;
    }

    // increment 10 sec index and check if data is ready
    if (counts_10s_index >= COUNTS_10S_LEN) {
      counts_10s_index = 0;
      counts_10s_ready = true;
    }

    // increment 30 sec index and check if data is ready
    if (counts_30s_index >= COUNTS_30S_LEN) {
      counts_30s_index = 0;
      counts_30s_ready = true;
    }

    // increment 1 min sec index and check if data is ready
    if (counts_1m_index >= COUNTS_1M_LEN) {
      counts_1m_index = 0;
      counts_1m_ready = true;
    }
  }
  
  // collect 5m data every 5s
  if (millis() >= count_5s_time + PERIOD_5S) {
    // sum total counts
    total += count_5s;
    
    // accumulate 5 min data
    counts_5m[counts_5m_index++] = count_5s;

    // reset count within 5 sec
    count_5s = 0;
    
    // reset last 5 sec time
    count_5s_time = millis();

    // increment 5 min index and check if data is ready
    if (counts_5m_index >= COUNTS_5M_LEN) {
      counts_5m_index = 0;
      counts_5m_ready = true;
    }

    // calculate max cpm each 5 sec
    cpm = get5sCPS() * 60;
    
    if (cpm > max_cpm) {
      max_cpm = cpm; 
      max_time = time;
    }
  }

  // collect 10m data every 10s
  if (millis() >= count_10s_time + PERIOD_10S) {
    // accumulate 10 min data
    counts_10m[counts_10m_index++] = count_10s;

    // reset count within 10 sec
    count_10s = 0;
    
    // reset 10 sec time
    count_10s_time = millis();

    // increment 10 min index and check if data is ready
    if (counts_10m_index >= COUNTS_10M_LEN) {
      counts_10m_index = 0;
      counts_10m_ready = true;
    }
  }
}

// returns average CPS within 5 sec
float get5sCPS() {
  float sum = 0;
  
  // check if 5 sec stats is ready
  byte max = counts_5s_ready ? COUNTS_5S_LEN : counts_5s_index;
  
  // division by zero check
  if (max) {
    for (byte i = 0; i < max; i++) {
      sum += counts_5s[i];
    }
    
    return sum / max;
  }
  
  return 0.;
}

// returns average CPS within 10 sec
float get10sCPS() {
  float sum = 0;
  
  // check if 10 sec stats is ready
  byte max = counts_10s_ready ? COUNTS_10S_LEN : counts_10s_index;
  
  // division by zero check
  if (max) {
    for (byte i = 0; i < max; i++) {
      sum += counts_10s[i];
    }
    
    return sum / max;
  }
  
  return 0.;
}

// returns average CPS within 30 sec
float get30sCPS() {
  float sum = 0;
  
  // check if 30 sec stats is ready
  byte max = counts_30s_ready ? COUNTS_30S_LEN : counts_30s_index;

  // division by zero check
  if (max) {
    for (byte i = 0; i < max; i++) {
      sum += counts_30s[i];
    }

    return sum / max;
  }

  return 0.;
}

// returns average CPS within 1 min
float get1mCPS() {
  float sum = 0;
  
  // check if 1 min stats is ready
  byte max = counts_1m_ready ? COUNTS_1M_LEN : counts_1m_index;
  
  // division by zero check
  if (max) {
    for (byte i = 0; i < max; i++) {
      sum += counts_1m[i];
    }
    
    return sum / max;
  }
  
  return 0.;
}

// returns average CPS within 5 min
float get5mCPS() {
  float sum = 0;
  
  // check if 5 min stats is ready
  byte max = counts_5m_ready ? COUNTS_5M_LEN : counts_5m_index;
  
  // division by zero check
  if (max) {
    for (byte i = 0; i < max; i++) {
      sum += counts_5m[i];
    }
    
    return sum / max / 5.;
  }
  
  return 0.;
}

// returns average CPS within 10 min
float get10mCPS() {
  float sum = 0;
  
  // check if 10 min stats is ready
  byte max = counts_10m_ready ? COUNTS_10M_LEN : counts_10m_index;
  
  // division by zero check
  if (max) {
    for (byte i = 0; i < max; i++) {
      sum += counts_10m[i];
    }
    
    return sum / max / 10.;
  }

  return 0.;
}

void clearDisplay() {
  // The OLED display does not always reset the cursor after a clear(), so it's done here
  lcd.clear(); // clear the screen
  lcd.setCursor(0,0); // reset the cursor for the poor OLED
  lcd.setCursor(0,0); // do it again for the OLED
}

void readEEPROM() {
  byte alarm_setting = EEPROM.read(ALARM_ADDR);
  if (alarm_setting == 255) alarm_setting = 0; // deal with virgin EEPROM
  alarm_usv = alarm_setting / 10.;
}

long readVCC() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

int getAvailRAM() { 
  int memSize = 2048; // if ATMega328
  byte *buff;
  while ((buff = (byte *) malloc(--memSize)) == NULL);
  free(buff);
  return memSize;
}

// reads LOW ACTIVE push buttom and debounces
byte readButton(byte button_pin) {
  if (digitalRead(button_pin)) {
    // still high, nothing happened, get out
    return HIGH;
  } else {
    // it's LOW - switch pushed
    // wait for debounce period
    delay(DEBOUNCE_PERIOD);
    if (digitalRead(button_pin)) {
      // no longer pressed
      return HIGH;
    } else {
      // 'twas pressed
      return LOW;
    }
  }
}


