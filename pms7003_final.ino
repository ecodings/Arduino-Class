
/*
  Name:      PMS_Monitor.ino
  Created:   2019-03-05 오후 3:40:07
  Author:    kwonkyo
  PMS7003 센서를 사용해서 미세입자농도를 측정하고, 측정된 Data를 시리얼모니터로 확인하는 코드입니다.
  아두이노가 시리얼모니터와 연결된 상태에서 전원이 들어가면, 시리얼 모니터에서 사용자 입력을 기다립니다. 사용자가 정수값을 입력하면 센서가 켜지고 센서 부팅에 필요한 시간만큼 기다렸다가 준비가 되면 입력된 횟수 만큼 측정을 연속적으로 하고 대기상태로 들어갑니다. (PMS라이브러리 예제 참조)
*/
#include "DHT.h"
#include <PMS.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <stdio.h>
#include "RTClib.h"

File myFile;
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// 소프트웨어시리얼 포트를 설정하고 PMS7003센서 통신용으로 연결
#define SoftRX 2
#define SoftTX 3
#define DHTPIN A0
#define RED A1
#define Green A2
#define Blue A3

SoftwareSerial mySerial(SoftRX, SoftTX);
PMS pms(mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2); // i2c address 0x27 or 0x3F
#define DHTTYPE DHT22   // DHT 22  (AM2302) - 이거 선택

int check1, check2;
DHT dht(DHTPIN, DHTTYPE);

// 디버깅용 시리얼은 하드웨어시리얼 포트를 사용합니다.
#define DEBUG_OUT Serial

// 측정 간격 설정
// :::|------|------|------|::::::::::|------|------|------|:::::::::::
//    ↑     ↑     ↑           ↑              ↑
//   요청   측정   측정   다음요청시까지_대기   측정간격(차수내)
static const uint32_t PMS_READ_DELAY = 30000;       // 센서 부팅후 대기시간 (30 sec)
static const uint32_t PMS_READ_GAP = 3000;          // 측정간격 (1 sec)

// 전역변수 선언
uint8_t cycles = 0;     // 반복측정 횟수
bool PMS_POWER = false; // PMS7003센서의 동작상태
int prevVal = LOW;    //CO2 농도 측정 모드 체크
long pm1,pm2,pm10, cnt;  //CO2 농도 측정값 보정 위한 수식

void setup() {
  DEBUG_OUT.begin(9600);        // 디버깅용
  mySerial.begin(9600); // 소프트 시리얼용

  rtc.begin();
  DateTime now = rtc.now();
  check1 = now.minute();
  check1 = int(check1 / 10) * 10;
  check2 = check1;
  // 패시브모드로 변경 후 센서 대기상태로 설정
  pms.passiveMode();
  pms.sleep();
  lcd.init();
  lcd.backlight();
  Serial.print("Initializing SD card...");
  if (!SD.begin(6)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  //DEBUG_OUT.println("반복측정 횟수를 입력하세요.");
  myFile = SD.open("test.txt", FILE_WRITE);
  if (!myFile) {
    Serial.println("error opening test.txt");
  }
  Serial.println("Check");
  dht.begin();

  pm1=0;
  pm2=0;
  pm10=0;
  cnt=0;
}

// the loop function runs over and over again until power down or reset
void loop() {
  // 센서동작여부를 판단에 사용할 타이머 변수
  static uint32_t timerLast = 0;      // 센터부팅시작시간 (값이 유지되도록 정적변수 사용
  static uint32_t timerBefore = 0;    // 직전측정시간
  //lcd.clear(); // LCD 지우기
  DateTime now = rtc.now();


  // 시리얼모니터로 값을 입력받아 입력받은 값을 cycles 변수에 할당
  //  cycles = 60;  //한번에 몇번을 측정할지 입력받기
  //int time_check;
  //time_check=now.minute()-int(now.minute()/10)*10;
  //  DEBUG_OUT.print(cycles);

  // 측정요청이 있으면 센서를 깨우고 깨운 시간을 기억
  if (!PMS_POWER ) {
    lcd.setCursor(0, 0); // (0, 0)에 커서 설정
    lcd.print("boot");
    DEBUG_OUT.println("Waking up.");
    pms.wakeUp();
    timerLast = millis();
    PMS_POWER = true;
  }

  // 입력받은 cycles 수만큼 측정
  //   센서가 깨어나고나서 최소대기상태를 지났을 때만 측정간격을 두고 측정 실시
  while (1) {
    uint32_t timerNow = millis();
    if (timerNow - timerLast >= PMS_READ_DELAY) {
      if (timerNow - timerBefore >= PMS_READ_GAP) {
        timerBefore = timerNow;
        //lcd.clear();
        readData();
      }
    }
  }

  // 측정이 끝나면 cycles 변수는 0이되었기 때문에 센서를 대기모드로 바꾸고 다음 측정요청을 대기
  if (PMS_POWER && cycles == 0) {
    DEBUG_OUT.println("Going to sleep.");
    DEBUG_OUT.println("반복측정 횟수를 입력하세요.");
    pms.sleep();
    PMS_POWER = false;
  }
}

// PMS7003센서에 측정요청을 하고 데이터를 읽어와서 시리얼 화면에 표시하는 함수
void readData()
{
  lcd.setCursor(0, 0); // (0, 0)에 커서 설정
  lcd.print("PM:");
  DateTime now = rtc.now();
  PMS::DATA data;
  check1 = now.minute();
  check1 = int(check1 / 10) * 10;
  int h = dht.readHumidity();
  int t = dht.readTemperature();

  // Clear buffer (removes potentially old data) before read. Some data could have been also sent before switching to passive mode.
  while (Serial.available()) {
    Serial.read();
  }

  DEBUG_OUT.println("Send read request...");
  pms.requestRead();
  myFile = SD.open("dust.txt", FILE_WRITE);

  DEBUG_OUT.println("Reading data...");
  if (pms.readUntil(data))
  {
    DEBUG_OUT.println();

    if (isnan(t) || isnan(h)) {
      Serial.println("Failed to read from DHT");
    } else {
      Serial.print("/");
      Serial.print(h);
      Serial.print("/");
      Serial.print(t);
      Serial.println("/");
    }

    Serial.print(now.year(), DEC);
    Serial.print("/");
    Serial.print(now.month(), DEC);
    Serial.print("/");
    Serial.print(now.day(), DEC);
    Serial.print("/");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.println("  ");

    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);

    lcd.print(data.PM_AE_UG_10_0);
    lcd.print("(");

    DEBUG_OUT.print("PM 2.5 (ug/m3): ");
    DEBUG_OUT.println(data.PM_AE_UG_2_5);
    lcd.print(data.PM_AE_UG_2_5);

    DEBUG_OUT.print("PM 10.0 (ug/m3): ");
    DEBUG_OUT.println(data.PM_AE_UG_10_0);
    DEBUG_OUT.println(int(pm1/cnt));
    DEBUG_OUT.println(int(pm2/cnt));
    DEBUG_OUT.println(int(pm10/cnt));

    pm1+=data.PM_AE_UG_1_0;
    pm2+=data.PM_AE_UG_2_5;
    pm10+=data.PM_AE_UG_10_0;
    cnt++;
    
    lcd.print(") ");
    

    if (data.PM_AE_UG_10_0 <= 15) {
      analogWrite(RED, 255);
      analogWrite(Green, 255);
      analogWrite(Blue, 0);
      lcd.print("Clean!  ");
    } else if (data.PM_AE_UG_10_0 <= 30) {
      analogWrite(RED, 255);
      analogWrite(Green, 100);
      analogWrite(Blue, 100);
      lcd.print("Good   ");
    } else if (data.PM_AE_UG_10_0 <= 50) {
      analogWrite(RED, 255);
      analogWrite(Green, 0);
      analogWrite(Blue, 255);
      lcd.print("normal  ");      
    } else if (data.PM_AE_UG_10_0 <= 75) {
      analogWrite(RED, 0);
      analogWrite(Green, 0);
      analogWrite(Blue, 255);
      lcd.print("Careful  ");      
    } else if (data.PM_AE_UG_10_0 <= 100) {
      analogWrite(RED, 0);
      analogWrite(Green, 128);
      analogWrite(Blue, 255);
      lcd.print("Bad!!  ");      
    } else {
      analogWrite(RED, 0);
      analogWrite(Green, 255);
      analogWrite(Blue, 255);
      lcd.print("xxxx!!   ");
    }
    lcd.setCursor(0, 1); // (0, 0)에 커서 설정
    lcd.print(t);
    lcd.print((char)223);
    lcd.print("C/");
    lcd.print(h);
    lcd.print("%  ");
    lcd.print(now.hour(), DEC);
    lcd.print(":");
    lcd.print(now.minute(), DEC);
    lcd.print("  ");
    //lcd.print(ppm);
    DEBUG_OUT.println();

    if (check1 != check2) {   //십분 단위가 바뀌었을 때만 SD 카드에 저장.
      pm1=int(pm1/cnt);
      pm2=int(pm2/cnt);
      pm10=int(pm10/cnt);
      myFile.print(now.year(), DEC);
      myFile.print("/");
      myFile.print(now.month(), DEC);
      myFile.print("/");
      myFile.print(now.day(), DEC);
      myFile.print("/");
      myFile.print(now.hour(), DEC);
      myFile.print('/');
      myFile.print(now.minute(), DEC);
      myFile.print("/");
      myFile.print(pm1);
      myFile.print("/");
      myFile.print(pm2);
      myFile.print("/");
      myFile.print(pm10);
      myFile.print("/");
      myFile.print(t);
      myFile.print("/");
      myFile.println(h);
      check2 = check1;
      cnt=0;
      pm1=0;
      pm2=0;
      pm10=0;
    }
  }
  else
  {
    DEBUG_OUT.println("No data.");
  }
  myFile.close();
}
