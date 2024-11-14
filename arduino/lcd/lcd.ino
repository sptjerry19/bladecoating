#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Khởi tạo màn hình LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2); // Địa chỉ 0x27, màn hình 16x2

// Các chân cho phím bấm
const int upButton = 2;
const int downButton = 3;
const int selectButton = 4;

int menuIndex = 0; // Biến chỉ mục menu hiện tại

String menuOptions[] = {"Option 1", "Option 2", "Option 3", "Option 4"}; // Các lựa chọn menu

void setup() {
  // Khởi tạo màn hình LCD
  lcd.init();
  lcd.backlight();

  // Cấu hình các phím bấm
  pinMode(upButton, INPUT);
  pinMode(downButton, INPUT);
  pinMode(selectButton, INPUT);
}

void loop() {
  // Hiển thị menu
  displayMenu();

  // Kiểm tra các phím bấm
  if (digitalRead(upButton) == HIGH) {
    menuIndex--;
    if (menuIndex < 0) {
      menuIndex = 3; // Quay lại cuối danh sách
    }
    delay(200); // Chờ sau mỗi lần bấm phím
  }
  
  if (digitalRead(downButton) == HIGH) {
    menuIndex++;
    if (menuIndex > 3) {
      menuIndex = 0; // Quay lại đầu danh sách
    }
    delay(200); // Chờ sau mỗi lần bấm phím
  }

  if (digitalRead(selectButton) == HIGH) {
    // Thực hiện hành động khi lựa chọn được chọn
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Selected: " + menuOptions[menuIndex]);
    delay(1000); // Chờ 1 giây để hiển thị lựa chọn
  }
}

void displayMenu() {
  // Xóa màn hình và hiển thị menu
  lcd.clear();
  
  // Hiển thị các lựa chọn menu
  for (int i = 0; i < 4; i++) {
    if (i == menuIndex) {
      lcd.setCursor(0, i);      // Đặt con trỏ tại dòng i
      lcd.print("> " + menuOptions[i]); // In dấu ">" cho lựa chọn hiện tại
    } else {
      lcd.setCursor(0, i);      // Đặt con trỏ tại dòng i
      lcd.print("  " + menuOptions[i]); // In các lựa chọn khác
    }
  }
}
