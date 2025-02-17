#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// Cấu hình LCD I2C (địa chỉ 0x27, kích thước 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Định nghĩa chân cho driver động cơ bước (kiểu DRIVER: cần STEP và DIR)
#define STEP_PIN 8
#define DIR_PIN 9

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
// setup()
//
void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  
  timeToMove = 10.0;          // 10 giây
  distanceToMove = 200.0;       // 200 mm
  totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;
  targetSpeed = totalSteps / timeToMove;
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(targetSpeed / 2);
  
  Serial.begin(9600);
  delay(1000);
  
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(timeToMove);
  lcd.print(" s");
  delay(1500);
  lcd.clear();
  
  jsonData = "[\n";
  firstSample = true;
  sampleCounter = 0;
}

//
// loop()
//
void loop() {
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
  
  delay(10);
}
