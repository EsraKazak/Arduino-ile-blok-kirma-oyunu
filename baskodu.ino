#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2lib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define POT_PIN A0

const int downButtonPin = 11;
const int upButtonPin = 12;
const int selectButtonPin = 13;
const int numLives = 3;
const int brickWidth = 16;
const int brickHeight = 8;
const int numRows = 3;
const int numCols = 8;

const int segmentPins[] = {4, 5, 6, 7, 8, 9, 10};
int score = 0;

int cursorPosition = 0;
bool selectionMade = false;
bool gameStarted = false;
int lives = numLives;

float ballX;
float ballY;
float ballDX =3;
float ballDY =3;

bool bricks[numRows][numCols];

const int ledPins[] = {0,1,2};
const int lightSensorPin = A1;

float objectX;
float objectY;
float objectSpeed = 1;
bool objectActive = false;

int level = 1; 
bool newLevel = false; // Yeni seviyeye geçiş için


void setup() {
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(downButtonPin, INPUT_PULLUP);
  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(selectButtonPin, INPUT_PULLUP);

  for (int i = 0; i < numLives; ++i) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH);
  }

  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }


  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT - 10; 

  randomSeed(analogRead(0));
  for (int i = 0; i < numRows; ++i) {
    for (int j = 0; j < numCols; ++j) {
      bricks[i][j] = random(2);
    }
  }
}

void loop() {
  int potValue = analogRead(POT_PIN);
  int paddleX = map(potValue, 0, 1023, 0, SCREEN_WIDTH - 30);

  showScoreOnSegment(score);
  int lightLevel = analogRead(lightSensorPin);

  if (lightLevel > 2) {
    oled.setTextColor(SSD1306_WHITE);
  } else {
    oled.setTextColor(SSD1306_BLACK);
  }

  if (!selectionMade) {
    displayOptions();

    if (digitalRead(downButtonPin) == LOW) {
      cursorPosition++;
      if (cursorPosition > 1) cursorPosition = 0;
      delay(200); // Debouncing
    }

    if (digitalRead(upButtonPin) == LOW) {
      cursorPosition--;
      if (cursorPosition < 0) cursorPosition = 1;
      delay(200); // Debouncing
    }

    if (digitalRead(selectButtonPin) == LOW) {
      if (cursorPosition == 0) {
        selectionMade = true;
        gameStarted = false;
        ballX = paddleX + 25;
        ballY = SCREEN_HEIGHT - 12;
      } else {
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setCursor(0, 20);
        oled.println("Oyuna ilgi icin");
        oled.setCursor(0, 30);
        oled.println("tesekkurler!");
        oled.display();
        delay(2000);
        selectionMade = false;
      }
      delay(200); // Debouncing
    }
  } else {
    if (!gameStarted) {
      ballX = paddleX + 25;
      ballY = SCREEN_HEIGHT - 12;
      gameStarted = true;
    }

    moveBall();
    checkCollisions(paddleX);
    drawGame(paddleX);
    updateLivesIndicator();

    if (gameOver()) {
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setCursor(0, 20);
      oled.println("Oyun bitti!");
      oled.setCursor(0, 30);
      oled.println("Skor: " + String(score));
      oled.display();
      delay(3000);
      backToMainMenu();
    }
  }
}


void showScoreOnSegment(int score) {
  int digitValues[] = { //katota göre
    B1111110, // 0
    B1111001, // 1
    B1101101, // 2
    B1111001, // 3
    B0110011, // 4
    B1011011, // 5
    B1011111, // 6
    B1110000, // 7
    B1111111, // 8
    B1111011  // 9
  };

  int digit = score % 10;

  digitalWrite(segmentPins[0], bitRead(digitValues[digit], 0));
  digitalWrite(segmentPins[1], bitRead(digitValues[digit], 1));
  digitalWrite(segmentPins[2], bitRead(digitValues[digit], 2));
  digitalWrite(segmentPins[3], bitRead(digitValues[digit], 3));
  digitalWrite(segmentPins[4], bitRead(digitValues[digit], 4));
  digitalWrite(segmentPins[5], bitRead(digitValues[digit], 5));
  digitalWrite(segmentPins[6], bitRead(digitValues[digit], 6));
}

void displayOptions() {
  oled.clearDisplay();
  oled.setTextSize(1);

  if (cursorPosition == 0) {
    oled.setCursor(20, 20);
    oled.println("> Baslat");
  } else {
    oled.setCursor(0, 20);
    oled.println("  Baslat");
  }

  if (cursorPosition == 1) {
    oled.setCursor(20, 40);
    oled.println("> Cikis");
  } else {
    oled.setCursor(0, 40);
    oled.println("  Cikis");
  }

  oled.display();
}


void backToMainMenu() {
  score = 0;
  gameStarted = false;
  lives = numLives;
  resetLivesIndicator();
  selectionMade = false;
}

void moveBall() {
  ballX += ballDX;
  ballY += ballDY;
}

void startNewLevel() {
  level++; 

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Seviye: " + String(level)); 
  oled.setTextSize(2);
  oled.setCursor(0, 20);
  oled.println("Yeni Seviye");
  oled.setCursor(0, 40);
  oled.println("Basliyor...");
  oled.display();

  delay(5000); 

  // Top hızını %20 artırma
  ballDX *= 1.2; // Yatay hızı
  ballDY *= 1.2; // Dikey hızı

  
  resetGame();
  gameStarted = false; 
}





void checkCollisions(int paddleX) {
 
  if (ballX <= 0 || ballX >= SCREEN_WIDTH) {
    ballDX = -ballDX;
  }
  
  if (ballY <= 0) {
    ballDY = -ballDY;
  }
  
  
  if (ballY >= SCREEN_HEIGHT - 10) { 
    if (ballX >= paddleX && ballX <= paddleX + 50) { 
      ballDY = -ballDY; 
    
    } else {
       ballDY = -ballDY;
      loseLife(); 
    }
  }

  
  int brickX = (int)(ballX / brickWidth);
  int brickY = (int)(ballY / brickHeight);
  if (brickX >= 0 && brickX < numCols && brickY >= 0 && brickY < numRows && bricks[brickY][brickX]) {
    ballDY = -ballDY;
    bricks[brickY][brickX] = false;
    score++;
  }

  bool allBricksBroken = true;
  for (int i = 0; i < numRows; ++i) {
    for (int j = 0; j < numCols; ++j) {
      if (bricks[i][j]) {
        allBricksBroken = false;
        break;
      }
    }
  }

  if (allBricksBroken) {
    startNewLevel(); 
  }
}


void drawGame(int paddleX) {
  oled.clearDisplay();
  oled.setTextSize(1);

 
  oled.setCursor(0, 0);
  oled.println("Seviye: " + String(level)); 


  oled.fillRect(paddleX, 60, 30, 4, SSD1306_WHITE);
  oled.drawCircle(ballX, ballY, 3, SSD1306_WHITE);


  for (int i = 0; i < numRows; ++i) {
    for (int j = 0; j < numCols; ++j) {
      if (bricks[i][j]) {
        oled.drawRect(j * brickWidth, i * brickHeight, brickWidth, brickHeight, SSD1306_WHITE);
      }
    }
  }

  oled.display(); 
}


void resetGame() {

  for (int i = 0; i < numRows; ++i) {
    for (int j = 0; j < numCols; ++j) {
      bricks[i][j] = random(2);
    }
  }

  
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  gameStarted = false;
 
  resetLivesIndicator();
}

void resetScoreOnSegment() {
  digitalWrite(segmentPins[0], LOW);
  digitalWrite(segmentPins[1], LOW);
  digitalWrite(segmentPins[2], LOW);
  digitalWrite(segmentPins[3], LOW);
  digitalWrite(segmentPins[4], LOW);
  digitalWrite(segmentPins[5], LOW);
  digitalWrite(segmentPins[6], LOW);
}

bool gameOver() {
  if (lives <= 0) {
    resetScoreOnSegment();
    return true;
  }
  return false;
}

void updateLivesIndicator() {
  for (int i = 0; i < numLives; ++i) {
    if (i < lives) {
      digitalWrite(ledPins[i], HIGH);
    } else {
      digitalWrite(ledPins[i], LOW);
    }
  }
}

void resetLivesIndicator() {
  for (int i = 0; i < numLives; ++i) {
    digitalWrite(ledPins[i], HIGH);
  }
}

void loseLife() {
  lives--;
  updateLivesIndicator();
}

void moveObject() {
  if (objectActive) {
    objectY += objectSpeed;
   
    if (objectY >= SCREEN_HEIGHT) {
      objectActive = false;
    }
  }
}

void drawObject() {
  if (objectActive) {
    oled.fillCircle(objectX, objectY, 3, SSD1306_WHITE);
  }
}