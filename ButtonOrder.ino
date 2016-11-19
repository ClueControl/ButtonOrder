/*

  Button Order - Requires players to push buttons in a specific order to solve the puzzle
				
  2016 Shawn Yates
  Want to automate this puzzle?  Visit our website to learn how:
  www.cluecontrol.com
  
  written and tested with Arduino 1.6.9
 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
  
  written and tested with Arduino 1.6.9
 
   -------------------
*/

/*
// pin assignments
 * Signal     Pin                
 *            Arduino Uno     
 * ------------------------
 * 0   Serial RX
 * 1   Serial TX
 * 2   SW1 - laser game    
 * 3   SW2 - laser game
 * 4   SW3 - laser game
 * 5   SW4 - laser game    
 * 6   SW5 - mag lock button  
 * 7   SW6 - program button
 
 * 8   SW7 - reset button     
 * 9   RL1 - Maglock relay & LED	     
 * 10	ChipSelect for Ethernet
 * 11   SPI MOSI for Ethernet
 * 12   SPI MISO for Ethernet
 * 13   SPI SCK  for Ethernet
 
 * 14  (A0)     RL2 - laser relay & LED
 * 15  (A1)     Power LED1
 * 16  (A2)		
 * 17  (A3)		
 * 18  (A4/SDA)	
 * 19  (A5/SCK)	
 
 
 The Ethernet Shield SPI bus (10,11,12,13) with 10 as select
 also uses 4 as select for the SD card on the ethernet shield
 high deselects for the chip selects.  Reserve these incase the
 puzzle is ever connected to ClueControl via an Ethernet shield.
 
 General operation:
 
	STARTUP
		If a winning pattern is not stored for the laser game, 
		go to the programming process.  If a winning pattern is stored, 
		go to the reset process.  

	PROGRAMMING
		Capture the order of the button presses for 2 to 10 buttons.  When
		the users presses teh program button again, store the sequence as 
		the winning sequence and go to the reset routine.
		
		The sequence is stored as the pin numbers of the buttons
		(eg sequence SW1   SW2  SW1   SW3 is stored as 2,3,2,4)
		
	RESET
		Called at startup, after programming and when the reset input is activated
		Turn the maglock (RL1) on
		Turn the laser control (RL2) on
		Clear any in progress game play.
		Go to game play.

	GAME PLAY

		stage 1 - laser game
			Monitor SW1 - SW4.  Each time a button is pressed, see if it is
			the next button in the winning sequence.  If it is, continue the sequence
			if it is not, reset the sequence buffer.
			
			If the full winning sequence is entered, deactivate the laser, go to 
			stage 2.
			
			SW5 is inactive/ignored during stage 1.
		
		Stage 2 - maglock
			SW1-4 are inactive/ignored during stage 2.
			Activating SW5 in stage 2 turns off the maglock.
			
			The game will sit idle in stage 2 until the reset is activated.
			
 */

#include <EEPROM.h>     // winning sequence will be stored in EEProm so it is not lost when power is cycled

// Constant Values

#define DEBOUNCE_DLY 50  			// Number of mS before a switch state is accepted


// I/O Pin Naming

#define 	SW1		2			//four buttons for the laser sequence game
#define		SW2		3			//the code relies on these four buttons
#define		SW3		4			//being adjacent pin numbers
#define		SW4		5

#define 	MagButton	6		//mag lock release button (SW5)
#define		PrgButton	7		//Program button
#define		RstButton	8		//reset button

#define		MagOut		9		//maglock output
#define		LasOut		14		//laser output
#define		LED1		15		//LED

// Variables/Storage

byte			NextButton = 1;			//holds the EEPROM address for the next button in sequence
byte			EEVal = 255;			//used to read the value from the EEPROM
bool 			Stage2 = false;			//indicate if the game has reached stage 2

bool			SW1State;
bool			SW2State;
bool			SW3State;
bool			SW4State;
bool			MagState;
bool			PrgState;
bool			RstState;


void setup()
{
	// put your setup code here, to run once:
	
	// serial setup
	Serial.begin(9600);
	Serial.println("Serial interface started");
	
	//Pin Setup
	pinMode(SW1,INPUT_PULLUP);
	pinMode(SW2,INPUT_PULLUP);
	pinMode(SW3,INPUT_PULLUP);
	pinMode(SW4,INPUT_PULLUP);
	
	pinMode(MagButton,INPUT_PULLUP);
	pinMode(PrgButton,INPUT_PULLUP);
	pinMode(RstButton,INPUT_PULLUP);
	
	pinMode(MagOut,OUTPUT);
	pinMode(LasOut,OUTPUT);
	pinMode(LED1,OUTPUT);
	
	
	
	EEVal = EEPROM.read(NextButton);
	if (EEVal > SW4 || EEVal < SW1)		// if the next button value is invalid, goto programming mode	
	{
		Serial.println("No winning sequence detected, going to program mode");
		RecordSequence();
	}
	
	ResetGame();
	
	
}

void loop() {
	// put your main code here, to run repeatedly:
	
	if (!Stage2)							//if stage2 is false, check the 
	{	//buttons for the laser game
		CheckButton(SW1, SW1State);
		CheckButton(SW2, SW2State);
		CheckButton(SW3, SW3State);
		CheckButton(SW4, SW4State);
	} else									//if stage 2 is active, check the
	{	//button for the maglock
		if (DebounceSW(MagButton) != MagState)
		{
			MagState = !MagState;
			if (MagState)
			{
				Serial.print("Mag button activated - turning off maglock");
				digitalWrite(MagOut,LOW);
			}
		}
	}
	
	if (DebounceSW(PrgButton) !=PrgState)
	{	
		PrgState = !PrgState;
		if (PrgState) {	RecordSequence();}
	}
		
	if (DebounceSW(RstButton) != RstState)  
	{	
		RstState = !RstState;
		if (RstState) {ResetGame();}
	}
		
}	//end of loop

void CheckButton(byte SWx, bool& SWState)
{	//check the laser game buttons.
	//Read the button, 
	//if it is the next in the sequence
	//	  advance the sequence
	//	  if the sequence is finished, go to stage 2
	//if it is not the next in sequence,
	//	  reset the sequence
	
	bool IsActive;
	IsActive = DebounceSW(SWx);
	
	if (IsActive == SWState) { return;}	//if the switch state is the same as it was before, take no action
	
	SWState = IsActive;					//store the new state
	
	if (!IsActive) {	return;}		//if the new state is off - return now
	
	Serial.print("Pin # ");
	Serial.print(SWx);
	Serial.println(" activated.");
	
	//the switch is active
	EEVal = EEPROM.read(NextButton);
	
	if (SWx == EEVal)
	{
		NextButton++;						//increment the button address
		EEVal = EEPROM.read(NextButton);	//read the new value
		
		Serial.print("Next expected pin activation: ");
		Serial.println(EEVal);
		
		if (EEVal == 255)					//if the value is 255, the sequence is over
		{
			Serial.println("Sequence solved, going to stage 2!");
			Stage2 = true;
			digitalWrite(LasOut,LOW);
		} 
		
	} else
	{
		Serial.println("Wrong button pressed, resetting sequence");
		NextButton = 1;
		EEVal = EEPROM.read(NextButton);	//read the new value
		
		Serial.print("Next expected pin activation: ");
		Serial.println(EEVal);
	}
	
} //end of CheckButton

void RecordSequence()
{
	//record a new winning sequence by monitoring the buttons.
	//for each game button pressed, store the value.  Also montor
	//the program button - when it is pressed, write 255 to the next
	//address and exit programming.
	
	Serial.println("Entered programming mode");
	
	NextButton = 1;
	do
	{
		if (DebounceSW(SW1) == true ) {	AddButton(SW1);}
		if (DebounceSW(SW2) == true ) {	AddButton(SW2);}
		if (DebounceSW(SW3) == true ) {	AddButton(SW3);}
		if (DebounceSW(SW4) == true ) {	AddButton(SW4);}
		
		if (DebounceSW(PrgButton) == true) 
		{
			AddButton(255);
			NextButton = 11;
		}
		
	} while (NextButton<11);
	
	ResetGame();
	
	
} //end of RecordSequence

void AddButton(byte SWx)
{
	EEPROM.write(NextButton,SWx);
	Serial.print(" Pin # ");
	Serial.print(SWx);
	Serial.println(" added to sequence.");
	NextButton++;
}

void ResetGame()
{
	//Reset the game and get ready for a new player
	
	NextButton = 1;								//reset to the first expected button
	EEVal = EEPROM.read(NextButton);			//read the value
	
	
	digitalWrite(MagOut,HIGH);					//turn on the mag and laser
	digitalWrite(LasOut,HIGH);
	
	Serial.println("The game is reset, mag on, laser on, ready to play!");
	
	Serial.print("Next expected pin activation: ");
	Serial.println(EEVal);
		
} //end of ResetGame


bool DebounceSW(byte SWx)
{
	//read the passed switch twice and make sure both readings match
	//this prevents multiple triggers due to mechanical noise in 
	//the switch
	
	
	bool PossVal2;
	bool PossVal = !digitalRead(SWx);		//invert the reading due to the use of pullups
	
	while(true)
	{
		delay(DEBOUNCE_DLY);					//delay by the debounce amount
		PossVal2 = !digitalRead(SWx);			//re-read the switch
		
		if (PossVal == PossVal2)				//if the two reads are the same
		{
			return (PossVal);					//return the read value
		}
		
		//this code will only execute if the two reads did not match
		//Now read the pin again and look for a match.
		//If the button is cycling very fast, it is possible the code
		//will deadlock here.  This is a very slim possibility
		
		PossVal = !digitalRead(SWx);			//re-take the first reading
		//and loop back to the delay
	}
	
	return (PossVal); 	//this line is never executed, but makes the compiler happy.
	
}






