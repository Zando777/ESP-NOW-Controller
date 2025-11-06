#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// I2C Pins
#define SDA_PIN 8
#define SCL_PIN 9

// Left Joystick pins - Using available ADC pins
#define LEFT_VRX 4  // ADC1_CH4
#define LEFT_VRY 3  // ADC1_CH3 (was aux switch)
#define LEFT_SW 5

// Right Joystick pins
#define RIGHT_VRX 2  // ADC1_CH2
#define RIGHT_VRY 1  // ADC1_CH1
#define RIGHT_SW 0

// Aux toggle switch - Moved to non-ADC pin
#define AUX_SWITCH 6  // Digital input only (not ADC)

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables to store joystick values
int leftX, leftY, rightX, rightY;
bool leftButton, rightButton, auxSwitch;

// Welcome screen timing
unsigned long welcomeStartTime = 0;
bool welcomeShown = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  // Rotate display 180 degrees (upside down)
  display.setRotation(2);
  
  // Configure joystick pins
  pinMode(LEFT_VRX, INPUT);
  pinMode(LEFT_VRY, INPUT);
  pinMode(LEFT_SW, INPUT_PULLUP);
  
  pinMode(RIGHT_VRX, INPUT);
  pinMode(RIGHT_VRY, INPUT);
  pinMode(RIGHT_SW, INPUT_PULLUP);
  
  // Configure aux switch
  pinMode(AUX_SWITCH, INPUT_PULLUP);
  
  // Show welcome message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(F("ESP-NOW"));
  display.setCursor(5, 35);
  display.println(F("Controller"));
  display.display();
  
  welcomeStartTime = millis();
  welcomeShown = true;
  
  Serial.println("========================================");
  Serial.println("ESP-NOW Controller Started");
  Serial.println("========================================");
  Serial.println("Pin Configuration:");
  Serial.println("  OLED: SDA=GPIO8, SCL=GPIO9");
  Serial.println("  Left Joystick: VRX=GPIO4, VRY=GPIO3, SW=GPIO5");
  Serial.println("  Right Joystick: VRX=GPIO2, VRY=GPIO1, SW=GPIO0");
  Serial.println("  Aux Switch: GPIO6");
  Serial.println("  Note: GPIO 0-4 are the only reliable ADC pins on ESP32-C3");
  Serial.println("========================================");
}

void loop() {
  // Show welcome screen for 2 seconds
  if (welcomeShown && (millis() - welcomeStartTime < 2000)) {
    return;
  }
  welcomeShown = false;
  
  // Read all analog inputs
  leftX = analogRead(LEFT_VRX);
  leftY = analogRead(LEFT_VRY);
  rightX = analogRead(RIGHT_VRX);
  rightY = analogRead(RIGHT_VRY);
  
  // Read digital inputs (active LOW with pullup)
  leftButton = !digitalRead(LEFT_SW);
  rightButton = !digitalRead(RIGHT_SW);
  auxSwitch = !digitalRead(AUX_SWITCH);
  
  // Clear display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Display Left Joystick
  display.setCursor(0, 0);
  display.print(F("L: X:"));
  display.print(leftX);
  display.print(F(" Y:"));
  display.print(leftY);
  
  display.setCursor(0, 10);
  display.print(F("L-BTN: "));
  display.print(leftButton ? F("ON") : F("OFF"));
  
  // Display Right Joystick
  display.setCursor(0, 22);
  display.print(F("R: X:"));
  display.print(rightX);
  display.print(F(" Y:"));
  display.print(rightY);
  
  display.setCursor(0, 32);
  display.print(F("R-BTN: "));
  display.print(rightButton ? F("ON") : F("OFF"));
  
  // Display Aux Switch
  display.setCursor(0, 44);
  display.print(F("AUX: "));
  display.print(auxSwitch ? F("ON") : F("OFF"));
  
  // Display joystick directions
  display.setCursor(0, 54);
  
  // Left joystick direction
  if (leftX < 1000) display.print(F("L<"));
  else if (leftX > 3000) display.print(F("L>"));
  else if (leftY < 1000) display.print(F("LD"));
  else if (leftY > 3000) display.print(F("LU"));
  else display.print(F("L-"));
  
  display.print(F(" "));
  
  // Right joystick direction
  if (rightX < 1000) display.print(F("R<"));
  else if (rightX > 3000) display.print(F("R>"));
  else if (rightY < 1000) display.print(F("RD"));
  else if (rightY > 3000) display.print(F("RU"));
  else display.print(F("R-"));
  
  // Update display
  display.display();
  
  // Serial debugging output
  Serial.println("----------------------------------------");
  Serial.print("Time: "); Serial.println(millis());
  
  Serial.println("\n[Left Joystick]");
  Serial.print("  VRX (GPIO4): "); Serial.print(leftX);
  if (leftX < 1000) Serial.println(" -> LEFT");
  else if (leftX > 3000) Serial.println(" -> RIGHT");
  else Serial.println(" -> CENTER");
  
  Serial.print("  VRY (GPIO3):  "); Serial.print(leftY);
  if (leftY < 1000) Serial.println(" -> DOWN");
  else if (leftY > 3000) Serial.println(" -> UP");
  else Serial.println(" -> CENTER");
  
  Serial.print("  SW (GPIO5):   "); Serial.println(leftButton ? "PRESSED" : "RELEASED");
  
  Serial.println("\n[Right Joystick]");
  Serial.print("  VRX (GPIO2):  "); Serial.print(rightX);
  if (rightX < 1000) Serial.println(" -> LEFT");
  else if (rightX > 3000) Serial.println(" -> RIGHT");
  else Serial.println(" -> CENTER");
  
  Serial.print("  VRY (GPIO1):  "); Serial.print(rightY);
  if (rightY < 1000) Serial.println(" -> DOWN");
  else if (rightY > 3000) Serial.println(" -> UP");
  else Serial.println(" -> CENTER");
  
  Serial.print("  SW (GPIO0):   "); Serial.println(rightButton ? "PRESSED" : "RELEASED");
  
  Serial.println("\n[Aux Switch]");
  Serial.print("  GPIO6:        "); Serial.println(auxSwitch ? "ON" : "OFF");
  
  delay(50); // Small delay to avoid flickering
}