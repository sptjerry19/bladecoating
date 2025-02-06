#include <AccelStepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ====== CHÂN ĐIỀU KHIỂN ======
// Chân Joystick và nút bấm
#define JOYSTICK_X A0
#define JOYSTICK_Y A1
#define BUTTON_PIN 2

// Chân điều khiển TB6600
#define STEP_PIN 8
#define DIR_PIN 9
#define ENABLE_PIN 10

// ====== LCD ======
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ====== ĐỘNG CƠ ======
// Khởi tạo đối tượng AccelStepper (chế độ DRIVER)
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// ====== THÔNG SỐ CƠ HỌC ======
const int mech_stepsPerRevolution = 6400;   // Bước/vòng của động cơ (theo yêu cầu mới)
const float leadScrewPitch = 5.0;           // Vít me di chuyển 5 mm mỗi vòng

// Các thông số do người dùng điều chỉnh (mặc định)
float distanceToMove = 200.0;   // mm (quãng đường di chuyển)
float timeToMove = 10.0;        // giây (thời gian di chuyển)
int motorAcceleration = 500;    // step/s² (gia tốc)

// Các biến tính toán
long totalSteps = (distanceToMove / leadScrewPitch) * mech_stepsPerRevolution; // tổng số bước cần di chuyển
float targetSpeed = totalSteps / timeToMove;  // steps/s (tốc độ mục tiêu)

// ====== MENU ======
enum MenuState {
  MAIN_MENU,
  ADJUST_PARAMS,
  RUN_MOTOR
};

MenuState menuState = MAIN_MENU;
int cursorPosition = 0;
String mainMenu[] = {"Chinh Thong So", "Bat Dau Dong Co"};
int menuCount = sizeof(mainMenu) / sizeof(mainMenu[0]);

// Trong chế độ chỉnh thông số, các mục được chỉnh:
// 0: Distance, 1: Time, 2: Acceleration
int paramIndex = 0;
const int paramCount = 3;

// ====== BIẾN ĐIỀU KHIỂN JOYSTICK ======
int joystickX = 0, joystickY = 0;
int buttonState = HIGH, prevButtonState = HIGH;

// ====== DỮ LIỆU JSON ======
String jsonData = "";
bool firstSample = true;
int sampleCounter = 0;

// ====== HÀM TIỆN ÍCH ======
void displayMainMenu();
void handleJoystick();
void executeMenuOption();
void adjustParameters();
void updateParametersDisplay();
void runMotorMotion();
void recordSample();
void moveWithSampling(long target, unsigned long moveDuration);
void handleIoTCommand(String command);

void setup() {
  // ====== KHỞI TẠO LCD ======
  lcd.begin(16, 2);
  lcd.setBacklight(true);
  lcd.clear();
  
  // Hiển thị menu chính ban đầu
  displayMainMenu();

  // ====== KHỞI TẠO JOYSTICK & NÚT ======
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // ====== KHỞI TẠO ĐỘNG CƠ ======
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW); // Kích hoạt động cơ

  // Cài đặt ban đầu cho động cơ
  stepper.setAcceleration(motorAcceleration);
  stepper.setMaxSpeed(targetSpeed);

  // ====== KHỞI TẠO SERIAL ======
  Serial.begin(9600);
  delay(1000);
}

void loop() {
  // Kiểm tra lệnh IoT từ Serial (web)
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    handleIoTCommand(command);
  }
  
  // Cập nhật đọc giá trị Joystick và nút
  joystickX = analogRead(JOYSTICK_X);
  joystickY = analogRead(JOYSTICK_Y);
  buttonState = digitalRead(BUTTON_PIN);

  // Xử lý menu hoặc điều chỉnh thông số tùy theo trạng thái hiện tại
  switch (menuState) {
    case MAIN_MENU:
      handleJoystick();
      // Nếu nhấn nút, chọn mục trong menu chính
      if (buttonState == LOW && prevButtonState == HIGH) {
        executeMenuOption();
        delay(300);
      }
      break;
      
    case ADJUST_PARAMS:
      // Trong chế độ chỉnh thông số, dùng joystick Y để tăng/giảm giá trị của mục hiện tại
      if (joystickY < 400) {  // tăng giá trị
        if (paramIndex == 0) {
          distanceToMove += 10;   // tăng 10 mm
          if(distanceToMove > 1000) distanceToMove = 1000;
        } else if (paramIndex == 1) {
          timeToMove += 1;        // tăng 1 giây
          if(timeToMove > 60) timeToMove = 60;
        } else if (paramIndex == 2) {
          motorAcceleration += 50;
          if(motorAcceleration > 2000) motorAcceleration = 2000;
        }
        updateParametersDisplay();
        delay(200);
      }
      else if (joystickY > 600) {  // giảm giá trị
        if (paramIndex == 0) {
          distanceToMove -= 10;
          if(distanceToMove < 10) distanceToMove = 10;
        } else if (paramIndex == 1) {
          timeToMove -= 1;
          if(timeToMove < 1) timeToMove = 1;
        } else if (paramIndex == 2) {
          motorAcceleration -= 50;
          if(motorAcceleration < 50) motorAcceleration = 50;
        }
        updateParametersDisplay();
        delay(200);
      }
      
      // Nếu nhấn nút, chuyển sang mục tiếp theo
      if (buttonState == LOW && prevButtonState == HIGH) {
        paramIndex++;
        if (paramIndex >= paramCount) {
          // Khi kết thúc việc chỉnh, cập nhật lại các thông số tính toán
          totalSteps = (distanceToMove / leadScrewPitch) * mech_stepsPerRevolution;
          targetSpeed = totalSteps / timeToMove;
          stepper.setMaxSpeed(targetSpeed);
          stepper.setAcceleration(motorAcceleration);
          // Quay trở lại menu chính
          menuState = MAIN_MENU;
          displayMainMenu();
        } else {
          updateParametersDisplay();
        }
        delay(300);
      }
      break;
      
    case RUN_MOTOR:
      // Khi chạy động cơ, ta thực hiện chuyển động với lấy mẫu
      runMotorMotion();
      // Sau khi hoàn thành, quay về menu chính
      menuState = MAIN_MENU;
      displayMainMenu();
      break;
  }
  
  prevButtonState = buttonState;
}

// ====== HIỂN THỊ MENU CHÍNH ======
void displayMainMenu() {
  lcd.clear();
  for (int i = 0; i < menuCount; i++) {
    lcd.setCursor(0, i);
    if (i == cursorPosition)
      lcd.print("> ");
    else
      lcd.print("  ");
    lcd.print(mainMenu[i]);
  }
}

// ====== XỬ LÝ JOYSTICK TRONG MENU CHÍNH ======
void handleJoystick() {
  // Dùng trục Y để thay đổi lựa chọn menu
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

// ====== THỰC THI LỰA CHỌN MENU ======
void executeMenuOption() {
  if (cursorPosition == 0) { 
    // Chuyển sang chế độ chỉnh thông số
    menuState = ADJUST_PARAMS;
    paramIndex = 0;
    updateParametersDisplay();
  } else if (cursorPosition == 1) {
    // Chuyển sang chạy động cơ (với chuyển động tiến - lùi và lấy mẫu)
    menuState = RUN_MOTOR;
  }
}

// ====== CHẾ ĐỘ CHỈNH THÔNG SỐ ======
void updateParametersDisplay() {
  lcd.clear();
  if (paramIndex == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Distance(mm):");
    lcd.setCursor(0, 1);
    lcd.print(distanceToMove);
  } else if (paramIndex == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Time(s):");
    lcd.setCursor(0, 1);
    lcd.print(timeToMove);
  } else if (paramIndex == 2) {
    lcd.setCursor(0, 0);
    lcd.print("Accel(step/s^2):");
    lcd.setCursor(0, 1);
    lcd.print(motorAcceleration);
  }
}

// ====== CHẠY ĐỘNG CƠ VỚI LẤY MẪU VÀ GỬI JSON ======
void runMotorMotion() {
  // Khởi tạo chuỗi JSON
  jsonData = "[\n";
  firstSample = true;
  sampleCounter = 0;
  
  // Cập nhật lại các thông số tính toán
  totalSteps = (distanceToMove / leadScrewPitch) * mech_stepsPerRevolution;
  targetSpeed = totalSteps / timeToMove;
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(motorAcceleration);
  
  // Hiển thị thông báo trên LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moving Forward");
  
  // Di chuyển tiến (với thời gian đã nhập chuyển đổi sang ms)
  moveWithSampling(totalSteps, (unsigned long)(timeToMove * 1000));
  
  // Hiển thị chuyển động ngược trên LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Returning Back");
  
  // Di chuyển lùi về vị trí ban đầu
  moveWithSampling(0, (unsigned long)(timeToMove * 1000));
  
  // Kết thúc chuyển động
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Operation Done!");
  delay(1000);
  
  // Đóng chuỗi JSON
  jsonData += "\n]";
  
  // Gửi JSON qua Serial (để web có thể lấy về)
  Serial.println(jsonData);
  
  delay(1000);
}

// ====== HÀM LẤY MẪU ======
void recordSample() {
  sampleCounter++;
  // Lấy tốc độ hiện tại (steps/s) và đổi sang mm/s:
  float currentSpeedSteps = stepper.speed(); 
  float currentSpeedMM = currentSpeedSteps * leadScrewPitch / mech_stepsPerRevolution;
  
  // Tính quãng đường đã đi (mm)
  float currentDistanceMM = abs(stepper.currentPosition()) * leadScrewPitch / mech_stepsPerRevolution;
  
  if (!firstSample) {
    jsonData += ",\n";
  }
  
  jsonData += "  { \"time\": " + String(sampleCounter) +
              ", \"speed\": " + String(currentSpeedMM, 1) +
              ", \"distance\": " + String(currentDistanceMM, 1) + " }";
  
  firstSample = false;
}

// ====== HÀM CHẠY VỚI LẤY MẪU ======
void moveWithSampling(long target, unsigned long moveDuration) {
  stepper.moveTo(target);
  unsigned long startTime = millis();
  unsigned long lastSampleTime = millis();
  
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    
    // Lấy mẫu mỗi 1 giây
    if (millis() - lastSampleTime >= 1000) {
      recordSample();
      lastSampleTime = millis();
    }
  }
  
  // Nếu thời gian chuyển động chưa đủ moveDuration, đợi thêm
  unsigned long elapsed = millis() - startTime;
  if (elapsed < moveDuration) {
    delay(moveDuration - elapsed);
  }
}

// ====== XỬ LÝ LỆNH IoT TỪ WEB ======
void handleIoTCommand(String command) {
  // Ví dụ đơn giản: các lệnh "start", "stop", "reverse", "speed_up", "speed_down"
  if (command == "start") {
    // Nếu lệnh start từ web được nhận, bắt đầu chuyển động
    runMotorMotion();
  } else if (command == "stop") {
    stepper.stop();
  } else if (command == "reverse") {
    // Đảo hướng: thay đổi nhân hướng
    // (Trong trường hợp này, bạn có thể cần xử lý lại vị trí hiện tại nếu muốn đảo chuyển động ngay)
    stepper.move(-stepper.distanceToGo());
  } else if (command == "speed_up") {
    targetSpeed += 50;
    stepper.setMaxSpeed(targetSpeed);
  } else if (command == "speed_down") {
    targetSpeed -= 50;
    if(targetSpeed < 50) targetSpeed = 50;
    stepper.setMaxSpeed(targetSpeed);
  }
}
