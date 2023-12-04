#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include "deck.h"
#include "switches.h"

Card player_hand[10];
Card dealer_hand[10];
int player_hand_size = 0;//# of cards
int dealer_hand_size = 0;
int wallet = 150;
int player_bet = 0;
int player_hand_value = 0;//total value
int dealer_hand_value = 0;
bool betting = true; // boolean
bool second_hand = false; // boolean
bool stats = false;
bool dealers_turn = false;
bool fin = false;

void initLCD();
void sendCommand(unsigned char command);
void sendData(unsigned char data);
void displayString(char *str);
void displayInt(int value);
int isButtonPressed(int button);
void initGame();
void dealInitialCards();
void playerTurn();
void dealerTurn();
int calculateHandValue(int hand[], int size);
void displayHands();

int main(void) {

  WDTCTL = WDTPW + WDTHOLD; // Stop the watchdog timer
  initLCD();
  initButtons();
  initGame();

  while (1) {
    displayString("Place your bet");
    while (!isButtonPressed(BUTTON4)) {
      if (isButtonPressed(BUTTON1)) {
	player_bet++;
	displayInt(player_bet);
	__delay_cycles(100000);
      } else if (isButtonPressed(BUTTON2) && player_bet > 0) {
	player_bet--;
	displayInt(player_bet);
	__delay_cycles(100000);
      }
    }
    dealInitialCards();
    displayHands();
    playerTurn();
    dealerTurn();

    // Check winner and update display accordingly
    // ...
    // Allow the player to start a new round
    // ...
  }
}

void sendCommand(unsigned char command) {
  if(betting){ // (inc|dec|bet|stats)
    switch(command){
    case 1://increase bet
      bet += 10;
      break;
    case 2://decrease bet
      if(bet <= 0){
	// send error message
	break;
      }
      bet -= 10;
      break;
    case 3:// toggle statistics
      if(stats){stats = false;}
      else{stats = true;}
      break;
    case 4:// start round
      betting = false;
      dealInitialCards();
      break;
  }
  else if(playing){       // (hit|stand|double|split)
    switch(command){
    case 1:// request an additional card
      player_hand[player_hand_size] = drawCard();
      updateDisplay();
      break;
    case 2:// proclaim to be done
      dealerTurn();
      break;
    case 3:// double the bet & recieve a final card
      bet *= 2;
      player_hand[player_hand_size] = drawCard();
      updateDisplay();
      dealerTurn();
      break;
    case 4:
      /* IFF the initial pair of cards given, are the same value 
      (i.e. two eights | two kings, etc) they can be split and play
      each one like two separate hands. The original bet will go 
      with one hand and the player is required to place an equal bet
      on the second hand. Each hand has to be played one at a time. 
      Should the player lack the funds necessary to bet on the second
      hand then they'll be unable to 'split' their hand.*/
      if(checkWallet() && checkTAK()){
	splitHand();
      }
      else{
	// send error message
      }
      break;    
  }
  else if(fin){ // (YES           NO)
    if(command == 1){ menu();}
    if(command == 4){ mMenu();
  }
}

void splitHand() {
  second_hand = true;
  
}

bool checkWallet(){
  if(wallet < bet){return false;}
  return true;
}

char checkTAK(){ // verify Two of A Kind (TAK)
  char val1 = player_hand[0];
  char val2 = player_hand[1];
  if(val1 == val2){return 1;}
  return 0;
}
void displayString(char *str) {

  // Display string on LCD
  // ...
}

void displayCards(){
  char val;
  char suit;
  for(int i = 0; i < player_hand_size; i++){
    val = player_hand[i].value;
    suit = player_hand[i].suit;    
    drawHand(20+(10 * i),80,50,60, COLOR_WHITE, suit, val);
  }
  drawDealersHand();
  displayValues();
  
}
void displayValues(){
  int total = calculateHandValue(player_hand, player_hand_size);
  int dlrCrd = (int)dealer_hand[0].value;
  char handVal[12];
  char money = (char)wallet;
  char wString[15] = {'\0'};
  char dString[15] = {'\0'};
  sprintf(wString, "Wallet: %d", money);
  drawString5x7(5, 5, wString, COLOR_BLACK, COLOR_WHITE);
  sprintf(dString, "Dealer: %d", dlrCrd);
  drawString5x7(5, 50, dString, COLOR_BLACK, COLOR_WHITE);
  sprintf(handVal, "Hand: %d", total);
  drawString5x7(55, 70, totVal, COLOR_BLACK, COLOR_WHITE);
}

void drawDealersHand(){
  char val;
  char suit;
  for(int i = 0; i < dealer_hand_size; i++){
    if(i == 0 || dealers_turn){ // only show the first card face up, unless it's dealers turn
      val = dealer_hand[i].value;
      suit = dealer_hand[i].suit;
      drawHand(50, -40, 50, 60, COLOR_WHITE, suit, val);
    }
    else{
      // draw the cards'backside
      drawBOC(50+(10 * i), -40, 50, 60);
    }
}

void dealInitialCards() {
  for(int i=0; i < 2; i++){
    player_hand[i] = drawCard();
    dealer_hand[i] = drawCard();
    dealer_hand_size++;
    player_hand_size++;
  }
  player_hand_value = calculateHandValue(player_hand, player_hand_size);
  dealer_hand_value = calculateHandValue(dealer_hand, dealer_hand_size);
  if(dealer_hand_value == 21 && player_hand_value == 21){ declareDraw();}
  else if(dealer_hand_value == 21 && player_hand_value != 21){ delcareLoss();}
  else{ updateDisplay();}
}

int calculateHandValue(struct Card hand[], int size){
  int totalValue = 0;
  int numAces = 0;
  for (int i = 0; i < size; i++) {
    char cardValue = hand[i].value;
    if (cardValue >= '2' && cardValue <= '9') {
      totalValue += (cardValue - '0');  // Convert character to integer
    } else if (cardValue == 'A') {
      numAces++;
    } else {
      // '10', 'J', 'Q', 'K' are all worth 10 points
      totalValue += 10;
    }
  }
  // Add Aces with considering the optimal value (1 or 11)
  for (int i = 0; i < numAces; i++) {
    if (totalValue + 11 <= 21) {
      totalValue += 11;
    } else {
      totalValue += 1;
    }
  }
  return totalValue;
}
void updateDisplay() {
  clearScreen(COLOR_GREEN);
  displayCards();
  if(stats){ displayStatistics();}
}

void dealerTurn() {
  dealers_turn = true;
  while(dealer_hand_value < 16){
    dealer_hand[dealer_hand_size] = drawCard();
    dealer_hand_value = calculateHandValue(dealer_hand, dealer_hand_size);
    displayCards();
  }
  fin = true;
  if(dealer_hand_value > 21 || player_hand_value > dealer_hand_value){ declareWin();}
  else{ declareLoss();}
}

void declareWin(){
  char win[10] = {'\0'};
  drawString5x7(20, 80, "YOU WIN!", COLOR_GREEN, COLOR_BLACK);
  sprintf(win, "+ %d", bet);
  //drawString5x7(55, 70, totVal, COLOR_BLACK, COLOR_WHITE);
  drawString5x7(20, 90, win, COLOR_BLACK, COLOR_WHITE);
  wallet += (bet*2);
  bet = 0;
  delay_cycles(10000000);
  playAgain();
}
void declareLoss(){
   drawString5x7(20, 80, "YOU LOSE!", COLOR_GREEN, COLOR_BLACK);
   delay_cycles(10000000);
   playAgain();
}
void playAgain(){
  clearScreen(COLOR_GREEN);
  drawString5x7(20, 80, "PLAY AGAIN?", COLOR_BLACK, COLOR_GREEN);
  drawString5x7(1, 150, "YES             NO", COLOR_BLACK, COLOR_GREEN);	
}
