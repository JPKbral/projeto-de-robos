// A ideia do nome foi da Manu.

#include <AFMotor.h>
#include <Wire.h>
#include <MPU6050.h>

// Motores
AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);
AF_DCMotor motor4(4);

// IMU
MPU6050 imu;

// Variáveis
float gyroZ;
float angleZ = 0;
float gyroZ_offset = 0;
unsigned long lastTime;

void calibrarGiroscopio() {
  Serial.println("Calibrando giroscópio...");
  long soma = 0;
  int numLeituras = 1000;

  for (int i = 0; i < numLeituras; i++) {
    int16_t gz;
    imu.getRotation(NULL, NULL, &gz);
    soma += gz;
    delay(2); // tempo para estabilidade entre as leituras
  }

  gyroZ_offset = soma / (float)numLeituras;
  Serial.print("Offset do giroscópio Z: ");
  Serial.println(gyroZ_offset);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  imu.initialize();

  if (!imu.testConnection()) {
    Serial.println("Erro ao conectar ao IMU");
    while (1);
  }

  delay(1000); // estabilização inicial do sensor
  calibrarGiroscopio(); // realiza calibração parado

  lastTime = millis();
  int velocidade = 200;

  // Anda para frente
  motor1.setSpeed(velocidade);
  motor2.setSpeed(velocidade);
  motor3.setSpeed(velocidade);
  motor4.setSpeed(velocidade);
  motor1.run(FORWARD);
  motor2.run(FORWARD);
  motor3.run(FORWARD);
  motor4.run(FORWARD);
  delay(10000);

  // Para
  motor1.setSpeed(0);
  motor2.setSpeed(0);
  motor3.setSpeed(0);
  motor4.setSpeed(0);
  delay(500);

  // Gira 90 graus à direita usando giroscópio calibrado
  angleZ = 0;
  lastTime = millis();

  motor1.setSpeed(velocidade);
  motor2.setSpeed(velocidade);
  motor3.setSpeed(velocidade);
  motor4.setSpeed(velocidade);
  motor1.run(FORWARD);
  motor2.run(BACKWARD);
  motor3.run(BACKWARD);
  motor4.run(FORWARD);

  while (abs(angleZ) < 90) {
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;

    imu.getRotation(NULL, NULL, &gyroZ);
    float z = (gyroZ - gyroZ_offset) / 131.0;

    angleZ += z * deltaTime;
    Serial.print("Ângulo Z: ");
    Serial.println(angleZ);
  }

  // Para os motores
  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);
  motor4.run(RELEASE);
}

void loop() {
  // vazio
}
