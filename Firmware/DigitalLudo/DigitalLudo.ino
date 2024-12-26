/*
 * Project Name: Digital Ludo Board
 * Project Brief: Firmware for Digital Ludo Board built around ESP-32S3
 * Author: Jobit Joseph @ https://github.com/jobitjoseph
 * IDE: Arduino IDE 2.3.4
 * Arduino Core: ESP32 Arduino Core V 3.1.0
 * Arduino Board Config: Board: ESP32-S3 Dev Module
 *                       USB CDC On Boot : Enable
 *                       Flash Size : 16MB(128mb)
 *                       Partition Scheme : Custom
 *                       PSRAM : OPI PSRAM
 * Dependencies : Adafruit NeoPixel Library V 1.12.2 @ https://github.com/teknynja/Adafruit_NeoPixel
 *                PNGDec Library V 1.0.3 @https://github.com/bitbank2/PNGdec
 *                TFT_eSPI Library V 2.5.43 @https://github.com/Bodmer/TFT_eSPI
 *                AnimatedGIF Library V 2.1.1 @ https://github.com/bitbank2/AnimatedGIF
 * Hardware : ESP32-S3-WROOM-1-N16R8
 *            Waveshare 1.28inch Round LCD Display Module, 65K RGB 240x240
 *            74HC595 Shiftregister
 *            SK6812MINI-E RGB LED
 *            IP5306 Power Managment IC
 * Copyright B) Jobit Joseph
 * Copyright B) Semicon Media Pvt Ltd
 * Copyright B) Circuitdigest.com
 *
 * This code is licensed under the following conditions:
 *
 * 1. Non-Commercial Use:
 * This program is free software: you can redistribute it and/or modify it
 * for personal or educational purposes under the condition that credit is given
 * to the original author. Attribution is required, and the original author
 * must be credited in any derivative works or distributions.
 *
 * 2. Commercial Use:
 * For any commercial use of this software, you must obtain a separate license
 * from the original author. Contact the author for permissions or licensing
 * options before using this software for commercial purposes.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING
 * FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Jobit Joseph
 * Date: 25 December 2024
 *
 * For commercial use or licensing requests, please contact [jobitjoseph1@gmail.com].
 */


/*PNG decoder library*/
#include <PNGdec.h>          // Include the PNG decoder library
PNG png;                     // PNG decoder instance
#define MAX_IMAGE_WIDTH 240  // Adjust for your images
int16_t xpos = 0;
int16_t ypos = 0;
/*PNG decoder library*/
#include <Adafruit_NeoPixel.h>  //Adafruit neopixel Library
Adafruit_NeoPixel strip(88, 16, NEO_GRB + NEO_KHZ800);
const unsigned long flashInterval = 300;  // Fixed flash interval (300ms)

#include <SPI.h>
#include <TFT_eSPI.h>  //.  TFT_eSPI Library
TFT_eSPI tft = TFT_eSPI();

#include <AnimatedGIF.h>  // AnimatedGIF Library
#include "NotoSansBold36.h"
#include "ImgData.h"
AnimatedGIF gif;
#define GIF_IMAGE0 Ludo_intro
#define GIF_IMAGE00 Confetti
#define GIF_IMAGE11 Congrats
#define GIF_IMAGE1 Dice_1
#define GIF_IMAGE2 Dice_2
#define GIF_IMAGE3 Dice_3
#define GIF_IMAGE4 Dice_4
#define GIF_IMAGE5 Dice_5
#define GIF_IMAGE6 Dice_6
#define AA_FONT_LARGE NotoSansBold36
const uint8_t *gifImages[] = { GIF_IMAGE1, GIF_IMAGE2, GIF_IMAGE3, GIF_IMAGE4, GIF_IMAGE5, GIF_IMAGE6 };

const size_t gifSizes[] = {
	sizeof(GIF_IMAGE1),
	sizeof(GIF_IMAGE2),
	sizeof(GIF_IMAGE3),
	sizeof(GIF_IMAGE4),
	sizeof(GIF_IMAGE5),
	sizeof(GIF_IMAGE6)
};

// Define shift register pins
#define DATA_PIN 13   // SER (Serial Data Input)
#define LATCH_PIN 14  // RCLK (Register Clock)
#define CLOCK_PIN 15  // SRCLK (Shift Register Clock)


// GPIO Management
const int adcPins[7] = { 1, 2, 3, 4, 5, 6, 7 };  // GPIO1 to GPIO7
const int SPIPins[6] = { 8, 9, 10, 11, 12, 21 };

//RGB LED Address Mapping
int ledMapping[16][6] = {
	{ 9, 8, 7, 6, 5, 4 },
	{ 10, 11, 12, 13, 14, 15 },
	{ 21, 20, 19, 18, 17, 16 },
	{ 31, 30, 29, 28, 23, 22 },
	{ 32, 33, 34, 35, 36, 37 },
	{ 43, 42, 41, 40, 39, 38 },
	{ 53, 52, 51, 50, 45, 44 },
	{ 54, 55, 56, 57, 58, 59 },
	{ 65, 64, 63, 62, 61, 60 },
	{ 75, 74, 73, 72, 67, 66 },
	{ 76, 77, 78, 79, 80, 81 },
	{ 87, 86, 85, 84, 83, 82 },
	{ 0, 1, 2, 3, -1, -1 },
	{ 24, 25, 26, 27, -1, -1 },
	{ 46, 47, 48, 49, -1, -1 },
	{ 68, 69, 70, 71, -1, -1 },
};


// Touch detection threshold (adjust based on calibration)
int TOUCH_THRESHOLD = 20;  // Example value; calibrate for your setup
// Debounce settings
#define DEBOUNCE_COUNT 3
unsigned long lastdbug = 0;
// Arrays to hold touch states and debounce counters
uint8_t touchState[16][7] = { 0 };
uint8_t debounceCounter[16][7] = { 0 };
uint8_t stableState[16][7] = { 0 };


//Global Variables for Game Logic
int currentPlayer = 0;
int mode = 0;
int TotalWins = 0;  //To store number of player with all 4 tokens in home
int rollsInTurn = 0;
int diceRoll;            // Count of rolls in the current turn
bool extraRoll = false;  // Track if player gets a bonus roll
int highestRoll = 0;
int startingPlayer = -1;
int PlayerSetupCount = 0;

String Player[] = { "Blue", "Yellow", "Green", "Red" };
int rolls[4] = { 0 };
int HighRoller[] = { 0, 0, 0, 0 };
int PlayerPlace[4] = { 0, 0, 0, 0 };   // Player position for Blue, Yellow, Green, Red
int tokensInHome[4] = { 0, 0, 0, 0 };  // Tokens in home for Blue, Yellow, Green, Red
int tokensInMove[4] = { 0, 0, 0, 0 };
int tokensInStart[4] = { 4, 4, 4, 4 };  // Tokens in start for Blue, Yellow, Green, Red
//int playerPositions[4][4] = {{5,5,3,4},{44,44,3,4},{32,32,3,4},{19,19,3,4}}; // Token positions for each player (relative to their path)
int playerPositions[4][4] = { { 1, 2, 3, 4 }, { 1, 2, 3, 4 }, { 1, 2, 3, 4 }, { 1, 2, 3, 4 } };  // Token positions for each player (relative to their path)
// Paths for each player
const int bluePath[] = { -1, 0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 22, 23, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 44, 45, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 66, 67, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87 };
const int yellowPath[] = { -1, 24, 25, 26, 27, 23, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 44, 45, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 66, 67, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 };
const int greenPath[] = { -1, 46, 47, 48, 49, 45, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 66, 67, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 22, 23, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43 };
const int redPath[] = { -1, 68, 69, 70, 71, 67, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 22, 23, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 44, 45, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65 };
const int *playerPaths[] = { bluePath, yellowPath, greenPath, redPath };
const int pathLengths[] = { sizeof(bluePath) / sizeof(int), sizeof(yellowPath) / sizeof(int), sizeof(greenPath) / sizeof(int), sizeof(redPath) / sizeof(int) };

// Player colors
#define COLOR_BLUE strip.Color(0, 0, 255)
#define COLOR_YELLOW strip.Color(255, 255, 0)
#define COLOR_GREEN strip.Color(0, 255, 0)
#define COLOR_RED strip.Color(255, 0, 0)
const uint32_t playerColors[] = { COLOR_BLUE, COLOR_YELLOW, COLOR_GREEN, COLOR_RED };

// Safe squares
const int safeSquares[] = { 5, 13, 23, 35, 45, 57, 67, 79 };
#define SAFE_SQUARES_COUNT (sizeof(safeSquares) / sizeof(int))

struct LEDState {
	int playerColors[4];       // Colors of overlapping players
	int count;                 // Number of overlapping players
	unsigned long lastUpdate;  // Timestamp of the last update
	int currentIndex;          // Current color index for flashing
	bool state;                // ON/OFF state for flashing
};
LEDState ledStates[150];  // Array to track LED states (adjust size as needed)


// Function to initialize shift register pins
void setupShiftRegisters() {
	pinMode(DATA_PIN, OUTPUT);
	pinMode(LATCH_PIN, OUTPUT);
	pinMode(CLOCK_PIN, OUTPUT);
	// Initialize shift registers to all zeros
	shiftOutData(0);
}

// Function to initialize ADC input pins
void setupADC() {
	for (int i = 0; i < 7; i++) {
		pinMode(adcPins[i], OUTPUT);
	}
}

// Function to scan the entire matrix
void scanMatrix() {
	for (int row = 0; row < 16; row++) {
		activateRow(row);
		delayMicroseconds(50);  // Small delay to allow signals to stabilize
		readColumns(row);
	}
}

// Function to activate a specific row using shift registers
void activateRow(int row) {
	uint16_t data = 1 << row;  // Create a bitmask to activate the current row
	shiftOutData(data);
}

// Function to shift out data to the shift registers
void shiftOutData(uint16_t data) {
	digitalWrite(LATCH_PIN, LOW);
	for (int i = 15; i >= 0; i--) {
		digitalWrite(CLOCK_PIN, LOW);
		digitalWrite(DATA_PIN, (data & (1 << i)) ? HIGH : LOW);
		digitalWrite(CLOCK_PIN, HIGH);
	}
	digitalWrite(LATCH_PIN, HIGH);
}

// Function to read the columns (ADC inputs) for the active row
void readColumns(int row) {
	for (int col = 0; col < 7; col++) {
		pinMode(adcPins[col], INPUT);
		int adcValue = analogRead(adcPins[col]);
		pinMode(adcPins[col], OUTPUT);
		digitalWrite(adcPins[col], 0);
		touchState[row][col] = (adcValue > TOUCH_THRESHOLD) ? 1 : 0;
	}
}

// Function to handle debounce logic
void debounceKeys() {
	for (int row = 0; row < 16; row++) {
		for (int col = 0; col < 7; col++) {
			if (touchState[row][col] == stableState[row][col]) {
				debounceCounter[row][col] = 0;  // Reset counter if state is stable
			} else {
				debounceCounter[row][col]++;
				if (debounceCounter[row][col] >= DEBOUNCE_COUNT) {
					stableState[row][col] = touchState[row][col];
					debounceCounter[row][col] = 0;
					handleKeyEvent(row, col, stableState[row][col]);
				}
			}
		}
	}
}

// Function to handle key events (presses and releases)
void handleKeyEvent(int row, int col, uint8_t state) {
	if (state == 1) {
		return;
	} else {
		// Key released
		if (row < 12) {
			Serial.print("Touch at  : ");
			Serial.println(ledMapping[row][col]);
			handleTokenMove(ledMapping[row][col]);
		} else {
			Serial.print("Player  : ");
			Serial.println(row - 12);
			handleTokenMove(100 + (row - 12));
		}
	}
}

// Optional: Function to calibrate touch threshold
void calibrateTouchThreshold() {
	int maxValue = 0;
	for (int col = 0; col < 7; col++) {
		int adcValue = analogRead(adcPins[col]);
		if (adcValue > maxValue) {
			maxValue = adcValue;
		}
	}
	// Set TOUCH_THRESHOLD slightly above the maximum observed value
	TOUCH_THRESHOLD = maxValue + 100;  // Adjust the offset as needed
}

void pngDraw(PNGDRAW *pDraw) {
	uint16_t lineBuffer[MAX_IMAGE_WIDTH];
	png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
	tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void DisplayPNG(const uint8_t *pngArray, size_t arraySize) {
	int16_t rc = png.openFLASH((uint8_t *)pngArray, arraySize, pngDraw);
	if (rc == PNG_SUCCESS) {
		Serial.println("Successfully opened PNG file");
		Serial.printf("Image specs: (%d x %d), %d bpp, pixel type: %d\n",
		              png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());

		tft.startWrite();
		uint32_t dt = millis();
		rc = png.decode(NULL, 0);
		Serial.print(millis() - dt);
		Serial.println("ms");
		tft.endWrite();
		// png.close(); // Not needed for memory-to-memory decode
	} else {
		Serial.println("Failed to open PNG file");
	}
}

void playGIF(const uint8_t *gifImage, size_t gifSize) {
	//size_t gifSize = sizeof(gifImage); // Automatically calculate size
	if (gif.open((uint8_t *)gifImage, gifSize, GIFDraw)) {
		Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
		tft.startWrite();
		while (gif.playFrame(true, NULL)) {
			yield();
		}
		gif.close();
		tft.endWrite();
	} else {
		Serial.println("Failed to open GIF!");
	}
}
void drawLudoHomeArea(int tokensBlue, int tokensYellow, int tokensGreen, int tokensRed, int centerColorFlag) {
	// Colors for tokens and placeholders
	uint16_t blueColor = 0x0294;
	uint16_t yellowColor = 0xd5a0;
	uint16_t greenColor = 0x4420;
	uint16_t redColor = 0xb021;
	uint16_t centerCircleColor;

	// Set center circle color based on the flag
	switch (centerColorFlag) {
	case 0:
		centerCircleColor = blueColor;
		break;  // Light Blue
	case 1:
		centerCircleColor = yellowColor;
		break;
	case 2:
		centerCircleColor = greenColor;
		break;
	case 3:
		centerCircleColor = redColor;
		break;
	default:
		centerCircleColor = blueColor;
		break;  // Default to Light Blue
	}

	// Screen dimensions (assuming circular display of 240x240 pixels)
	int screenWidth = 240;
	int screenHeight = 240;
	int centerX = screenWidth / 2;
	int centerY = screenHeight / 2;
	int radius = screenWidth / 2;

	// Clear screen
	tft.fillScreen(TFT_BLACK);

	// Draw quadrant backgrounds
	tft.fillTriangle(centerX, centerY, 0, 0, screenWidth, 0, yellowColor);                      // Top-right (Yellow)
	tft.fillTriangle(centerX, centerY, screenWidth, 0, screenWidth, screenHeight, greenColor);  // Bottom-right (Green)
	tft.fillTriangle(centerX, centerY, screenWidth, screenHeight, 0, screenHeight, redColor);   // Bottom-left (Red)
	tft.fillTriangle(centerX, centerY, 0, screenHeight, 0, 0, blueColor);                       // Top-left (Blue)

	// Draw center circle
	int centerRadius = 49;                                          // Circle radius is 50 pixels diameter
	tft.fillCircle(centerX, centerY, centerRadius + 2, TFT_BLACK);  // Outer border
	tft.fillCircle(centerX, centerY, centerRadius, centerCircleColor);

	// Function to draw tokens and placeholders at left, right, top, and bottom centers
	auto drawTokens = [&](int cx, int cy, uint16_t color, int tokenCount) {
		int tokenRadius = 10;
		int spacing = 20;   // Spacing between tokens
		int maxTokens = 4;  // Max tokens per quadrant

		// Calculate positions for tokens (left, right, top, bottom centers of the quadrant)
		int positions[4][2] = {
			{ cx, cy + spacing },  // Left
			{ cx, cy - spacing },  // Right
			{ cx - spacing, cy },  // Top
			{ cx + spacing, cy }   // Bottom
		};

		for (int i = 0; i < maxTokens; i++) {
			int x = positions[i][0];
			int y = positions[i][1];

			if (i < tokenCount) {
				tft.fillCircle(x, y, tokenRadius + 2, TFT_BLACK);  // Outer outline
				tft.fillCircle(x, y, tokenRadius, color);          // Filled token
				tft.fillCircle(x, y, tokenRadius - 4, TFT_BLACK);  // Inner outline
			} else {
				tft.fillCircle(x, y, tokenRadius + 2, TFT_BLACK);  // Outer outline
				tft.fillCircle(x, y, tokenRadius, TFT_WHITE);      // Placeholder
			}
		}
	};

	// Draw tokens for each quadrant
	drawTokens(34, 120, blueColor, tokensBlue);      // Top-left (Blue)
	drawTokens(120, 34, yellowColor, tokensYellow);  // Top-right (Yellow)
	drawTokens(206, 120, greenColor, tokensGreen);   // Bottom-right (Green)
	drawTokens(120, 206, redColor, tokensRed);       // Bottom-left (Red)
}

void displayHighRollers(int left, int top, int right, int bottom, int offset) {
	if (left > 0) {
		tft.drawNumber(left, offset, 120);
	}
	if (top > 0) {
		tft.drawNumber(top, 120, offset);
	}
	if (right > 0) {
		tft.drawNumber(right, 240 - offset, 120);
	}
	if (bottom > 0) {
		tft.drawNumber(bottom, 120, 240 - offset);
	}
}

// Update LEDs
void updateLEDs() {
	const int numPlayers = 4;               // Total players
	const int numTokens = 4;                // Tokens per player
	const int numLEDs = strip.numPixels();  // Total LEDs

	// Reset the LED states
	for (int i = 0; i < numLEDs; i++) {
		ledStates[i].count = 0;
	}

	// Populate LED states
	for (int player = 0; player < numPlayers; player++) {
		for (int token = 0; token < numTokens; token++) {
			int pos = playerPositions[player][token];
			if (pos > 0 && pos < pathLengths[player]) {
				int ledIndex = playerPaths[player][pos];
				if (ledStates[ledIndex].count < numPlayers) {
					ledStates[ledIndex].playerColors[ledStates[ledIndex].count] = playerColors[player];
					ledStates[ledIndex].count++;
				}
			}
		}
	}

	// Update LEDs based on their states
	unsigned long currentMillis = millis();
	for (int i = 0; i < numLEDs; i++) {
		if (ledStates[i].count == 0) {
			// No tokens on this LED, turn it off
			strip.setPixelColor(i, 0);
		} else if (ledStates[i].count == 1) {
			// Single token, set to static color
			strip.setPixelColor(i, ledStates[i].playerColors[0]);
		} else {
			// Multiple tokens, handle flashing
			if (currentMillis - ledStates[i].lastUpdate >= flashInterval) {
				ledStates[i].lastUpdate = currentMillis;
				if (ledStates[i].state) {
					// Turn the LED off
					strip.setPixelColor(i, 0);
				} else {
					// Set the LED to the current color
					strip.setPixelColor(i, ledStates[i].playerColors[ledStates[i].currentIndex]);
					// Move to the next color
					ledStates[i].currentIndex = (ledStates[i].currentIndex + 1) % ledStates[i].count;
				}
				ledStates[i].state = !ledStates[i].state;
			}
		}
	}

	strip.show();
}

void setupPlayers() {
	DisplayPNG((uint8_t *)StartMessage, sizeof(StartMessage));
	Serial.println("Any Player touch start area!");
	while (mode == 5) {
		scanMatrix();
		debounceKeys();
		updateLEDs();
		if (millis() - lastdbug > 1000) {
			Serial.print("Mode: ");
			Serial.print(mode);
			Serial.print(" Player with High Roll: ");
			Serial.println(startingPlayer + 1);
			lastdbug = millis();
		}
	}
	currentPlayer = startingPlayer;
	DisplayPNG((uint8_t *)FirstPlayer, sizeof(FirstPlayer));
	tft.drawString(Player[currentPlayer], 120, 91);
	delay(2000);
}

// Manage player turns
void nextPlayer() {
	if (rollsInTurn > 0) {
		rollsInTurn = 0;
	}
	do {
		currentPlayer = (currentPlayer + 1) % 4;   // Move to the next player
	} while (tokensInHome[currentPlayer] == 4);  // Continue until a valid player is found
	mode = 0;
}

// Take token out of start area
void TakeTokenOut() {
	for (int i = 0; i < 4; i++) {
		if (playerPositions[currentPlayer][i] < 5) {
			playerPositions[currentPlayer][i] = 5;
			detectCollision(currentPlayer, playerPositions[currentPlayer][i]);
			tokensInStart[currentPlayer]--;
			tokensInMove[currentPlayer]++;
			return;
		}
	}
}

//Detect collision
void detectCollision(int currentPlayer, int movedTokenIndex) {
	// Get the new position of the moved token
	int newPosition = playerPaths[currentPlayer][movedTokenIndex];

	// Check if the new position is a safe square
	for (int i = 0; i < SAFE_SQUARES_COUNT; i++) {
		if (newPosition == safeSquares[i]) {
			// Safe square; do nothing
			return;
		}
	}

	// Check for collisions with other players
	for (int otherPlayer = 0; otherPlayer < 4; otherPlayer++) {
		if (otherPlayer == currentPlayer) {
			continue;  // Skip the current player
		}

		for (int otherToken = 0; otherToken < 4; otherToken++) {
			if (playerPaths[otherPlayer][otherToken] == newPosition) {
				// Collision detected; move the other player's token to a vacant start point
				for (int i = 1; i < 5; i++) {
					if (playerPositions[otherPlayer][0] != i && playerPositions[otherPlayer][1] != i && playerPositions[otherPlayer][2] != i && playerPositions[otherPlayer][3] != i) {
						playerPositions[otherPlayer][otherToken] = i;  // Move to start
						tokensInStart[otherPlayer]++;
						tokensInMove[otherPlayer]--;
						Serial.print("Collision! Token from player ");
						Serial.print(otherPlayer + 1);
						Serial.print(" moved back to start position.");
						Serial.println();
						break;
					}
					return;
				}
			}
		}
	}
}

//Check if there is any valid move for the player
void checkValidMove() {
	Serial.println("Checking for valid moves!");
	int validMoves = 0;
	int selectedToken = -1;

	// Check all tokens of the current player
	for (int token = 0; token < 4; token++) {
		int pos1 = playerPositions[currentPlayer][token];
		int pos = playerPaths[currentPlayer][pos1];
		// Check if the token can make a valid move
		bool isValid = true;
		// Check if the new position is any of the restricted positions
		for (int i = 0; i <= 4; i++) {
			if (pos == playerPaths[currentPlayer][i]) {
				isValid = false;
				break;
			}
		}
		// Ensure `playerPaths[currentPlayer][5]` is always valid
		if (pos == playerPaths[currentPlayer][5]) {
			isValid = true;
		}
		// Check the boundary condition
		if (pos1 + diceRoll > (pathLengths[currentPlayer] + 1)) {
			isValid = false;
		}
		// If valid, count the move and store the token index
		if (isValid) {
			validMoves++;
			selectedToken = token;
		}
		// Print debugging information
		Serial.print(" : Valid: ");
		Serial.println(isValid);
	}

	// If no valid moves are found
	if (validMoves == 0) {
		Serial.println("No valid moves are found!");
		DisplayPNG((uint8_t *)NoValidMoveMessage, sizeof(NoValidMoveMessage));  // Replace with your actual PNG for the message
		delay(2000);                                                            // Wait to allow the player to see the message
		if (diceRoll == 6 && rollsInTurn < 3) {
			mode = 0;
			return;
		}
		nextPlayer();  // Move to the next player
		return;
	}

	// If only one valid move is found
	if (validMoves == 1) {
		Serial.println("Single valid move found! Auto move!");
		playerPositions[currentPlayer][selectedToken] += diceRoll;
		detectCollision(currentPlayer, playerPositions[currentPlayer][selectedToken]);

		// Update LEDs and move to the next player
		// Wait to allow the player to see the message
		if (diceRoll == 6 && rollsInTurn < 3) {
			mode = 0;
			return;
		} else {
			nextPlayer();
			return;
		}
	}

	// If multiple valid moves are found
	Serial.println("Multiple valid moves are found! Select your token.");
	mode = 1;  // Change mode to 1 to allow token selection
}

// Token movement logic
void handleTokenMove(int TouchNumb) {
	int tmap = TouchNumb;
	if (mode == 5 && TouchNumb > 99) {
		tmap -= 100;
		if (rolls[tmap] == 1) {
			return;
		}
		rolls[tmap] = 1;
		int temproll = random(1, 7);
		playGIF(gifImages[temproll - 1], gifSizes[temproll - 1]);
		HighRoller[tmap] = temproll;
		DisplayPNG((uint8_t *)MessageBg, sizeof(MessageBg));
		displayHighRollers(HighRoller[0], HighRoller[1], HighRoller[2], HighRoller[3], 60);
		PlayerSetupCount++;
		if (temproll > highestRoll) {
			highestRoll = temproll;
			startingPlayer = tmap;
		}

		if (PlayerSetupCount == 4) {
			delay(1000);
			mode = 0;
		}
	} else {

		if (mode == 0) {
			if (tmap < 100) {
				return;
			}
			if (tmap >= 100 && currentPlayer == (tmap - 100)) {  // Touch detected in the starting area
				diceRoll = random(1, 7);
				playGIF(gifImages[diceRoll - 1], gifSizes[diceRoll - 1]);
				Serial.print("Dice roll: ");
				Serial.println(diceRoll);
				if (diceRoll == 6 || diceRoll == 1) {

					if (tokensInStart[currentPlayer] == 0) {
						//Serial.println("Select your tokenxx");
						checkValidMove();
					} else {
						TakeTokenOut();
						Serial.println("Token is out");
					}
					if (rollsInTurn == 3) {
						nextPlayer();
					}
					rollsInTurn++;
				} else {
					//rollsInTurn = 0;
					if (tokensInMove[currentPlayer] > 0) {
						Serial.println("Select your token");
						checkValidMove();
					} else {
						Serial.println("better luck next time");
						nextPlayer();
					}
				}
			}
			drawLudoHomeArea(tokensInHome[0], tokensInHome[1], tokensInHome[2], tokensInHome[3], currentPlayer);
			return;
		}
		if (mode == 1) {

			for (int i = 0; i < 4; i++) {
				int relativePos = playerPaths[currentPlayer][playerPositions[currentPlayer][i]];
				Serial.print("Token ");
				Serial.print(i);
				Serial.print(": Current Position ");
				Serial.print(relativePos);
				Serial.print(", Touch Map ");
				Serial.print(tmap);
				Serial.print(", Dice Roll ");
				Serial.println(diceRoll);
				if (relativePos == tmap && playerPositions[currentPlayer][i] + diceRoll < 61) {
					playerPositions[currentPlayer][i] += diceRoll;
					Serial.println("token moved");
					if (playerPositions[currentPlayer][i] == 60) {

						tft.fillScreen(TFT_BLACK);
						playGIF(Confetti, sizeof(Confetti));
						tft.fillScreen(TFT_BLACK);
						playGIF(Confetti, sizeof(Confetti));
						tft.fillScreen(TFT_BLACK);
						playGIF(Confetti, sizeof(Confetti));
						tokensInHome[currentPlayer]++;
						tokensInMove[currentPlayer]--;
					} else {
						detectCollision(currentPlayer, playerPositions[currentPlayer][i]);
					}
					if (tokensInHome[currentPlayer] == 4) {
						TotalWins += 1;
					}
					// Wait to allow the player to see the message
					if (diceRoll == 6) {
						if (rollsInTurn == 3) {
							nextPlayer();
							drawLudoHomeArea(tokensInHome[0], tokensInHome[1], tokensInHome[2], tokensInHome[3], currentPlayer);
							mode = 0;
							return;
						} else {
							drawLudoHomeArea(tokensInHome[0], tokensInHome[1], tokensInHome[2], tokensInHome[3], currentPlayer);
							mode = 0;
							return;
						}

					} else {
						nextPlayer();
						drawLudoHomeArea(tokensInHome[0], tokensInHome[1], tokensInHome[2], tokensInHome[3], currentPlayer);
						return;
					}
				}
			}
			return;
		}
		if (mode == 2) {
			return;
		}
	}
}


void setup() {
	Serial.begin(115200);

	strip.begin();            // Initialize NeoPixel strip object
	strip.show();             // Turn OFF all pixels ASAP
	strip.setBrightness(50);  // Set brightness
	strip.clear();
	strip.show();
	setupShiftRegisters();

	setupADC();

	tft.init();
	tft.setRotation(0);
	tft.fillScreen(TFT_BLACK);
	tft.loadFont(AA_FONT_LARGE);
	tft.setTextColor(TFT_WHITE);  // Set the text color to white
	tft.setTextDatum(MC_DATUM);
	tft.setTextSize(2);  // Set the text size

	mode = 5;

	gif.begin(BIG_ENDIAN_PIXELS);
	playGIF(GIF_IMAGE0, sizeof(GIF_IMAGE0));

	setupPlayers();

	drawLudoHomeArea(tokensInHome[0], tokensInHome[1], tokensInHome[2], tokensInHome[3], currentPlayer);
	//calibrateTouchThreshold();
}

void loop() {
	int count = 0;
	int winner = -1;  // Variable to track the winning player

	for (int i = 0; i < 4; i++) {
		if (tokensInHome[i] == 4) {
			count++;
			winner = i;  // Set the winner
		}
	}
	// If any player has all tokens in home, play the Congrats GIF
	if (count > 0) {

		while (true) {                // Loop indefinitely to show the Congrats GIF
			tft.fillScreen(TFT_BLACK);  // Clear the screen
			playGIF(Confetti, sizeof(Confetti));
			playGIF(Congrats, sizeof(Congrats));
			tft.drawString("WINNER", 120, 40);
			tft.drawString(Player[winner], 120, 200);
			delay(2000);
		}
	}

	scanMatrix();
	debounceKeys();
	updateLEDs();
	delay(10);  // Adjust the delay as needed for your application
}


