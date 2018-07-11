import processing.serial.*;

Serial myPort;  // The serial port
String[] commands;
String fileName;
int cmdPointer = 0;
int cmdLength;
boolean sendCommands = false;

void loadFile(File selection) 
{
  if (selection == null)
  {
     println("No valid file was selected"); 
     exit();
  }
  commands = loadStrings(selection);
  cmdLength = commands.length;
}

void setup() {
  selectInput("Select your drawbot (*.txt) command file", "loadFile");
  printArray(Serial.list());
  myPort = new Serial(this, Serial.list()[0], 9600);
  size(400, 150 );
}

void keyPressed()
{
  if (key == ENTER || key == RETURN)
  {
     sendCommands = !sendCommands; 
  }
}


void draw() {
  background(255);
  fill(0);
  if (cmdLength > 0)
  {
    if (sendCommands)
      {
      while (myPort.available() > 0) {
        if (cmdPointer < cmdLength){
          String inBuffer = myPort.readString();   
          if (inBuffer != null) {
            println(inBuffer);
            if (inBuffer == "CMDREQUEST");
              println(commands[cmdPointer] + "\n");
              myPort.write(commands[cmdPointer] + "\n");
              cmdPointer++;
          }
        }
      }
      fill(0);
      text("Sending Command " + str(cmdPointer) + " of " + str(cmdLength),110,85);
      fill(255);
      stroke(0);
      rect(0,100,399,49);
      noStroke();
      fill(0,255,0);
      rect(1,101,map(cmdPointer,0,cmdLength-1,0,396),48);
    }
    else
      text("Press ENTER to start/pause sending commands.",75,85);
  }
}
