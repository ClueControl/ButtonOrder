# ButtonOrder

This game required players to first push 4 buttons in an defined order.  Solving this turns off a laser output and enables the second stage.
The second stage is to push an open button which will turn off the maglock output.

Things to add
   * LED indicators
   * Monitor laser sensor and take action when players break the laser beam
   

STARTUP
If a winning pattern is not stored for the laser game, flash the indicators and wait for a winning pattern to be stored.  If a winning pattern is stored, proceed to reset.

PROGRAMMING
To store a new pattern, game master presses SW1-4 in a patter of 2 to 10 steps.  When complete, the game master presses the program button.  The unit will confirm with LED blinks and the pattern will be stored until the game master changes it again (it will not be lost when power is turned off).

RESET
Turn the maglock (RL1) on
Turn the laser control (RL2) on

GAME PLAY

If players enter a wrong pattern for the laser game, blink the indicator.
If players enter a correct pattern for the laser game, turn off the laser indicator.

If players press SW1 after entering a correct pattern, turn off the maglock.
	If players press SW1 before entering the correct pattern, nothing happens.

---------------------------

These repositoreis are here to help escape room owners add cool puzzles and props to their rooms.  

We make ClueControl, which is software used to monitor, control and automate escape rooms.  Please consider visiting our website at www.cluecontrol.com to check out what else we have to offer!
