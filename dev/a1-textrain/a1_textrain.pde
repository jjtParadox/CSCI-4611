/**
    CSci-4611 Assignment #1 Text Rain
**/


import processing.video.*;

// Global variables for handling video data and the input selection screen
String[] cameras;
Capture cam;
Movie mov;
PImage inputImage;
boolean inputMethodSelected = false;

float threshold = 0.5;
boolean showThreshold = false;

boolean blur = false;

boolean flip = false;

boolean processingImage = false;
boolean doneProcessing = false;
PImage bufferImage;
PImage thresholdImage;

PFont font;

String poem = "The rain is raining all around, It falls on field and tree, It rains on the umbrellas here, And on the ships at sea.";

int NUM_LETTERS = poem.length();

Letter[] letters;

Letter randomLetter(int pos) {
  Letter l = new Letter();
  
  l.yPos = -random(height);
  l.xPos = pos < poem.length() ? ((float) pos / (float) poem.length()) * (float) width + random(5) : random(width);
  
  l.letter = pos < poem.length() ? poem.charAt(pos) : (char) (random(26) + 'a');
  
  l.vel = random(30,120)/1000.0;
  l.lastUpdate = millis();
  
  return l;
}

void setup() {
  size(1280, 720);  
  inputImage = createImage(width, height, RGB);
  bufferImage = createImage(width, height, RGB);
  thresholdImage = createImage(width, height, RGB);
  
  thresholdImage.loadPixels();
  for (int i = 0; i < thresholdImage.pixels.length; i++) {
    thresholdImage.pixels[i] = color(255, 255, 255);
  }
  
  font = createFont("lmroman10-regular.otf", 20);
  
  letters = new Letter[NUM_LETTERS];
  for (int i = 0; i < letters.length; i++) {
    letters[i] = randomLetter(i);
    letters[i].yPos += height;
  }
  thread("processLetters");
}


void draw() {
  // When the program first starts, draw a menu of different options for which camera to use for input
  // The input method is selected by pressing a key 0-9 on the keyboard
  if (!inputMethodSelected) {
    cameras = Capture.list();
    int y=40;
    text("O: Offline mode, test with TextRainInput.mov movie file instead of live camera feed.", 20, y);
    y += 40; 
    for (int i = 0; i < min(9,cameras.length); i++) {
      text(i+1 + ": " + cameras[i], 20, y);
      y += 40;
    }
    return;
  }


  // This part of the draw loop gets called after the input selection screen, during normal execution of the program.

  
  // STEP 1.  Load an image, either from a movie file or from a live camera feed. Store the result in the inputImage variable
  
  if ((cam != null) && (cam.available())) {
    cam.read();
    inputImage.copy(cam, 0,0,cam.width,cam.height, 0,0,inputImage.width,inputImage.height);
  }
  else if ((mov != null) && (mov.available())) {
    mov.read();
    inputImage.copy(mov, 0,0,mov.width,mov.height, 0,0,inputImage.width,inputImage.height);
  }


  // Fill in your code to implement the rest of TextRain here..
  
  if (doneProcessing) {
    thresholdImage = bufferImage;
    processingImage = false;
    doneProcessing = false;
  }
  if (!processingImage) { 
    processingImage = true;
    //thread("processInput");
    new Thread() { public void run() { processInput(); } }.start();
  }
  
  inputImage.filter(GRAY);

  // Tip: This code draws the current input image to the screen
  pushMatrix();
  if (flip)
    scale(-1, 1);
  //set(0, 0, showThreshold ? thresholdImage : inputImage);
  image(showThreshold ? thresholdImage : inputImage, flip ? -width : 0, 0);
  popMatrix();
  
  fill(0, 0, 128);
  textFont(font);
  for (Letter i : letters) {
    text(i.letter, i.xPos, i.yPos);
  }
  
  //println(frameRate);
}

void processInput() {
  bufferImage = createImage(width, height, RGB);
  bufferImage.copy(inputImage, 0,0,inputImage.width,inputImage.height, 0,0,bufferImage.width,bufferImage.height);
  if (blur) {
    bufferImage.filter(BLUR);
  }
  bufferImage.filter(THRESHOLD, threshold);
  doneProcessing = true;
}

void processLetters() {
  while(true) {
    thresholdImage.loadPixels();
    for (int i = 0; i < letters.length; i++) {
      Letter l = letters[i];
      int time = millis();
      float nextPos = l.yPos;
      float xPos = flip ? width - l.xPos : l.xPos;
      if (xPos < width && xPos >= 0 && l.yPos + 5 < height && l.yPos + 5 >= 0) {
        if (brightness(thresholdImage.get((int) xPos, (int) l.yPos + 5)) > 128) {
          nextPos = l.yPos + (float) (time - l.lastUpdate) * l.vel;
        }
        if (nextPos + 4 < height) {
          while (nextPos + 4 >= 0 && brightness(thresholdImage.get((int) xPos, (int) nextPos + 4)) < 128) {
            nextPos -= 1;
          }
        }
      } else {
          nextPos = l.yPos + (float) (time - l.lastUpdate) * l.vel;
      }
      //try {
      //if (brightness(thresholdImage.get((int) l.xPos, (int) l.yPos + 10)) > 128) {
        //l.yPos += (float) (time - l.lastUpdate) * l.vel;
      //}
      //} catch (Exception e) {
        //l.yPos += (float) (time - l.lastUpdate) * l.vel;
      //}
      //try {
        //while (brightness(thresholdImage.pixels[(int) ((l.yPos + 10) * width + l.xPos)]) < 128) {
          //l.yPos -= 1;
        //}
      //} catch (Exception ignored) {}
      l.yPos = nextPos;
      l.lastUpdate = time;
      if (l.yPos > height) {
        letters[i] = randomLetter(i);
      }
    }
  }
}
  
class Letter {
  char letter;
  
  float xPos;
  float yPos;
  float vel = 50.0/1000.0;
  
  int lastUpdate;
}

void keyPressed() {
  
  if (!inputMethodSelected) {
    // If we haven't yet selected the input method, then check for 0 to 9 keypresses to select from the input menu
    if ((key >= '0') && (key <= '9')) { 
      int input = key - '0';
      if (input == 0) {
        println("Offline mode selected.");
        mov = new Movie(this, "TextRainInput3.mov");
        mov.loop();
        inputMethodSelected = true;
      }
      else if ((input >= 1) && (input <= 9)) {
        println("Camera " + input + " selected.");           
        // The camera can be initialized directly using an element from the array returned by list():
        cam = new Capture(this, cameras[input-1]);
        cam.start();
        inputMethodSelected = true;
      }
    }
    return;
  }


  // This part of the keyPressed routine gets called after the input selection screen during normal execution of the program
  // Fill in your code to handle keypresses here..
  
  if (key == CODED) {
    if (keyCode == UP) {
      threshold += 0.05;
    }
    else if (keyCode == DOWN) {
      threshold -= 0.05;
    }
  }
  else if (key == ' ') {
    showThreshold = !showThreshold;
  }
  else if (key == 'b') {
    blur = !blur;
  }
  else if (key == 'f') {
    flip = !flip;
  }
  
}
