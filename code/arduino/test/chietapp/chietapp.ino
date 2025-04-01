#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// Cấu hình LCD I2C (địa chỉ 0x27, kích thước 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Định nghĩa chân cho driver động cơ bước (kiểu DRIVER: cần STEP và DIR)
#define STEP_PIN 8
#define DIR_PIN 9

// Chân joystick
#define JOYSTICK_X A0
#define JOYSTICK_Y A1
#define BUTTON_PIN 2
// #define MODE_SWITCH_PIN 3  // Nút chuyển đổi chế độ điều khiển

// Biến điều khiển joystick
int joystickX = 0;
int joystickY = 0;
bool buttonPressed = false;
bool modeSwitchPressed = false;
bool joystickMode = true;  // true = điều khiển joystick, false = điều khiển IOT
const int JOYSTICK_DEADZONE = 100;  // Vùng chết của joystick
const int MAX_SPEED = 200;           // Tốc độ tối đa (mm/s)
const int MIN_SPEED = 0.1;          // Tốc độ tối thiểu (mm/s)

// Khởi tạo đối tượng AccelStepper (chế độ DRIVER)
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// --- THÔNG SỐ CƠ HỌC ---
// (Không khai báo const vì sẽ thay đổi theo lệnh nhận được)
int stepsPerRevolution = 6400;    // Bước/vòng của động cơ (200 full-steps x 32 microsteps)
float leadScrewPitch = 8.0;         // 8 mm/vòng (theo vít me T8 4-start)
float distanceToMove = 200.0;       // Mặc định 200 mm (có thể cập nhật qua lệnh)
// Tổng số bước cần thiết (sẽ cập nhật theo lệnh)
long totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;

// Thời gian di chuyển (giây), mặc định 10 giây (có thể cập nhật)
float timeToMove = 10.0;

// Tốc độ mục tiêu (bước/giây) dựa trên timeToMove
float targetSpeed = totalSteps / timeToMove;

// --- THÔNG SỐ VẬT LÝ ---
// Biến tốc độ (mm/s) hiện hành (sẽ được cập nhật khi nhận lệnh)
float speed = 0.0;  // Giá trị sẽ được cập nhật từ lệnh "forward"/"backward"
// Tốc độ mặc định nếu không có lệnh kèm theo (mm/s)
float defaultSpeed = 2.0;

// --- Các biến lưu trữ dữ liệu JSON ---
String jsonData = "";
bool firstSample = true;
int sampleCounter = 0;

// --- Các biến flag điều khiển ---
bool startCommandReceived = false;
bool stopCommandReceived = false;
bool forwardCommandReceived = false;
bool backwardCommandReceived = false;


void displayMenu() {
  int menuIndex = 0; // Chỉ số menu hiện tại (0 = Thủ công, 1 = IOT)
  bool menuSelected = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("> Thủ cong");
  lcd.setCursor(0, 1);
  lcd.print("  IOT");

  while (!menuSelected) {
    joystickY = analogRead(JOYSTICK_Y); // Đọc trục Y của joystick
    buttonPressed = digitalRead(BUTTON_PIN) == LOW; // Kiểm tra nút nhấn

    // Di chuyển lên/xuống trong menu
    if (joystickY < 400) { // Joystick đẩy lên
      menuIndex = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Thủ cong");
      lcd.setCursor(0, 1);
      lcd.print("  IOT");
      delay(200); // Debounce
    } else if (joystickY > 600) { // Joystick đẩy xuống
      menuIndex = 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  Thủ cong");
      lcd.setCursor(0, 1);
      lcd.print("> IOT");
      delay(200); // Debounce
    }

    // Xử lý khi nhấn nút để chọn
    if (buttonPressed) {
      menuSelected = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Selected: ");
      lcd.print(menuIndex == 0 ? "Thủ cong" : "IOT");
      delay(1000); // Hiển thị lựa chọn trong 1 giây
    }
  }

  // Cập nhật chế độ dựa trên lựa chọn
  joystickMode = (menuIndex == 0); // 0 = Thủ công, 1 = IOT
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(joystickMode ? "Mode: Thủ cong" : "Mode: IOT");
  delay(1000);
  lcd.clear();

  // Nếu chọn chế độ thủ công, hiển thị thêm menu con
  if (joystickMode) {
    displayManualModeMenu();
  }
}

void displayManualModeMenu() {
  int manualModeIndex = 0; // Chỉ số menu con (0 = Chạy tự động, 1 = Điều khiển joystick)
  bool manualModeSelected = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("> Chay tu dong");
  lcd.setCursor(0, 1);
  lcd.print("  Dieu khien");

  while (!manualModeSelected) {
    joystickY = analogRead(JOYSTICK_Y); // Đọc trục Y của joystick
    buttonPressed = digitalRead(BUTTON_PIN) == LOW; // Kiểm tra nút nhấn

    // Di chuyển lên/xuống trong menu
    if (joystickY < 400) { // Joystick đẩy lên
      manualModeIndex = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Chay tu dong");
      lcd.setCursor(0, 1);
      lcd.print("  Dieu khien");
      delay(200); // Debounce
    } else if (joystickY > 600) { // Joystick đẩy xuống
      manualModeIndex = 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  Chay tu dong");
      lcd.setCursor(0, 1);
      lcd.print("> Dieu khien");
      delay(200); // Debounce
    }

    // Xử lý khi nhấn nút để chọn
    if (buttonPressed) {
      manualModeSelected = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Selected: ");
      lcd.print(manualModeIndex == 0 ? "Tu dong" : "Joystick");
      delay(1000); // Hiển thị lựa chọn trong 1 giây
    }
  }

  // Xử lý tùy chọn
  if (manualModeIndex == 0) {
    handleManualInput(); // Chạy tự động
  } else {
    handleJoystickSpeedControl(); // Điều khiển tốc độ bằng joystick
  }
}

void handleJoystickSpeedControl() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Joystick Control");

  while (true) {
    joystickY = analogRead(JOYSTICK_Y); // Đọc giá trị joystick
    buttonPressed = digitalRead(BUTTON_PIN) == LOW; // Kiểm tra nút nhấn

    // Tính toán tốc độ dựa trên vị trí joystick
    int centerY = 512; // Giá trị trung tâm của joystick
    float speedMultiplier = 0.0;

    if (abs(joystickY - centerY) > JOYSTICK_DEADZONE) {
      speedMultiplier = (float)(abs(joystickY - centerY) - JOYSTICK_DEADZONE) / (1024 - JOYSTICK_DEADZONE);
      speed = MIN_SPEED + (MAX_SPEED - MIN_SPEED) * speedMultiplier;

      if (joystickY > centerY) {
        stepper.setSpeed(speed); // Chuyển động tiến
        lcd.setCursor(0, 1);
        lcd.print("Forward ");
        lcd.print(speed, 1);
        lcd.print("mm/s ");
      } else {
        stepper.setSpeed(-speed); // Chuyển động lùi
        lcd.setCursor(0, 1);
        lcd.print("Backward ");
        lcd.print(speed, 1);
        lcd.print("mm/s ");
      }
    } else {
      stepper.setSpeed(0); // Dừng động cơ
      lcd.setCursor(0, 1);
      lcd.print("Stopped       ");
    }

    stepper.run();

    // Thoát chế độ nếu nhấn nút
    if (buttonPressed) {
      delay(500); // Debounce
      if (digitalRead(BUTTON_PIN) == LOW) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Exiting...");
        delay(1000);
        return; // Thoát khỏi hàm và quay lại màn hình trước đó
      }
    }
  }
}


//
// Hàm ghi nhận mẫu dữ liệu (thời gian, tốc độ, quãng đường)
//
void recordSample() {
  sampleCounter++;
  float currentSpeedSteps = stepper.speed();
  float currentSpeedMM = currentSpeedSteps * leadScrewPitch / stepsPerRevolution;
  float currentDistanceMM = abs(stepper.currentPosition()) * leadScrewPitch / stepsPerRevolution;
  
  if (!firstSample) {
    jsonData += ",\n";
  }
  jsonData += "  { \"time\": " + String(sampleCounter) +
              ", \"speed\": " + String(currentSpeedMM, 1) +
              ", \"distance\": " + String(currentDistanceMM, 1) + " }";
  firstSample = false;
}

//
// Hàm kiểm tra và xử lý lệnh nhận qua Serial (từ ESP8266)
//
void checkForIncomingCommand() {
  if (Serial.available() > 0) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();
    
    if (incoming.equalsIgnoreCase("start")) {
      Serial.println("Received start command");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cmd: START");
      startCommandReceived = true;
      stopCommandReceived = false;
      forwardCommandReceived = false;
      backwardCommandReceived = false;
    }
    else if (incoming.equalsIgnoreCase("stop")) {
      Serial.println("Received stop command");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cmd: STOP");
      stopCommandReceived = true;
    }
    else if (incoming.equalsIgnoreCase("reset")) {
      Serial.println("Received reset command");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cmd: RESET");
      jsonData = "[\n";
      firstSample = true;
      sampleCounter = 0;
      stopCommandReceived = false;
    }
    // Lệnh cập nhật thông số: dạng "D:20,S:5"
    else if (incoming.startsWith("D:") && incoming.indexOf("S:") > 0) {
      int commaIndex = incoming.indexOf(",");
      String dStr = incoming.substring(2, commaIndex);
      String sStr = incoming.substring(incoming.indexOf("S:") + 2);
      
      float newDistance = dStr.toFloat();
      float newSpeed = sStr.toFloat();
      
      if (newDistance > 0 && newSpeed > 0) {
        distanceToMove = newDistance;
        timeToMove = distanceToMove / newSpeed;
        totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;
        targetSpeed = totalSteps / timeToMove;
        stepper.setMaxSpeed(targetSpeed);
        stepper.setAcceleration(targetSpeed / 2);
        
        Serial.print("Updated parameters: Distance = ");
        Serial.print(distanceToMove);
        Serial.print(" mm, Speed = ");
        Serial.print(newSpeed);
        Serial.print(" mm/s, Time = ");
        Serial.print(timeToMove);
        Serial.println(" s");
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("D:");
        lcd.print(distanceToMove);
        lcd.print("mm");
        lcd.setCursor(0, 1);
        lcd.print("S:");
        lcd.print(newSpeed);
        lcd.print("mm/s");
      } else {
        Serial.println("Invalid D/S command values.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid D/S");
      }
    }
    // Lệnh "forward": chuyển động tiến đơn hướng
    else if (incoming.startsWith("forward")) {
      Serial.println("Received forward command");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cmd: FORWARD");
      int sIndex = incoming.indexOf("S:");
      if (sIndex != -1) {
        String speedStr = incoming.substring(sIndex + 2);
        float newSpeed = speedStr.toFloat();
        if (newSpeed > 0)
          speed = newSpeed;
        else
          speed = defaultSpeed;
      } else {
        speed = defaultSpeed;
      }
      // Reset vị trí động cơ về 0 để bắt đầu chuyển động mới
      stepper.setCurrentPosition(0);
      
      timeToMove = distanceToMove / speed;
      totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;
      targetSpeed = totalSteps / timeToMove;
      stepper.setMaxSpeed(targetSpeed);
      stepper.setAcceleration(targetSpeed / 2);
      
      forwardCommandReceived = true;
      startCommandReceived = false;
      stopCommandReceived = false;
      backwardCommandReceived = false;
    }
    // Lệnh "backward": chuyển động lùi đơn hướng
    else if (incoming.startsWith("backward")) {
      Serial.println("Received backward command");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cmd: BACKWARD");
      int sIndex = incoming.indexOf("S:");
      if (sIndex != -1) {
        String speedStr = incoming.substring(sIndex + 2);
        float newSpeed = speedStr.toFloat();
        if (newSpeed > 0)
          speed = newSpeed;
        else
          speed = defaultSpeed;
      } else {
        speed = defaultSpeed;
      }
      // Reset vị trí động cơ về 0 để bắt đầu chuyển động mới
      stepper.setCurrentPosition(0);
      
      timeToMove = distanceToMove / speed;
      totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;
      targetSpeed = totalSteps / timeToMove;
      stepper.setMaxSpeed(targetSpeed);
      stepper.setAcceleration(targetSpeed / 2);
      
      backwardCommandReceived = true;
      startCommandReceived = false;
      stopCommandReceived = false;
      forwardCommandReceived = false;
    }
  }
}

//
// Hàm di chuyển với lấy mẫu và kiểm tra lệnh từ Serial trong quá trình chuyển động
//
void moveWithSampling(long target, unsigned long moveDuration) {
  stepper.moveTo(target);
  unsigned long startTime = millis();
  unsigned long lastSampleTime = millis();
  bool stoppedEarly = false; // Biến đánh dấu nếu dừng sớm do stop
  
  while (stepper.distanceToGo() != 0) {
    if (stopCommandReceived) {
      Serial.println("Stop command processed: Stopping motor");
      stepper.moveTo(stepper.currentPosition());
      stoppedEarly = true;
      stopCommandReceived = false; // Reset flag để cho phép lệnh mới
      break;
    }
    
    stepper.run();
    checkForIncomingCommand();
    
    if (millis() - lastSampleTime >= 1000) {
      recordSample();
      lastSampleTime = millis();
    }
  }
  
  unsigned long elapsed = millis() - startTime;
  if (!stoppedEarly && (elapsed < moveDuration)) {
    delay(moveDuration - elapsed);
  }
}

//
// Hàm xử lý điều khiển bằng joystick
//
void handleJoystickControl() {
  // Đọc giá trị joystick
  joystickX = analogRead(JOYSTICK_X);
  joystickY = analogRead(JOYSTICK_Y);
  buttonPressed = digitalRead(BUTTON_PIN) == LOW;  // Pull-up resistor

  // Xử lý chuyển đổi chế độ khi nhấn giữ nút joystick
  if (buttonPressed) {
    delay(50);  // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {  // Kiểm tra lại sau debounce
      joystickMode = !joystickMode;  // Đảo ngược chế độ
      stepper.setSpeed(0);  // Dừng động cơ khi chuyển chế độ
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(joystickMode ? "Mode: Thủ công" : "Mode: IOT");
      delay(1000);
      lcd.clear();
      return;
    }
  }

  // Chỉ xử lý joystick nếu đang ở chế độ joystick
  if (!joystickMode) return;

  // Tính toán tốc độ dựa trên vị trí joystick
  int centerX = 512;  // Giá trị trung tâm của joystick
  float speedMultiplier = 0.0;

  // Xử lý hướng và tốc độ dựa trên trục X
  if (abs(joystickX - centerX) > JOYSTICK_DEADZONE) {
    speedMultiplier = (float)(abs(joystickX - centerX) - JOYSTICK_DEADZONE) / (1024 - JOYSTICK_DEADZONE);
    speed = MIN_SPEED + (MAX_SPEED - MIN_SPEED) * speedMultiplier;

    // Xác định hướng chuyển động
    if (joystickX > centerX) {
      stepper.setSpeed(targetSpeed);  // Chuyển động tiến
      lcd.setCursor(0, 1);
      lcd.print("Forward ");
      lcd.print(speed, 1);
      lcd.print("mm/s");
    } else {
      stepper.setSpeed(-targetSpeed);  // Chuyển động lùi
      lcd.setCursor(0, 1);
      lcd.print("Backward ");
      lcd.print(speed, 1);
      lcd.print("mm/s");
    }
  } else {
    stepper.setSpeed(0);  // Dừng động cơ
    lcd.setCursor(0, 1);
    lcd.print("Stopped");
  }

  // Xử lý các lệnh bổ sung trong chế độ thủ công
  if (joystickY < 400) {  // Joystick đẩy lên để bắt đầu
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cmd: START");
    stepper.setCurrentPosition(0); // Reset vị trí
    moveWithSampling(-totalSteps, (unsigned long)(timeToMove * 1000));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Forward Done");
  } else if (joystickY > 600) {  // Joystick đẩy xuống để lùi
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cmd: BACKWARD");
    stepper.setCurrentPosition(0); // Reset vị trí
    moveWithSampling(totalSteps, (unsigned long)(timeToMove * 1000));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Backward Done");
  } else if (buttonPressed) {  // Nhấn nút để reset
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cmd: RESET");
    jsonData = "[\n";
    firstSample = true;
    sampleCounter = 0;
    stopCommandReceived = false;
    stepper.setCurrentPosition(0); // Reset vị trí động cơ
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System Reset");
  }
}

void handleManualInput() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nhap quang duong:");
  delay(1000);

  // Nhập quãng đường bằng joystick
  int inputDistance = 0;
  while (true) {
    joystickY = analogRead(JOYSTICK_Y); // Đọc giá trị joystick
    buttonPressed = digitalRead(BUTTON_PIN) == LOW; // Kiểm tra nút nhấn

    if (joystickY < 400) { // Joystick đẩy lên
      inputDistance += 1; // Tăng quãng đường
    } else if (joystickY > 600) { // Joystick đẩy xuống
      inputDistance = max(0, inputDistance - 1); // Giảm quãng đường, không âm
    }

    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(inputDistance);
    lcd.print(" mm   ");

    if (buttonPressed) { // Nhấn nút để xác nhận
      delay(500); // Debounce
      if (digitalRead(BUTTON_PIN) == LOW) break;
    }

    delay(200); // Để tránh thay đổi quá nhanh
  }
  distanceToMove = inputDistance;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nhap toc do:");
  delay(1000);

  // Nhập tốc độ bằng joystick
  float inputSpeed = 0;
  while (true) {
    joystickY = analogRead(JOYSTICK_Y); // Đọc giá trị joystick
    buttonPressed = digitalRead(BUTTON_PIN) == LOW; // Kiểm tra nút nhấn

    if (joystickY < 400) { // Joystick đẩy lên
      inputSpeed += 0.1; // Tăng tốc độ
    } else if (joystickY > 600) { // Joystick đẩy xuống
      inputSpeed = max(0.0f, inputSpeed - 0.1); // Giảm tốc độ, không âm
    }

    lcd.setCursor(0, 1);
    lcd.print("S:");
    lcd.print(inputSpeed, 1);
    lcd.print(" mm/s   ");

    if (buttonPressed) { // Nhấn nút để xác nhận
      delay(500); // Debounce
      if (digitalRead(BUTTON_PIN) == LOW) break;
    }

    delay(200); // Để tránh thay đổi quá nhanh
  }
  speed = inputSpeed;

  // Tính toán các thông số
  timeToMove = distanceToMove / speed;
  totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;
  targetSpeed = totalSteps / timeToMove;
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(targetSpeed / 2);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("D:");
  lcd.print(distanceToMove);
  lcd.print("mm");
  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.print(speed);
  lcd.print("mm/s");
  delay(2000);

  // Chạy động cơ
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Running...");
  stepper.setCurrentPosition(0);
  moveWithSampling(-totalSteps, (unsigned long)(timeToMove * 1000));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Done!");
  delay(2000);
}

//
// setup()
//
void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();

  // Thiết lập chân joystick
  pinMode(JOYSTICK_X, INPUT);
  pinMode(JOYSTICK_Y, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  timeToMove = 10.0;          // 10 giây
  distanceToMove = 200.0;     // 200 mm
  totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;
  targetSpeed = totalSteps / timeToMove;
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(targetSpeed / 2);

  Serial.begin(9600);
  delay(1000);

  // Hiển thị menu khi khởi động
  displayMenu();

  jsonData = "[\n";
  firstSample = true;
  sampleCounter = 0;
}

//
// loop()
//
void loop() {
  // Xử lý điều khiển joystick
  handleJoystickControl();
  
  // Chỉ xử lý lệnh IOT nếu đang ở chế độ IOT
  if (!joystickMode) {
    checkForIncomingCommand();
    
    if (startCommandReceived) {
      startCommandReceived = false;
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Moving Forward");
      stepper.setCurrentPosition(0); // Reset vị trí
      moveWithSampling(-totalSteps, (unsigned long)(timeToMove * 1000));
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Returning Back");
      moveWithSampling(0, (unsigned long)(timeToMove * 1000));
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Operation Done!");
      
      jsonData += "\n]";
      Serial.println(jsonData);
      
      jsonData = "[\n";
      firstSample = true;
      sampleCounter = 0;
    }
    else if (forwardCommandReceived) {
      forwardCommandReceived = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Moving Forward");
      stepper.setCurrentPosition(0); // Reset vị trí
      moveWithSampling(-totalSteps, (unsigned long)(timeToMove * 1000));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Forward Done");
    }
    else if (backwardCommandReceived) {
      backwardCommandReceived = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Moving Backward");
      stepper.setCurrentPosition(0); // Reset vị trí
      moveWithSampling(totalSteps, (unsigned long)(timeToMove * 1000));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Backward Done");
    }
  }
  
  // Chạy động cơ
  stepper.run();
  
  delay(10);
}
