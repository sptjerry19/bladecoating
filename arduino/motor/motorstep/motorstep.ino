#include <AccelStepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Chân joystick
#define JOYSTICK_X A0
#define JOYSTICK_Y A1
#define BUTTON_PIN 2

// Chân điều khiển TB6600
#define STEP_PIN 8
#define DIR_PIN 9
#define ENABLE_PIN 10

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Khởi tạo động cơ
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Joystick và nút bấm
int joystickX = 0, joystickY = 0;
int buttonState = 0, prevButtonState = 0;

// Menu
int cursorPosition = 0;
String mainMenu[] = {"Chinh Thong So", "Bat Dau Dong Co"};
int menuCount = sizeof(mainMenu) / sizeof(mainMenu[0]);

// Thông số động cơ
int motorSpeed = 1000;     // Tốc độ (step/giây)
int motorAcceleration = 500; // Gia tốc (step/giây^2)
int motorDirection = 1;    // Hướng động cơ (1: thuận, -1: nghịch)

void setup() {
  // LCD
  lcd.begin(16, 2);
  lcd.setBacklight(true);
  lcd.clear();
  displayMainMenu();

  // Joystick và nút bấm
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Động cơ
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW); // Kích hoạt động cơ
  stepper.setMaxSpeed(motorSpeed);
  stepper.setAcceleration(motorAcceleration);

  lcd.print("Khoi Dong OK");
  delay(1000);
  lcd.clear();
}

void loop() {
  joystickX = analogRead(JOYSTICK_X);
  joystickY = analogRead(JOYSTICK_Y);
  buttonState = digitalRead(BUTTON_PIN);

  // Điều hướng menu
  handleJoystick();
  if (buttonState == LOW && prevButtonState == HIGH) {
    executeMenuOption();
  }
  prevButtonState = buttonState;
}

// Điều hướng menu bằng joystick
void handleJoystick() {
  if (joystickY < 400) { // Lên
    cursorPosition--;
    if (cursorPosition < 0) cursorPosition = menuCount - 1;
    displayMainMenu();
    delay(200);
  } 
  else if (joystickY > 600) { // Xuống
    cursorPosition++;
    if (cursorPosition >= menuCount) cursorPosition = 0;
    displayMainMenu();
    delay(200);
  }
}

// Hiển thị menu chính
void displayMainMenu() {
  lcd.clear();
  for (int i = 0; i < menuCount; i++) {
    lcd.setCursor(0, i);
    if (i == cursorPosition) {
      lcd.print("> ");
    } else {
      lcd.print("  ");
    }
    lcd.print(mainMenu[i]);
  }
}

// Thực thi tùy chọn menu
void executeMenuOption() {
  if (cursorPosition == 0) {
    adjustMotorSettings();
  } else if (cursorPosition == 1) {
    runMotor();
  }
}

// Chỉnh thông số động cơ
void adjustMotorSettings() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Toc Do: ");
  lcd.print(motorSpeed);
  lcd.setCursor(0, 1);
  lcd.print("Gia Toc: ");
  lcd.print(motorAcceleration);

  while (true) {
    joystickX = analogRead(JOYSTICK_X);
    joystickY = analogRead(JOYSTICK_Y);
    buttonState = digitalRead(BUTTON_PIN);

    // Điều chỉnh tốc độ
    if (joystickY < 400) {
      motorSpeed -= 100;
      if (motorSpeed < 100) motorSpeed = 100;
      updateMotorSettings();
    }
    if (joystickY > 600) {
      motorSpeed += 100;
      if (motorSpeed > 2000) motorSpeed = 2000;
      updateMotorSettings();
    }

    // Điều chỉnh gia tốc
    if (joystickX > 600) {
      motorAcceleration += 50;
      if (motorAcceleration > 1000) motorAcceleration = 1000;
      updateMotorSettings();
    }
    if (joystickX < 400) {
      motorAcceleration -= 50;
      if (motorAcceleration < 50) motorAcceleration = 50;
      updateMotorSettings();
    }

    // Thoát menu
    if (buttonState == LOW && prevButtonState == HIGH) {
      break;
    }
    prevButtonState = buttonState;
  }
  stepper.setMaxSpeed(motorSpeed);
  stepper.setAcceleration(motorAcceleration);
  displayMainMenu();
}

// Cập nhật thông số động cơ trên LCD
void updateMotorSettings() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Toc Do: ");
  lcd.print(motorSpeed);
  lcd.setCursor(0, 1);
  lcd.print("Gia Toc: ");
  lcd.print(motorAcceleration);
  delay(200);
}

// Chạy động cơ
void runMotor() {
  lcd.clear();
  lcd.print("Chay Dong Co...");
  lcd.setCursor(0, 1);
  lcd.print("Nhan nut de Dung");

  stepper.moveTo(10000); // Quay 10.000 bước (hoặc tùy bạn điều chỉnh)

  while (stepper.distanceToGo() != 0) {
    stepper.run(); // Thực thi chuyển động

    buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW) {
      stepper.stop(); // Dừng động cơ
      break;
    }
  }

  lcd.clear();
  lcd.print("Da Dung Dong Co");
  delay(1000);
  displayMainMenu();
}
