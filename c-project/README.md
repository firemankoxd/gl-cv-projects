# C semester assignment
### About
This is my semester work for subject called *Algorithmization and Programming*. Please, keep in mind that this was my first coding project on University (created in 2018). It is far away from well-written code and it is meant to demonstrate mostly the problem solving skill I used back then.
*This readme file is meant to help you understand what is the code about, as I didn't want to risk renaming all the variables to english.*

### What does the software do?
The software was designed to be a console version of MS Paint. User starts with a menu with the following options: **Create**, **Load**, **Controls** and **Exit**. Each menu iteam can be reached with UP and DOWN arrow keys. When the **Controls** option is selected, the screen shows key bindings for 5 seconds and the app goes back to the menu. After selecting **Create** option, the blank drawing screen is loaded. The screen is surrounded by a frame, which represents X and Y limits of cursor movement. User can move cursor via arrow keys, draw a character on a cursor position with `Space` key, delete the charater on cursor position with `C` key, draw a rectangle with `R` key, erase the whole drawing with `M` key and save the drawing with the `S` key. After user saves the drawing, exits the app, and launching it again, he can choose **Load** option to load his drawing from the file.

### Variables translations
As I stated before, I found it easier to tell you about variables instead of renaming each one in the code. Here's a small table which translates majority of slovak-named variables that I used into english ones.

| Original variable name | Translation | Comment |
| --------------- | --------------- | --------------- |
| `znak` | Character/Symbol | - |
| `vytvor` | Create | Creating a new drawing |
| `nacitaj` | Load | Loading an existing drawing |
| `moznostMenu` | Menu option | Used to keep track of chosen option in menu |