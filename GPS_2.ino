#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <EEPROM.h>

#define pulsante1 A1
#define pulsante2 A2

//U8G2_ST7567_JLX12864_F_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/  6, /* data=*/ 7, /* cs=*/ 3 , /* dc=*/ 5, /* reset=*/ 4);
U8G2_ST7567_JLX12864_F_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/5, /* data=*/6, /* cs=*/9, /* dc=*/7, /* reset=*/8);

TinyGPSPlus gps;
SoftwareSerial ss(4, 2);

char Time[] = "00:00";
char TimeS[] = "00:00:00";
char Date[] = "00/00/2000";
byte ultimo_second;
boolean StatoPulsante = false;
int p1, p2, C;
float S, Vm;
short int x = 0;
short int luminosita, contrasto, utc, dst, t;

const unsigned char satellite[8] PROGMEM = {
  B01001100,
  B01000010,
  B00101010,
  B00110000,
  B00011000,
  B00100110,
  B01110000,
  B11111000,
};

void setup() {
  u8g2.begin();
  u8g2.setFontPosTop();
  ss.begin(9600);
  pinMode(pulsante1, INPUT);
  pinMode(pulsante2, INPUT);
  pinMode(A7, INPUT);
  pinMode(3, OUTPUT);
  u8g2.enableUTF8Print();
  luminosita = EEPROM.read(1);
  analogWrite(3, luminosita);
  contrasto = EEPROM.read(0);
  u8g2.setContrast(contrasto);
  utc = EEPROM.read(2) - 12;
  dst = EEPROM.read(3);
}

void (*resetBoard)() = 0;

void loop() {
  while ((ss.available() > 0) && (StatoPulsante == false)) {
    if (gps.encode(ss.read())) {
      tempo(false);
      u8g2.clearBuffer();
      displayInfo();
      u8g2.sendBuffer();
    }
  }

  Impostazioni();

  while (millis() > 5000 && gps.charsProcessed() < 10) {
    for (int i = 5; i > -1; i--) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.setCursor(0, 0);
      u8g2.print("Errore: ");
      u8g2.print(i);
      u8g2.setCursor(0, 9);
      u8g2.print("GPS NEO-6M");
      u8g2.setCursor(0, 18);
      u8g2.print("comunicazione fallita");
      u8g2.sendBuffer();
      delay(1000);
    }
    resetBoard();
  }
}

void displayInfo() {
  u8g2.drawLine(0, 30, 128, 30);
  u8g2.drawLine(64, 30, 64, 64);
  u8g2.drawFrame(0, 0, 128, 64);

  u8g2.setFont(u8g2_font_t0_22b_tf);
  u8g2.setCursor(4, 33);
  u8g2.print(Time);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(3, 49);
  u8g2.print(Date);

  if (digitalRead(A7) == HIGH) {
    u8g2.setCursor(69, 53);
    u8g2.print("In carica");
  } else {
    u8g2.drawCircle(74, 56, 5, U8G2_DRAW_ALL);
    x = gps.course.deg();
    u8g2.setCursor(84, 53);
    switch (x) {
      case 330 ... 359:
      case 0 ... 30:
        u8g2.print("Nord");
        u8g2.drawLine(74, 56, 74, 53);
        break;
      case 31 ... 59:
        u8g2.print("NE");
        break;
      case 60 ... 120:
        u8g2.print("Est");
        u8g2.drawLine(74, 56, 77, 56);
        break;
      case 121 ... 149:
        u8g2.print("SE");
        break;
      case 150 ... 210:
        u8g2.print("Sud");
        u8g2.drawLine(74, 56, 74, 60);
        break;
      case 211 ... 239:
        u8g2.print("SO");
        break;
      case 240 ... 300:
        u8g2.print("Ovest");
        u8g2.drawLine(74, 56, 71, 56);
        break;
      case 301 ... 229:
        u8g2.print("NO");
        break;
      default:
        u8g2.print("N/A");
        u8g2.setCursor(72, 52);
        u8g2.print("?");
    }
    x = 0;
  }

  u8g2.drawXBMP(118, 21, 8, 8, satellite);
  u8g2.setCursor(111, 21);
  u8g2.print(gps.satellites.value());

  u8g2.setCursor(69, 31);
  u8g2.print("Alt: ");
  x = gps.altitude.meters();
  u8g2.print(x);
  u8g2.print("m");

  u8g2.setCursor(69, 42);
  u8g2.print("Vm : ");
  u8g2.print(Vm, 1);

  u8g2.setFont(u8g2_font_logisoso24_tn);
  u8g2.setCursor(2, 3);
  if (gps.speed.isValid()) {
    if (gps.speed.kmph() < 1) {
      u8g2.print("0.0");
    } else {
      u8g2.print(gps.speed.kmph(), 1);
      C++;
      S = S + gps.speed.kmph();
      Vm = S / C;
      if (C > 216000) {  //resetta le variabili dopo circa 1h
        C, Vm = 0;
      }
    }
    u8g2.setFont(u8g2_font_t0_22b_tf);
    u8g2.print(" km/h");
  } else {
    u8g2.setFont(u8g2_font_t0_22b_tf);
    u8g2.print("--- km/h");
  };
}

void tempo(boolean secondi) {
  setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
  short int ADtime = (3600 * utc) + (3600 * dst);
  adjustTime(ADtime);
  if (ultimo_second != gps.time.second()) {
    ultimo_second = gps.time.second();
    if(secondi){
      TimeS[6] = second() / 10 + '0';
      TimeS[7] = second() % 10 + '0';
      TimeS[3] = minute() / 10 + '0';
      TimeS[4] = minute() % 10 + '0';
      TimeS[0] = hour() / 10 + '0';
      TimeS[1] = hour() % 10 + '0';
    } else {
      Time[3] = minute() / 10 + '0';
      Time[4] = minute() % 10 + '0';
      Time[0] = hour() / 10 + '0';
      Time[1] = hour() % 10 + '0';
    }

    Date[8] = (year() / 10) % 10 + '0';
    Date[9] = year() % 10 + '0';
    Date[3] = month() / 10 + '0';
    Date[4] = month() % 10 + '0';
    Date[0] = day() / 10 + '0';
    Date[1] = day() % 10 + '0';


  }
}

void Impostazioni() {
  if (digitalRead(pulsante1) == HIGH) {
    p1++;
  } else {
    p1 = 0;
  }
  if (p1 == 1) {
    StatoPulsante = true;
    x = 1;
    p1 = 2;
  }

  while (StatoPulsante) {
    ss.end();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(28, 0);
    u8g2.setDrawColor(1);
    u8g2.print("IMPOSTAZIONI");
    u8g2.drawBox(0, x * 11, 128, 10);
    //-----------------------------
    u8g2.setCursor(2, 11);
    if (x == 1) {
      u8g2.setDrawColor(0);
      u8g2.write(62);
    } else {
      u8g2.setDrawColor(1);
      u8g2.write(0);
    };
    u8g2.print("Luminosità");
    //-----------------------------
    u8g2.setCursor(2, 22);
    if (x == 2) {
      u8g2.setDrawColor(0);
      u8g2.write(62);
    } else {
      u8g2.setDrawColor(1);
      u8g2.write(0);
    };
    u8g2.print("Contrasto");
    //-----------------------------
    u8g2.setCursor(2, 33);
    if (x == 3) {
      u8g2.setDrawColor(0);
      u8g2.write(62);
    } else {
      u8g2.setDrawColor(1);
      u8g2.write(0);
    };
    u8g2.print("Data e ora");
    //-----------------------------
    u8g2.setCursor(2, 44);
    if (x == 4) {
      u8g2.setDrawColor(0);
      u8g2.write(62);
    } else {
      u8g2.setDrawColor(1);
      u8g2.write(0);
    }
    u8g2.print("Log dati");
    //-----------------------------
    u8g2.setCursor(2, 55);
    if (x == 5) {
      u8g2.setDrawColor(0);
      u8g2.write(62);
    } else {
      u8g2.setDrawColor(1);
      u8g2.write(0);
    }
    u8g2.print("Esci");

    u8g2.sendBuffer();

    if (digitalRead(pulsante2) == HIGH) {
      p2++;
    } else {
      p2 = 0;
    }
    if (p2 == 1) {
      x++;
    };

    switch (x) {
      case 1:
        if (digitalRead(pulsante1) == HIGH) {
          p1++;
        } else {
          p1 = 0;
        }
        if (p1 == 1) { Luminosita(); };
      break;
      case 2:
        if (digitalRead(pulsante1) == HIGH) {
          p1++;
        } else {
          p1 = 0;
        }
        if (p1 == 1) { Contrasto(); };
      break;
      case 3:
        if (digitalRead(pulsante1) == HIGH) {
          p1++;
        } else {
          p1 = 0;
        }
        if (p1 == 1) { Dataeora(); };
      break;
      case 4:
      if (digitalRead(pulsante1) == HIGH) {
          p1++;
        } else {
          p1 = 0;
        }
        if (p1 == 1) { LogDati(); };
      break;
      case 5:
        if (digitalRead(pulsante1) == HIGH) {
          p1++;
        } else {
          p1 = 0;
        }
        if (p1 == 1) {
          StatoPulsante = false;
          ss.begin(9600);
          u8g2.setDrawColor(1);
          u8g2.clearBuffer();
          u8g2.setCursor(0, 0);
          u8g2.print("Salvataggio...");
          u8g2.sendBuffer();
          delay(500);
        };
        break;
      default: x = 1;
    }
  }
}

void Luminosita() {
  x = 0;
  p2 = 0;
  p1 = 2;

  while (p1 != 1) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 0);
    u8g2.print("Regola luminosità");
    u8g2.drawFrame(14, 38, 100, 11);
    u8g2.drawTriangle(12, 37, 12, 49, 6, 43);
    u8g2.drawTriangle(116, 37, 116, 49, 122, 43);
    u8g2.drawBox(16 + x, 40, 8, 7);
    u8g2.sendBuffer();

    if (digitalRead(pulsante2) == HIGH) {
      p2++;
    } else {
      p2 = 0;
    }
    if (p2 == 1) { x = x + 5; };
    if (x > 89) { x = 0; };
    luminosita = (x * 255) / 89;
    analogWrite(3, luminosita);
    if (digitalRead(pulsante1) == HIGH) {
      p1++;
    } else {
      p1 = 0;
    }
  }
  EEPROM.write(1, luminosita);
}

void Contrasto() {
  x = 31;
  p1 = 31;

  while (p1 != 1) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 0);
    u8g2.print("Regola contrasto");
    u8g2.drawFrame(14, 38, 100, 11);
    u8g2.drawTriangle(12, 37, 12, 49, 6, 43);
    u8g2.drawTriangle(116, 37, 116, 49, 122, 43);
    u8g2.drawBox(16 + x, 40, 8, 7);
    u8g2.sendBuffer();

    if (digitalRead(pulsante2) == HIGH) {
      p2++;
    } else {
      p2 = 0;
    }
    if (p2 == 1) { x = x + 5; };
    if (x > 89) { x = 0; };
    contrasto = (x * 255) / 89;
    u8g2.setContrast(contrasto);
    if (digitalRead(pulsante1) == HIGH) {
      p1++;
    } else {
      p1 = 0;
    }
  }
  EEPROM.write(0, contrasto);
}

void Dataeora() {
  x = 1;

  while (x != 3) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 0);
    u8g2.setDrawColor(1);
    u8g2.print("Data e ora");
    u8g2.drawBox(0, x * 12, 128, 10);

    switch (x) {
      case 1:
        if (digitalRead(pulsante2) == HIGH) {
          p2++;
        } else {
          p2 = 0;
        }
        if (p2 == 1) { utc++; };
        if (utc > 12) { utc = -12; };
        u8g2.setCursor(0, 12);
        u8g2.setDrawColor(0);
        u8g2.write(62);
        u8g2.print("UTC: ");
        u8g2.print(utc);
        u8g2.setCursor(0, 24);
        u8g2.setDrawColor(1);
        u8g2.write(0);
        u8g2.print("DST: ");
        u8g2.print(dst);
        break;
      case 2:
        if (digitalRead(pulsante2) == HIGH) {
          p2++;
        } else {
          p2 = 0;
        }
        if (p2 == 1) { dst++; };
        if (dst > 1) { dst = 0; };
        u8g2.setCursor(0, 12);
        u8g2.setDrawColor(1);
        u8g2.write(0);
        u8g2.print("UTC: ");
        u8g2.print(utc);
        u8g2.setCursor(0, 24);
        u8g2.setDrawColor(0);
        u8g2.write(62);
        u8g2.print("DST: ");
        u8g2.print(dst);
        break;
    }
    u8g2.sendBuffer();
    if (digitalRead(pulsante1) == HIGH) {
      p1++;
    } else {
      p1 = 0;
    }
    if (p1 == 1) { x++; }
  }
  EEPROM.write(2, utc + 12);
  EEPROM.write(3, dst);
}

void LogDati(){
  ss.begin(9600);
  p1 = 2;
  while (p1 != 1){
    while ((ss.available() > 0)) {
        if (gps.encode(ss.read())) {
          tempo(true);
          u8g2.clearBuffer();    
          u8g2.setCursor(0, 0);  
          u8g2.print("Lat: ");
          u8g2.print(gps.location.lat(), 6);
          u8g2.setCursor(0, 11);
          u8g2.print("Lng: ");
          u8g2.print(gps.location.lng(), 6);
          u8g2.setCursor(0, 22);
          u8g2.print("Alt: ");
          u8g2.print(gps.altitude.meters());
          u8g2.print("  Sat: ");
          u8g2.print(gps.satellites.value());
          u8g2.setCursor(0, 33);
          u8g2.print("Orario: ");
          u8g2.print(TimeS);
          u8g2.setCursor(0, 44);
          u8g2.print("Sovraccarico GPS: ");
          u8g2.print(ss.overflow() ? "Sì" : "No");
          u8g2.setCursor(0, 55);
          u8g2.print("Dispersione: ");
          u8g2.print(gps.hdop.value());
          u8g2.sendBuffer();
      }
    }
    if (digitalRead(pulsante1) == HIGH) {
      p1++;
    } else {
      p1 = 0;
    }
  }
}

