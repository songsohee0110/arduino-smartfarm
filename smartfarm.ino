#include <SoftwareSerial.h>

SoftwareSerial bluetooth(10, 11);  // RX, TX

const bool SOIL_ENABLED = true;  // 토양 센서 미연결 시 false

const unsigned long SAMPLE_INTERVAL = 100;
const unsigned long REPORT_INTERVAL = 30000;
const unsigned long CONFIRM_TIME = 500;    // 변화가 0.5초 유지
const unsigned long MIN_OUTPUT_GAP = 3000; // 출력 최소 3초 간격

const float ADC_VOLTAGE = 5.0;  // 기준 전압 가정
const float ALPHA = 0.2;       // 평균 처리: 작을수록 부드럽게 변함

// 큰 변화 기준: 전압 차이
const float SOIL_CHANGE = 1.0;
const float LIGHT_CHANGE = 0.5;
const float GAS_CHANGE = 1.0;

struct Sensor {
  byte pin;
  bool enabled;
  float threshold;
  float filtered;
  float reported;
  int direction;
  unsigned long changeStarted;
};

Sensor soil  = {A0, SOIL_ENABLED, SOIL_CHANGE, 0, 0, 0, 0};
Sensor light = {A1, true, LIGHT_CHANGE, 0, 0, 0, 0};
Sensor gas   = {A2, true, GAS_CHANGE, 0, 0, 0, 0};

unsigned long lastSampleTime = 0;
unsigned long lastReportTime = 0;
bool started = false;

void readSensor(Sensor &sensor, bool first) {
  if (!sensor.enabled) return;

  float voltage = analogRead(sensor.pin) * ADC_VOLTAGE / 1023.0;

  if (first) {
    sensor.filtered = voltage;
  } else {
    sensor.filtered += ALPHA * (voltage - sensor.filtered);
  }
}

// 기준을 넘는 변화가 같은 방향으로 유지되는지 확인
bool changeConfirmed(Sensor &sensor, unsigned long now) {
  if (!sensor.enabled) return false;

  float difference = sensor.filtered - sensor.reported;
  int direction = 0;

  if (difference >= sensor.threshold) {
    direction = 1;
  } else if (difference <= -sensor.threshold) {
    direction = -1;
  }

  if (direction == 0) {
    sensor.direction = 0;
    return false;
  }

  if (direction != sensor.direction) {
    sensor.direction = direction;
    sensor.changeStarted = now;
    return false;
  }

  return now - sensor.changeStarted >= CONFIRM_TIME;
}

void printValue(Print &output, Sensor &sensor) {
  if (sensor.enabled) {
    output.print(sensor.filtered, 3);
  } else {
    output.print("NA");
  }
}

void sendValues(Print &output, bool changed) {
  output.print(changed ? "[CHANGE] " : "[PERIODIC] ");

  output.print("Soil: ");
  printValue(output, soil);

  output.print(", Light: ");
  printValue(output, light);

  output.print(", Gas: ");
  printValue(output, gas);
  output.println();
}

void saveReference(Sensor &sensor) {
  sensor.reported = sensor.filtered;
  sensor.direction = 0;
}

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);
}

void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime < SAMPLE_INTERVAL) return;
  lastSampleTime = now;

  bool first = !started;

  readSensor(soil, first);
  readSensor(light, first);
  readSensor(gas, first);

  bool changed = false;

  if (!first) {
    // 각 센서의 확인 시간을 모두 갱신
    bool soilChanged = changeConfirmed(soil, now);
    bool lightChanged = changeConfirmed(light, now);
    bool gasChanged = changeConfirmed(gas, now);

    changed = soilChanged || lightChanged || gasChanged;
  }

  bool periodic = first ||
    now - lastReportTime >= REPORT_INTERVAL;

  bool canOutput = first ||
    now - lastReportTime >= MIN_OUTPUT_GAP;

  if (canOutput && (periodic || changed)) {
    sendValues(Serial, changed);
    sendValues(bluetooth, changed);

    saveReference(soil);
    saveReference(light);
    saveReference(gas);

    lastReportTime = now;
    started = true;
  }
}
