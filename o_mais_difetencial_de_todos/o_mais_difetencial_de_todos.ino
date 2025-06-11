#include <AFMotor.h>
#include <Wire.h>
#include <Servo.h>

// Definições
#define MPU_ADDR 0x68
#define CALIBRATION_SAMPLES 2000
#define PRINT_INTERVAL 100 // ms
#define I2C_RESET_INTERVAL 5000 // ms

// Variáveis do giroscópio
float RateRoll, RatePitch, RateYaw;
float RateCalibrationRoll, RateCalibrationPitch, RateCalibrationYaw;
bool mpuInitialized = false;

// Servo motores
Servo meuServoG; // Garra
Servo meuServoC; // Dispenser

// Motores
AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);
AF_DCMotor motor4(4);

void setup() {
  // Inicialização serial
  Serial.begin(57600);
  
  // Configuração dos servos
  meuServoG.attach(9);
  meuServoC.attach(10);
  
  // LED de status
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  
  // Inicialização I2C e MPU6050
  Wire.setClock(400000);
  Wire.begin();
  delay(100);

    if (!initMPU()) {
    Serial.println("Falha na inicialização do MPU6050");
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  
  // Calibração do giroscópio
  calibrateGyro();
  digitalWrite(13, LOW);
  
  // Sequência de movimentos do robô
  executarSequencia();
}

void loop() {
  // Vazio
}

// Funções do MPU6050 --------------------------------------------------------
bool initMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0x00); // Desliga sleep mode
  if (Wire.endTransmission() != 0) return false;
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A); // CONFIG
  Wire.write(0x05); // Filtro passa-baixa 5Hz
  if (Wire.endTransmission() != 0) return false;
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B); // GYRO_CONFIG
  Wire.write(0x08); // ±500°/s
  if (Wire.endTransmission() != 0) return false;
  
  mpuInitialized = true;
  return true;
}

void calibrateGyro() {
  Serial.println("Calibrando giroscópio...");
  
  RateCalibrationRoll = 0;
  RateCalibrationPitch = 0;
  RateCalibrationYaw = 0;
  
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    if (readGyro()) {
      RateCalibrationRoll += RateRoll;
      RateCalibrationPitch += RatePitch;
      RateCalibrationYaw += RateYaw;
    }
    delay(1);
    
    // Feedback visual
    if (i % 100 == 0) digitalWrite(13, !digitalRead(13));
  }
  
  RateCalibrationRoll /= CALIBRATION_SAMPLES;
  RateCalibrationPitch /= CALIBRATION_SAMPLES;
  RateCalibrationYaw /= CALIBRATION_SAMPLES;
  
  Serial.println("Calibração completa");
  Serial.print("Offsets - Roll: "); Serial.print(RateCalibrationRoll);
  Serial.print(" Pitch: "); Serial.print(RateCalibrationPitch);
  Serial.print(" Yaw: "); Serial.println(RateCalibrationYaw);
}

bool readGyro() {
  // Lê dados do giroscópio
  static bool needsReset = false;
  
  if (needsReset) {
    Wire.begin();
    delay(10);
    needsReset = false;
  }
  
  // Lê dados do giroscópio
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43); // Registro GYRO_XOUT_H
  if (Wire.endTransmission() != 0) {
    needsReset = true;
    return false;
  }
  
  if (Wire.requestFrom(MPU_ADDR, 6) != 6) {
    needsReset = true;
    return false;
  }
  
  // Lê e processa os dados
  int16_t GyroX = Wire.read() << 8 | Wire.read();
  int16_t GyroY = Wire.read() << 8 | Wire.read();
  int16_t GyroZ = Wire.read() << 8 | Wire.read();
  
  // Converte para °/s
  RateRoll = (float)GyroX / 65.5;
  RatePitch = (float)GyroY / 65.5;
  RateYaw = (float)GyroZ / 65.5;
  
  return true;
}

// Funções de Controle do Robô -----------------------------------------------
void subir_garra() {
  Serial.println("Subindo garra...");
  for (int pos = 0; pos <= 180; pos++) {
    meuServoG.write(pos);
    delay(15);
  }
  delay(1000);
}

void abaixar_garra() {
  Serial.println("Descendo garra...");
  for (int pos = 180; pos >= 0; pos--) {
    meuServoG.write(pos);
    delay(15);
  }
  delay(1000);
}

void acionar_dispenser() {
  for (int pos = 0; pos <= 180; pos++) {
    meuServoC.write(pos);
    delay(15);
  }
  delay(1000);
  for (int pos = 180; pos >= 0; pos--) {
    meuServoC.write(pos);
    delay(15);
  }
  delay(1000);
  meuServoC.write(90);
}

void AndaParaFrente(int vel) {
  motor1.setSpeed(vel);
  motor2.setSpeed(vel);
  motor3.setSpeed(vel);
  motor4.setSpeed(vel);
  motor1.run(FORWARD);
  motor2.run(FORWARD);
  motor3.run(FORWARD);
  motor4.run(FORWARD);
}

void pararMotores() {
  motor1.setSpeed(0);
  motor2.setSpeed(0);
  motor3.setSpeed(0);
  motor4.setSpeed(0);
}

void Giro_Esq(int vel, float anguloDesejado) {
  float anguloAtual = 0;
  unsigned long tempoAnterior = millis();
  
  while(abs(anguloAtual) < abs(anguloDesejado)) {

    if (!readGyro()) continue;
    
    float roll = RateRoll - RateCalibrationRoll;
    
    unsigned long tempoAtual = millis();
    float deltaT = (tempoAtual - tempoAnterior) / 1000.0;
    anguloAtual += roll * deltaT;
    tempoAnterior = tempoAtual;
    
    Serial.println("Girando para a esquerda...");

    motor1.setSpeed(vel);
    motor2.setSpeed(vel);
    motor3.setSpeed(vel);
    motor4.setSpeed(vel);
    motor1.run(BACKWARD);
    motor2.run(FORWARD);
    motor3.run(BACKWARD);
    motor4.run(FORWARD);
    
    delay(10);
  }
  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);
  motor4.run(RELEASE);
}

void Giro_Dir(int vel, float anguloDesejado) {
  float anguloAtual = 0;
  unsigned long tempoAnterior = millis();
  
  while(abs(anguloAtual) < abs(anguloDesejado)) {

    if (!readGyro()) continue;
    
    float roll = RateRoll - RateCalibrationRoll;
    
    unsigned long tempoAtual = millis();
    float deltaT = (tempoAtual - tempoAnterior) / 1000.0;
    anguloAtual += roll * deltaT;
    tempoAnterior = tempoAtual;

    Serial.println("Girando para a direita...");
    
    motor1.setSpeed(vel);
    motor2.setSpeed(vel);
    motor3.setSpeed(vel);
    motor4.setSpeed(vel);
    motor1.run(FORWARD);
    motor2.run(BACKWARD);
    motor3.run(FORWARD);
    motor4.run(BACKWARD);
    
    delay(10);
  }
  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);
  motor4.run(RELEASE);

}

void executarSequencia() {
  int velocidade = 200;

  // Sequência 1
  abaixar_garra();
  AndaParaFrente(velocidade);
  acionar_dispenser();
  delay(2000);
  pararMotores();
  delay(1000);
  subir_garra();

  // Giro esquerda 90°
  Giro_Esq(velocidade, 90);
  delay(1000);

  // Anda 36 cm
  AndaParaFrente(velocidade);
  delay(1062);
  pararMotores();
  delay(1000);

  // Giro esquerda -90°
  Giro_Esq(velocidade, -90);
  delay(1000);

  // Sequência 2
  abaixar_garra();
  AndaParaFrente(velocidade);
  acionar_dispenser();
  delay(5310);
  pararMotores();
  delay(2000);
  subir_garra();

  // Giro direita 90°
  Giro_Dir(velocidade, 90);
  delay(1000);

  // Anda 36 cm
  AndaParaFrente(velocidade);
  delay(1062);
  pararMotores();
  delay(2000);

  // Giro direita 90°
  Giro_Dir(velocidade, 90);
  delay(2000);

  pararMotores();
  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);
  motor4.run(RELEASE);
}