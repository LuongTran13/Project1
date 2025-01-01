#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo s1;

// Cấu hình bàn phím
const byte numRows = 4;
const byte numCols = 4;
char keymap[numRows][numCols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[numRows] = {2, 3, 4, 5};
byte colPins[numCols] = {6, 7, 8, 9};
Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);

// Biến toàn cục
char fixedPassword[] = "1234"; // Mật khẩu không thể thay đổi
char inputPassword[5];         // Mật khẩu người dùng nhập
int storedPasswordsStart = 10; // Vị trí EEPROM lưu mật khẩu động
int numPasswords = 0;          // Số lượng mật khẩu hiện tại
const int maxPasswords = 5; // Số lượng mật khẩu tối đa
char passwords[maxPasswords][5]; // Mảng chứa mật khẩu

int incorrectAttempts = 0;     // Đếm số lần nhập sai
const int buzzerPin = 13;      // Chân nối còi báo

void setup() {
  Serial.begin(9600);
  s1.attach(12);
  s1.write(0);
  pinMode(buzzerPin, OUTPUT);  // Cấu hình chân còi
  lcd.init();
  lcd.backlight();
  lcd.print("Khoa Thong Minh!");
  delay(2000);
  lcd.clear();
  lcd.print("Nhap Mat Khau:");
}

void loop() {
  char key = myKeypad.getKey();
  
  if (key) {
    static int pos = 0; // Vị trí nhập mật khẩu
    if (key == '#') {
      inputPassword[pos] = '\0'; // Kết thúc chuỗi nhập
      pos = 0; // Reset vị trí nhập
      
      if (checkPassword(inputPassword)) {
        unlockDoor();
        incorrectAttempts = 0; // Reset số lần nhập sai khi đúng
      } else {
        incorrectAttempts++; // Tăng số lần nhập sai
        lcd.clear();
        lcd.print("Mat Khau Sai!");
        delay(2000);
        lcd.clear();
        lcd.print("Nhap Mat Khau:");
        // Nếu nhập sai từ lần thứ 3 trở lên, kêu còi
        if (incorrectAttempts >= 3) {
          triggerBuzzer();
        }
      }
    } else if (key == 'A') {
      addPassword();
    } else if (key == 'B') {
      changePassword();
    } else if (pos < 4) {
      inputPassword[pos++] = key;
      lcd.setCursor(pos - 1, 1);
      lcd.print('*');
    }
  }
}

bool checkPassword(const char* password) {
  // Kiểm tra mật khẩu cố định
  if (strcmp(password, fixedPassword) == 0) {
    return true;
  }

  // Kiểm tra mật khẩu đã lưu trong EEPROM
  char storedPassword[5];
  for (int i = 0; i < numPasswords; i++) {
    for (int j = 0; j < 4; j++) {
      storedPassword[j] = EEPROM.read(storedPasswordsStart + i * 4 + j);
    }
    storedPassword[4] = '\0'; // Kết thúc chuỗi

    if (strcmp(password, storedPassword) == 0) {
      return true;
    }
  }
  return false;
}

void unlockDoor() {
  lcd.clear();
  lcd.print("Mo cua");
  s1.write(180);
  delay(5000);
  s1.write(0);
  lcd.clear();
  lcd.print("Dong cua");
  delay(2000);
  lcd.clear();
  lcd.print("Nhap Mat Khau:");
}

void addPassword() {
  char masterPassword[5];  // Mật khẩu chủ cần kiểm tra
  lcd.clear();
  lcd.print("Mat Khau Chinh:");
  
  int pos = 0;
  while (pos < 4) {
    char key = myKeypad.getKey();
    if (key) {
      masterPassword[pos++] = key;
      lcd.setCursor(pos - 1, 1);
      lcd.print('*');
    }
  }
  masterPassword[4] = '\0'; // Kết thúc chuỗi

  // Kiểm tra mật khẩu chủ
  if (strcmp(masterPassword, fixedPassword) != 0) {
    lcd.clear();
    lcd.print("Mat Khau Sai!");
    delay(2000);
    lcd.clear();
    lcd.print("Nhap Mat Khau:");
    return; // Không cho phép thêm mật khẩu
  }

  // Tiếp tục thêm mật khẩu nếu mật khẩu chủ đúng
  if (numPasswords >= maxPasswords) {
    lcd.clear();
    lcd.print("Mat Khau Day");
    delay(2000);
    lcd.clear();
    lcd.print("Nhap Mat Khau:");
    return;
  }

  lcd.clear();
  lcd.print("Them Mat Khau:");
  char newPassword[5];
  pos = 0;

  while (pos < 4) {
    char key = myKeypad.getKey();
    if (key) {
      newPassword[pos++] = key;
      lcd.setCursor(pos - 1, 1);
      lcd.print('*');
    }
  }

  newPassword[4] = '\0'; // Kết thúc chuỗi

  // Lưu mật khẩu vào EEPROM
  for (int i = 0; i < 4; i++) {
    EEPROM.write(storedPasswordsStart + numPasswords * 4 + i, newPassword[i]);
  }

  // Lưu mật khẩu vào mảng `passwords`
  strcpy(passwords[numPasswords], newPassword);
  numPasswords++; // Tăng số lượng mật khẩu
  lcd.clear();
  lcd.print("Them Thanh cong");
  delay(2000);
  lcd.clear();
  lcd.print("Nhap Mat Khau:");
}


void changePassword() {
  char oldPassword[5];  // Mật khẩu cũ cần kiểm tra
  char newPassword[5];  // Mật khẩu mới sẽ thay thế
  int matchedIndex = -1; // Lưu chỉ mục của mật khẩu cũ nếu tìm thấy

  lcd.clear();
  lcd.print("Doi Mat Khau:");
  delay(1000);
  lcd.clear();
  lcd.print("Nhap Mat Khau:");

  // Nhập mật khẩu cần thay đổi
  int pos = 0;
  while (pos < 4) {
    char key = myKeypad.getKey();
    if (key) {
      oldPassword[pos++] = key;
      lcd.setCursor(pos - 1, 1);
      lcd.print("*");
    }
  }
  oldPassword[4] = '\0'; // Kết thúc chuỗi

  // Kiểm tra mật khẩu có tồn tại không (trừ mật khẩu 1234)
  for (int i = 0; i < numPasswords; i++) {
    if (strcmp(oldPassword, passwords[i]) == 0) {
      matchedIndex = i; // Mật khẩu hợp lệ, lưu chỉ mục
      break;
    }
  }

  if (matchedIndex == -1) { // Không tìm thấy mật khẩu
    lcd.clear();
    lcd.print("Khong Chinh Xac");
    delay(2000);
    lcd.clear();
    lcd.print("Nhap Mat Khau:");
    return;
  }

  // Cho phép nhập mật khẩu mới
  lcd.clear();
  lcd.print("Mat Khau Moi:");
  pos = 0;
  while (pos < 4) {
    char key = myKeypad.getKey();
    if (key) {
      newPassword[pos++] = key;
      lcd.setCursor(pos - 1, 1);
      lcd.print("*");
    }
  }
  newPassword[4] = '\0'; // Kết thúc chuỗi

  // Ghi mật khẩu mới vào vị trí cũ trong mảng passwords
  strcpy(passwords[matchedIndex], newPassword);

  // Ghi mật khẩu mới vào EEPROM
  for (int i = 0; i < 4; i++) {
      EEPROM.write(storedPasswordsStart + matchedIndex * 4 + i, newPassword[i]);
  }

  lcd.clear();
  lcd.print("Da Doi Mat Khau");
  delay(2000);
  lcd.clear();
  lcd.print("Nhap Mat Khau:");
}
void triggerBuzzer() {
  digitalWrite(buzzerPin, HIGH); // Bật còi
  delay(1000);                   // Kêu trong 1 giây
  digitalWrite(buzzerPin, LOW);  // Tắt còi
}

