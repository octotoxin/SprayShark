#include <Arduino.h>
#include "movement.h"

namespace {
constexpr uint8_t RPWM_PIN = 1;
constexpr uint8_t LPWM_PIN = 2;
constexpr uint8_t R_EN_PIN = 4;
constexpr uint8_t L_EN_PIN = 5;
constexpr uint8_t DRIVE_PWM = 255;

enum class Direction { STOPPED, LEFT, RIGHT };
Direction currentDirection = Direction::STOPPED;
}

void actuatorStop() {
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
  digitalWrite(R_EN_PIN, LOW);
  digitalWrite(L_EN_PIN, LOW);
  currentDirection = Direction::STOPPED;
}

void movementInit() {
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(R_EN_PIN, OUTPUT);
  pinMode(L_EN_PIN, OUTPUT);
  digitalWrite(R_EN_PIN, LOW);
  digitalWrite(L_EN_PIN, LOW);
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
  currentDirection = Direction::STOPPED;
}

void actuatorLeft() {
  if (currentDirection == Direction::LEFT) {
    return;
  }
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
  digitalWrite(R_EN_PIN, HIGH);
  digitalWrite(L_EN_PIN, HIGH);
  analogWrite(RPWM_PIN, DRIVE_PWM);
  currentDirection = Direction::LEFT;
}

void actuatorRight() {
  if (currentDirection == Direction::RIGHT) {
    return;
  }
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
  digitalWrite(R_EN_PIN, HIGH);
  digitalWrite(L_EN_PIN, HIGH);
  analogWrite(LPWM_PIN, DRIVE_PWM);
  currentDirection = Direction::RIGHT;
}