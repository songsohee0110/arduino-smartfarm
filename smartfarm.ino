#include <SoftwareSerial.h>

SoftwareSerial bluetooth(10, 11);  // RX: D10, TX: D11

const unsigned long SAMPLE_INTERVAL = 100;    // 0.1초마다 측정
const unsigned long REPORT_INTERVAL = 30000;  // 30초마다 정기 출력

// 큰 변화 기준 (0~1023 원시값 기준)
const int SOIL_CHANGE = 100;
const int LIGHT_CHANGE = 100;
const int GAS_CHANGE = 100;

unsigned long lastSampleTime = 0;
unsigned long lastReportTime = 0;

int lastSoil = 0;
int lastLight = 0;
int lastGas = 0;

bool firstReport = true;

// 컴퓨터와 휴대폰에 같은 형식으로 출력
void sendValues(Print &output, int soil, int light,
                int gas, bool changed) {
  output.print(changed ? "[CHANGE] " : "[PERIODIC] ");

  output.print("Soil: ");
  output.print(soil);

  output.print(", Light: ");
  output.print(light);

  output.print(", Gas: ");
  output.println(gas);
}

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);
}

void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime < SAMPLE_INTERVAL) {
    return;
  }
  lastSampleTime = now;

  int soil = analogRead(A0);
  int light = analogRead(A1);
  int gas = analogRead(A2);

  // 마지막 출력값과 비교
  bool changed = !firstReport &&
    (abs(soil - lastSoil) >= SOIL_CHANGE ||
     abs(light - lastLight) >= LIGHT_CHANGE ||
     abs(gas - lastGas) >= GAS_CHANGE);

  bool timeToReport =
    now - lastReportTime >= REPORT_INTERVAL;

  if (firstReport || changed || timeToReport) {
    sendValues(Serial, soil, light, gas, changed);
    sendValues(bluetooth, soil, light, gas, changed);

    lastSoil = soil;
    lastLight = light;
    lastGas = gas;

    // 출력 시점부터 다시 1분을 셈
    lastReportTime = now;
    firstReport = false;
  }
}