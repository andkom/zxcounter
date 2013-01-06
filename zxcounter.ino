#include <stdio.h>
#include <LiquidCrystal.h>

#define VERSION           1.1
#define RATIO             175.43

#define PERIOD_1S         1000
#define PERIOD_5S         5000
#define PERIOD_10S        10000
#define REFRESH_PERIOD    1000
#define BUTTON_PERIOD     500
#define DEBOUNCE_PERIOD   50

#define COUNTS_5S_LEN     5
#define COUNTS_10S_LEN    10
#define COUNTS_30S_LEN    30
#define COUNTS_1M_LEN     60
#define COUNTS_5M_LEN     60
#define COUNTS_10M_LEN    60

#define CPM_LIMIT_1S      7500
#define CPM_LIMIT_5S      1500
#define CPM_LIMIT_10S     300         
#define CPM_LIMIT_30S     60

#define BAR_BLOCKS        5
#define BAR_SCALE         5.

#define BUTTON_PIN        10

#define MODE_AUTO_STATS   0
#define MODE_1S_STATS     1
#define MODE_5S_STATS     2
#define MODE_10S_STATS    3
#define MODE_30S_STATS    4
#define MODE_1M_STATS     5
#define MODE_5M_STATS     6
#define MODE_10M_STATS    7
#define MODE_MAX          8
#define MODE_DOSE         9

volatile unsigned long count_1s, count_5s, count_10s;

int mode = MODE_AUTO_STATS;
unsigned long total, counts_1s, counts_5s[COUNTS_5S_LEN], counts_10s[COUNTS_10S_LEN], counts_30s[COUNTS_30S_LEN], counts_1m[COUNTS_1M_LEN], counts_5m[COUNTS_5M_LEN], counts_10m[COUNTS_10M_LEN];
unsigned long time, max_cpm, max_time, count_1s_time, count_5s_time, count_10s_time, last_refresh, last_button_time;
boolean counts_5s_ready, counts_10s_ready, counts_30s_ready, counts_1m_ready, counts_5m_ready, counts_10m_ready;
byte counts_5s_index, counts_10s_index, counts_30s_index, counts_1m_index, counts_5m_index, counts_10m_index;

byte bar_0[8] = {0x00, 0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00}; //blank
byte bar_1[8] = {0x10, 0x10, 0x18, 0x18, 0x18, 0x10, 0x10, 0x00}; //1 bar
byte bar_2[8] = {0x18, 0x18, 0x1c, 0x1c, 0x1c, 0x18, 0x18, 0x00}; //2 bars
byte bar_3[8] = {0x1C, 0x1C, 0x1e, 0x1e, 0x1e, 0x1C, 0x1C, 0x00}; //3 bars
byte bar_4[8] = {0x1E, 0x1E, 0x1f, 0x1f, 0x1f, 0x1E, 0x1E, 0x00}; //4 bars
byte bar_5[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00}; //5 bars

LiquidCrystal lcd(3, 4, 5, 6, 7, 8);

void setup() {
  Serial.begin(9600);
  attachInterrupt(0, click, FALLING);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH);
  lcd.begin(16,2);
  lcd.createChar(0, bar_0);
  lcd.createChar(1, bar_1);
  lcd.createChar(2, bar_2);
  lcd.createChar(3, bar_3);
  lcd.createChar(4, bar_4);
  lcd.createChar(5, bar_5);  
  reset();  
}

void loop() {
  collectData();  

  if (millis() >= last_button_time + BUTTON_PERIOD) {
    last_button_time = millis();
    if (readButton(BUTTON_PIN) == LOW) {
      mode++;

      if (mode > MODE_DOSE) {
        mode = 0;
      }

      if (mode == MODE_AUTO_STATS || mode == MODE_MAX || mode == MODE_DOSE) {
        lcd.clear();
        printScale();      
      }
    }
  }    
    
  if (millis() >= last_refresh + REFRESH_PERIOD) {
    last_refresh = millis();

    switch (mode) {
      case MODE_AUTO_STATS:
        displayAutoStats();
        break;
        
      case MODE_1S_STATS:
        displayStats(counts_1s, true, 1, false);
        break;

      case MODE_5S_STATS:
        displayStats(get5sCPS(), counts_5s_ready, 5, false);
        break;

      case MODE_10S_STATS:
        displayStats(get10sCPS(), counts_10s_ready, 10, false);
        break;

      case MODE_30S_STATS:
        displayStats(get30sCPS(), counts_30s_ready, 30, false);
        break;

      case MODE_1M_STATS:
        displayStats(get1mCPS(), counts_1m_ready, 1, true);
        break;

      case MODE_5M_STATS:
        displayStats(get5mCPS(), counts_5m_ready, 5, true);
        break;

      case MODE_10M_STATS:
        displayStats(get10mCPS(), counts_10m_ready, 10, true);
        break;

      case MODE_MAX:
        displayMax();
        break;

      case MODE_DOSE:
        displayDose();
        break;
    }
  }
}

void displayAutoStats() {
  byte period;
  float cps_5s = get5sCPS(), cpm_5s = cps_5s * 60., avg_cps, avg_cpm, avg_usv;
    
  if (cpm_5s > CPM_LIMIT_1S) {
    period = 1;
    avg_cps = counts_1s;
  } else if (cpm_5s > CPM_LIMIT_5S) {
    period = 5;
    avg_cps = cps_5s;
  } else if (cpm_5s > CPM_LIMIT_10S) {
    period = 10;
    avg_cps = get10sCPS();
  } else if (cpm_5s > CPM_LIMIT_30S) {
    period = 30;
    avg_cps = get30sCPS();
  } else {
    period = 60;
    avg_cps = get1mCPS();
  }
      
  avg_cpm = avg_cps * 60.;    
  avg_usv = avg_cpm / RATIO;
    
  lcd.setCursor(4, 0);
  lcd.print("      ");
  lcd.setCursor(4, 0);
  printCPM(cpm_5s);

  lcd.setCursor(11, 0);
  lcd.print("     ");
  lcd.setCursor(11, 0);  
  displayBar(counts_1s * 60. / RATIO);
  
  lcd.setCursor(0, 1);
  printSv(avg_usv);
  
  lcd.setCursor(6, 1);
  lcd.print("      ");
  lcd.setCursor(6, 1);
  printDose(avg_usv, 2);

  lcd.setCursor(13, 1);
  printPeriod(period, false);
}

void displayStats(float cps, boolean ready, int period, boolean minutes) {
  float cpm = cps * 60., usv = cpm / RATIO;

  lcd.setCursor(4, 0);
  lcd.print("      ");
  lcd.setCursor(4, 0);
  printCPM(cpm);
  
  lcd.setCursor(11, 0);
  printTime(time);
  
  lcd.setCursor(0, 1);
  printSv(usv);
  
  lcd.setCursor(6, 1);
  lcd.print("      ");
  lcd.setCursor(6, 1);
  printDose(usv, 2);
  
  if (!ready) {
    lcd.print("?");
  }

  lcd.setCursor(13, 1);
  printPeriod(period, minutes);
}

void displayMax() {
  float max_usv = max_cpm / RATIO;

  lcd.setCursor(4, 0);
  lcd.print("      ");
  lcd.setCursor(4, 0);
  printCPM(max_cpm);
  
  lcd.setCursor(11, 0);
  printTime(max_time);
  
  lcd.setCursor(0, 1);
  printSv(max_usv);
  
  lcd.setCursor(6, 1);
  lcd.print("      ");
  lcd.setCursor(6, 1);
  printDose(max_usv, 2);
}

void displayDose() {
  float usv = 0;
  
  if (time) {
    usv = ((total / (time / 60.)) / RATIO) * (time / 3600.);
  }

  lcd.setCursor(4, 0);
  lcd.print("      ");
  lcd.setCursor(4, 0);
  printCPM(total);
  
  lcd.setCursor(11, 0);
  printTime(time);
  
  lcd.setCursor(0, 1);
  printSv(usv);
  
  lcd.setCursor(4, 1);
  lcd.print("        ");
  lcd.setCursor(4, 1);
  printDose(usv, 3);
}

void printTime(unsigned long t) {
  char str[6];
  int min = t / 60, sec = t % 60;
 
  sprintf(str, "%02d:%02d", min, sec);
  
  lcd.print(str);
}

void printCPM(unsigned long cpm) {
  if (cpm >= 1000000) {
    lcd.print(cpm / 1000000., 1);
    lcd.print("M");
  } else {
    lcd.print(cpm);
  }
}

void printSv(float usv) {
  if (usv >= 500000) {
    lcd.print(" ");
  } else if (usv >= 500) {
    lcd.print("m");
  } else {
    lcd.print("u");
  }
}

void printDose(float usv, int base) {
  if (usv >= 500000) {
    lcd.print(usv / 1000000., base);
  } else if (usv >= 500) {
    lcd.print(usv / 1000., base);
  } else {
    lcd.print(usv, base);
  }
}
void printPeriod(int period, boolean minutes) {
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

void displayBar(float usv) {
  if (usv > BAR_SCALE) {
    usv = BAR_SCALE;
  }
  
  float scaler = BAR_SCALE / float(BAR_BLOCKS * 5);
  float bar_usv = usv / scaler;
  unsigned int full_blocks = int(bar_usv) / BAR_BLOCKS;
  unsigned int prtl_blocks = int(bar_usv) % BAR_BLOCKS;

  for (int i = 0; i < full_blocks; i++) {
    lcd.write(5);
  }
  
  lcd.write(prtl_blocks);
}

byte readButton(int button_pin) {
  if (digitalRead(button_pin)) {
    return HIGH;
  } else {
    delay(DEBOUNCE_PERIOD);
    if (digitalRead(button_pin)) {
      return HIGH;
    } else {
      return LOW;
    }
  }
}

void printScale() {
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

void reset() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("ZX Counter");
  lcd.setCursor(2, 1);
  lcd.print("Version ");
  lcd.print(VERSION);
  delay(1500);
  lcd.clear();
  printScale();
  
  counts_1s = 0;

  for (int i = 0; i < COUNTS_5S_LEN; i++) {
    counts_5s[i] = 0;
  }

  for (int i = 0; i < COUNTS_10S_LEN; i++) {
    counts_10s[i] = 0;
  }

  for (int i = 0; i < COUNTS_30S_LEN; i++) {
    counts_30s[i] = 0;
  }

  for (int i = 0; i < COUNTS_1M_LEN; i++) {
    counts_1m[i] = 0;
  }

  for (int i = 0; i < COUNTS_5M_LEN; i++) {
    counts_5m[i] = 0;
  }

  for (int i = 0; i < COUNTS_10M_LEN; i++) {
    counts_10m[i] = 0;
  }

  counts_5s_ready = counts_10s_ready = counts_30s_ready = counts_1m_ready = counts_10m_ready = false;
  last_button_time = last_refresh = count_1s_time = count_5s_time = count_10s_time = millis();
  time = total = max_cpm = max_time = count_1s = count_5s = count_10s = 0;
}

void click() {
  count_1s++;
  count_5s++;
  count_10s++;
}

void collectData() {
  unsigned long cpm;
  
  // collect 1s, 5s, 10s, 30s, 1m every 1s
  if (millis() >= count_1s_time + PERIOD_1S) {
    // increment time
    time++;
    counts_1s = count_1s;

    count_1s = 0;
    count_1s_time = millis();

    counts_5s[counts_5s_index++] = counts_1s;
    counts_10s[counts_10s_index++] = counts_1s;
    counts_30s[counts_30s_index++] = counts_1s;
    counts_1m[counts_1m_index++] = counts_1s;
    
    if (counts_5s_index >= COUNTS_5S_LEN) {
      counts_5s_index = 0;
      counts_5s_ready = true;
    }

    if (counts_10s_index >= COUNTS_10S_LEN) {
      counts_10s_index = 0;
      counts_10s_ready = true;
    }

    if (counts_30s_index >= COUNTS_30S_LEN) {
      counts_30s_index = 0;
      counts_30s_ready = true;
    }

    if (counts_1m_index >= COUNTS_1M_LEN) {
      counts_1m_index = 0;
      counts_1m_ready = true;
    }
  }
  
  // collect 5m data every 5s
  if (millis() >= count_5s_time + PERIOD_5S) {
    total += count_5s;
    counts_5m[counts_5m_index++] = count_5s;

    count_5s = 0;
    count_5s_time = millis();

    if (counts_5m_index >= COUNTS_5M_LEN) {
      counts_5m_index = 0;
      counts_5m_ready = true;
    }

    // collect max cpm
    cpm = get5sCPS() * 60;
    
    if (cpm > max_cpm) {
      max_cpm = cpm; 
      max_time = time;
    }
  }

  // collect 10m data every 10s
  if (millis() >= count_10s_time + PERIOD_10S) {
    counts_10m[counts_10m_index++] = count_10s;

    count_10s = 0;
    count_10s_time = millis();

    if (counts_10m_index >= COUNTS_10M_LEN) {
      counts_10m_index = 0;
      counts_10m_ready = true;
    }
  }
}

float get5sCPS() {
  float sum = 0;
  int max = counts_5s_ready ? COUNTS_5S_LEN : counts_5s_index;
  if (max) {
    for (int i = 0; i < max; i++) {
      sum += counts_5s[i];
    }
    return sum / max;
  }
  return 0.;
}

float get10sCPS() {
  float sum = 0;
  int max = counts_10s_ready ? COUNTS_10S_LEN : counts_10s_index;
  if (max) {
    for (int i = 0; i < max; i++) {
      sum += counts_10s[i];
    }
    return sum / max;
  }
  return 0.;
}

float get30sCPS() {
  float sum = 0;
  int max = counts_30s_ready ? COUNTS_30S_LEN : counts_30s_index;
  if (max) {
    for (int i = 0; i < max; i++) {
      sum += counts_30s[i];
    }
    return sum / max;
  }
  return 0.;
}

float get1mCPS() {
  float sum = 0;
  int max = counts_1m_ready ? COUNTS_1M_LEN : counts_1m_index;
  if (max) {
    for (int i = 0; i < max; i++) {
      sum += counts_1m[i];
    }
    return sum / max;
  }
  return 0.;
}

float get5mCPS() {
  float sum = 0;
  int max = counts_5m_ready ? COUNTS_5M_LEN : counts_5m_index;
  if (max) {
    for (int i = 0; i < max; i++) {
      sum += counts_5m[i];
    }
    return sum / max / 5.;
  }
  return 0.;
}

float get10mCPS() {
  float sum = 0;
  int max = counts_10m_ready ? COUNTS_10M_LEN : counts_10m_index;
  if (max) {
    for (int i = 0; i < max; i++) {
      Serial.print(counts_10m[i]);
      sum += counts_10m[i];
    }
    return sum / max / 10.;
  }
  return 0.;
}

