#include <Arduino.h>    
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// constants for keypad
#define ROWS 4
#define COLS 4

// Pin to use to send signals to WS2812B
#define LED_PIN 11

// Number of WS2812B LEDs attached to the Arduino
#define LED_COUNT 256

String sendData;
String CurrententeredNumber = ""; // Variable to store entering ID number
String enteredNumber = "";        // Variable to store entered ID number

volatile bool keypadflag = false;
volatile bool timerFlag = false;
volatile bool interruptFlag = false; 

unsigned char count = 0;
unsigned char val = 0;

const char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Define GPIO pins for rows and columns
const char ROW_PINS[ROWS] = {25, 24, 23, 22}; // Change these values according to your wiring
const char COL_PINS[COLS] = {12, 13, 14, 15}; // Change these values according to your wiring

const int slaveSelectPin = 17; // You can choose any GPIO pin for SS/CS
const int interruptPin = 20;   // GPIO pin to trigger interrupt

// Variables to store timer values
unsigned long previousMillis = 0;   // will store last time LED was updated
const long interval = 20000;         // interval at which to blink (milliseconds)

// variable to store url
String url = "https://erp.themaestro.in/webservice/getproductslocation?id=";

// Set the LCD address (you might need to change this)
LiquidCrystal_I2C lcd(0x27, 20, 4);
// Setting up the NeoPixel library
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void handleInterrupt()
{
  interruptFlag = true;
}

char scanKeypad()
{
  char k = 0;
  for (int row = 0; row < ROWS; ++row)
  {
    digitalWrite(ROW_PINS[row], LOW);

    for (int col = 0; col < COLS; ++col)
    {
      if (digitalRead(COL_PINS[col]) == LOW)
      {
        k = keys[row][col];
        while (digitalRead(COL_PINS[col]) == LOW)
          ; // Wait for key release
      }
    }

    digitalWrite(ROW_PINS[row], HIGH);
  }
  return k;
}

// void printBinary64(uint64_t value) {
//   for (int i = 63; i >= 0; --i) {
//     Serial.print((value >> i) & 0x01);
//   }
//   Serial.println();
// }

uint8_t reverseBits(uint8_t value) {
  uint8_t result = 0;
  for (int i = 0; i < 8; ++i) {
    result |= ((value >> i) & 0x01) << (7 - i);
  }
  return result;
} 

void rack_find_fun(uint64_t resVal)
{
  uint64_t manipulatedValue = 0;
  uint64_t mask = 0xFF;
  bool reverse = false;  // Flag to determine if the bits should be reversed

  for (int shift = 0; shift < 64; shift += 8) {
    // Extract 8 bits (1 byte)
    uint8_t byteValue = (resVal >> shift) & mask;

    // If the reverse flag is true, reverse the bits in the byte
    if (reverse) {
      byteValue = reverseBits(byteValue);
    }

    // Combine the byte into the manipulated value
    manipulatedValue |= (uint64_t)byteValue << shift;

    // Toggle the reverse flag for the next iteration
    reverse = !reverse;
  }

  // Serial.print("Manipulated Value: ");
  // printBinary64(manipulatedValue);

  // Loop through each bit of the 64-bit value
  for(int i = 0; i < 64; i++) {
    // Check if the i-th bit is set
    if(manipulatedValue & (1ULL << i)) {
      // Set the corresponding 4 LEDs to green
      for(int j = 0; j < 4; j++) {
        int ledIndex = i * 4 + j;
        if(ledIndex < LED_COUNT) {
          strip.setPixelColor(ledIndex, 255, 0, 0);
        } 
      }
    }
  }
  strip.show();   // Send the updated pixel colors to the hardware.
}

int getArrayLength(String arr[])
{
  int length = 0;
  // Iterate through the array until you find the end (assuming the array is terminated with a sentinel value)
  while (arr[length] != "null" && arr[length] != NULL)
  {
    length++;
  }
  return length;
}

void match_fun(String str[])
{
  // Initialize a 2D array to store positions
  String grid[8][8];

  // Loop through rows and columns to generate positions
  for (int row = 0; row < 8; row++)
  {
    for (int col = 0; col < 8; col++)
    {
      // Create the position string using the row and column indices
      grid[row][col] = "R" + String(row + 1) + "C" + String(col + 1);
    }
  }

  // Calculate the number of elements in the array
  int arrayLength = getArrayLength(str);

  // Serial.print("Number of elements in the array: ");
  // Serial.println(arrayLength);

  // Initialize an array to store generated numbers for matched elements
  int matchedNumbers[arrayLength];

  // Match and generate numbers for matched elements
  //Serial.println("match starts");
  for (int i = 0; i < arrayLength; i++)
  {
    for (int row = 0; row < 8; row++)
    {
      for (int col = 0; col < 8; col++)
      {
        if (grid[row][col] == str[i])
        {
          // Generate a number for the matched element
          int number = row * 8 + col + 1;
          // Serial.print("Match found: ");
          // Serial.print(str[i]);
          // Serial.print(" corresponds to number ");
          // Serial.println(number);

          // Store the generated number in the array
          matchedNumbers[i] = number;
        }
      }
    }
  }
  // Print the array of generated numbers
  // Serial.print("Array of generated numbers for matched elements: ");
  // for (int i = 0; i < arrayLength; i++)
  // {
  //   Serial.print(matchedNumbers[i]);
  //   Serial.print(" ");
  // }
  //Serial.println();

  // Create an 8-byte value
  uint64_t result = 0;

  // Set bits at positions corresponding to matched numbers
  for (int i = 0; i < arrayLength; i++)
  {
    if (matchedNumbers[i] >= 1 && matchedNumbers[i] <= 64)
    {
      result |= (uint64_t(1) << (matchedNumbers[i] - 1));
    }
  }
  //uint64_t binaryValue = 0;
  // Print the 8-byte value in binary
  // Serial.print("8-byte value in binary: ");
  // for (int i = 63; i >= 0; i--)
  // {
  //   binaryValue = (result >> i) & 1;
  //   Serial.print(binaryValue);
  // }
  //Serial.println();
  rack_find_fun(result);
}

void split_fun(String inputString)
{
  // Set the maximum number of tokens
  const int maxTokens = 64;

  // Array to store tokens
  String tokens[maxTokens];

  int startIndex = 0;
  int endIndex = 0;
  int tokenCount = 0;

  while (startIndex < inputString.length() && tokenCount < maxTokens)
  {
    // Find the index of the next comma or the end of the string
    endIndex = inputString.indexOf(", ", startIndex);

    // If no comma is found, use the end of the string
    if (endIndex == -1)
    {
      endIndex = inputString.length();
    }

    // Extract the substring
    String token = inputString.substring(startIndex, endIndex);
    tokens[tokenCount] = token;
    tokenCount++;

    // Move the start index to the next character after the comma
    startIndex = endIndex + 2;
  }

  // Print the tokens
  // for (int i = 0; i < tokenCount; i++)
  // {
  //   Serial.println(tokens[i]);
  // }
  match_fun(tokens);
}

void location_lcd_print(String location)
{
  // Calculate available space on the first line
  int availableSpaceLine1 = 20 - 15; // 11 is the length of "Location : "
  String resultString = "";

  for (int i = 0; i < location.length(); i++) {
    if (location[i] != ' ') {
      resultString += location[i];
    }
  }
  if (resultString.length() <= availableSpaceLine1) {
    lcd.print(location);
  } else {
    // Print as much as can fit on the first line
    lcd.print(resultString.substring(0, availableSpaceLine1));

    // Check if remaining text fits on the second line
    int availableSpaceLine2 = 20;
    String remainingText = resultString.substring(availableSpaceLine1);
    
    if (remainingText.length() <= availableSpaceLine2) {
      lcd.setCursor(0, 1);
      lcd.print(remainingText);
    } else {
      // Print as much as can fit on the second line
      lcd.setCursor(0, 1);
      lcd.print(remainingText.substring(0, availableSpaceLine2));

      // Print the remaining text on the third line
      lcd.setCursor(0, 2);
      lcd.print(remainingText.substring(availableSpaceLine2));
      // Check if remaining text fits on the third line
      int availableSpaceLine3 = 20;
      String remainingTextLine3 = remainingText.substring(availableSpaceLine2);

      if (remainingTextLine3.length() <= availableSpaceLine3) {
        lcd.setCursor(0, 2);
        lcd.print(remainingTextLine3);
      } 
      else {
        // Print as much as can fit on the third line
        lcd.setCursor(0, 2);
        lcd.print(remainingTextLine3.substring(0, availableSpaceLine3));

        // Print the remaining text on the fourth line
        lcd.setCursor(0, 3);
        lcd.print(remainingTextLine3.substring(availableSpaceLine3));
      }
    }
  }
}

void Json_parse_fun(String jsonString)
{
  // Parse JSON
  StaticJsonDocument<256> doc; // Adjust the size based on your JSON structure
  DeserializationError error = deserializeJson(doc, jsonString);

  // Check for parsing errors
  if (error)
  {
    // Serial.print(F("JSON parsing failed: "));
    // Serial.println(error.c_str());
    return;
  }
  // Access parsed values
  int status = doc["status"];
  String msg = doc["msg"];
  String location = doc["locations"];
  int noOfCells = doc["noOfCells"];

  // Use the parsed values as needed
  // Serial.print(F("status: "));
  // Serial.println(status);
  // Serial.print(F("msg: "));
  // Serial.println(msg);
  // Serial.print(F("locations: "));
  // Serial.println(location);
  // Serial.print(F("noOfCells: "));
  // Serial.println(noOfCells);
  if(msg == "Data not found")
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Location Not Found");
    lcd.setCursor(0, 2);
    lcd.print("Press CLR/BCK to end");
    lcd.setCursor(0, 3);
    lcd.print("Process");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Location Found:");
    location_lcd_print(location);
    split_fun(location);
  }
}
void urlSendfun(String idstr)
{
  digitalWrite(slaveSelectPin, LOW);
  sendData = url + idstr;
  //Serial.println();
  //Serial.println(sendData);
  sendData += "    ";
  for (char c : sendData)
  {
    SPI.transfer(c);
  }
  digitalWrite(slaveSelectPin, HIGH);
  count = 1;
}

int test_WiFi()
{
  while (interruptFlag != true)
  {
    delay(500);
  }
  interruptFlag = false;
  digitalWrite(slaveSelectPin, LOW); // Select the slave
  // Receive string data from the slave
  String WiFiStatus = "";
  while (true)
  {
    char c = SPI.transfer(0); // Send dummy byte to receive data
    if (c == '\0')
    {
      break; // Null character indicates the end of the string
    }
    WiFiStatus += c;
  }
  digitalWrite(slaveSelectPin, HIGH); // Deselect the slave
  //Serial.println("Received Data: " + receivedData);
  if(WiFiStatus == "WiFi connected")
  {
    return 1;
  }
  return 0;
}

void setup()
{
  Serial.begin(115200);
  strip.begin();           // Initialize NeoPixel object
  strip.setBrightness(255); // Set BRIGHTNESS to about 4% (max = 255)
  strip.clear();           // Turn off all LEDs
  strip.show();            // Send the updated pixel colors to the hardware.
  SPI.begin();
  delay(500);
  for (int i = 0; i < ROWS; ++i)
  {
    pinMode(ROW_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], HIGH);
  }
  for (int i = 0; i < COLS; ++i)
  {
    pinMode(COL_PINS[i], INPUT_PULLUP);
  }

  pinMode(slaveSelectPin, OUTPUT);
  // Setup interrupt pin
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);

  // Initialize the LCD with the number of columns and rows
  lcd.begin();
  // Clear the LCD
  lcd.clear();
  // Turn on the backlight (if available)
  lcd.backlight();
  lcd.setCursor(1,0);
  lcd.print("Maestro Technology");
  lcd.setCursor(2,1);
  lcd.print("Rack Identifier");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("Connecting to WiFi..");

  if(test_WiFi())
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("WiFi Connected.");
    delay(2000);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  // Print a message to the LCD
  lcd.print("Enter The Issue ID: ");
}

void loop()
{
  char key;
  unsigned long currentMillis = millis();  // get the current time
  key = scanKeypad();
  delay(100); // Adjust the delay according to your needs
  if (key != 0)
  {
    if ((key == 'A') && (keypadflag == false))
    {
      enteredNumber = CurrententeredNumber;
      CurrententeredNumber = "";
      if((enteredNumber=="")&&(keypadflag == false))
      {
        lcd.setCursor(2, 3); 
        lcd.print("ID is not Valid");
        delay(2000);
        lcd.setCursor(0, 3); 
        lcd.print("                    ");
        keypadflag = false;
      }
      else{
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Processing.....");
        keypadflag = true;
        urlSendfun(enteredNumber);
      }
    }
    else if(key=='D')
    {
      CurrententeredNumber = "";
      strip.clear();           // Turn off all LEDs
      strip.show();            // Send the updated pixel colors to the hardware.
      lcd.clear();
      lcd.setCursor(5, 1);
      lcd.print("Thank You");
      delay(1000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter The Issue ID: ");
      keypadflag = false;
    }
    else if(key == 'B')
    {
      if (CurrententeredNumber.length() > 0) {
        // Remove the last character from the string
        CurrententeredNumber.remove(CurrententeredNumber.length() - 1);
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(8, 1); 
        lcd.print(CurrententeredNumber);
      }
    }
    else
    {
      if(keypadflag == false)
      {
        // Concatenate the pressed key to the entered number
        CurrententeredNumber += key;
        lcd.setCursor(8, 1);
        lcd.print(CurrententeredNumber);
      }
    }
  }
  if(count == 1)
  {
    if(val==0)
    {
      previousMillis = currentMillis;
      val=1;
    }
    if(timerFlag == true)
    {
        timerFlag = false;
        val=0;
    }
    // check if the desired interval has passed since the last blink
    if (currentMillis - previousMillis >= interval) 
    {
      previousMillis = currentMillis;  // save the last time you blinked the LED
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Process Timeout");
      lcd.setCursor(0, 1);
      lcd.print("Try again");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter The Issue ID: ");
      keypadflag = false;
      count=0;
      val=0;
    }
  }
  if (interruptFlag)
  {
    interruptFlag = false;
    timerFlag = true;
    digitalWrite(slaveSelectPin, LOW);
    for (char c : "")
    {
      SPI.transfer(c);
    }
    digitalWrite(slaveSelectPin, HIGH);
    delay(100);
    digitalWrite(slaveSelectPin, LOW);      // Select the slave
    // Receive string data from the slave
    String receivedData = "";
    while (true)
    {
      char c = SPI.transfer(0); // Send dummy byte to receive data
      if (c == '\0')
      {
        break; // Null character indicates the end of the string
      }
      receivedData += c;
    }
    digitalWrite(slaveSelectPin, HIGH); // Deselect the slave
    //Serial.println("Received Data: " + receivedData);
    if(receivedData=="WiFi not connected")
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Not Connected..");
      lcd.setCursor(0, 1);
      lcd.print("Try again");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter The Issue ID: ");
      keypadflag = false;
      count = 0;
    }
    else if(receivedData=="Error on HTTP request")
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Error on HTTP");
      lcd.setCursor(0, 1);
      lcd.print("Request....");
      lcd.setCursor(11, 1);
      lcd.print("Try again");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter The Issue ID: ");
      keypadflag = false;
      count = 0;
    }
    else
    {
      count = 0;
      Json_parse_fun(receivedData);
    }
  }
}
