enum blinkStates {SLEEP, WAKE, PROD, GAMEA, GAMEB, MENU, PROGRAMA, PROGRAMB, LEARNA, LEARNB, READY, YAWN, KO};
byte blinkState;
bool menuToggle = true;
byte currentGame = GAMEB;
byte programType = PROGRAMB;
bool programInitiator = false;
Timer animTimer;
int animFrame = 0;

Timer idleTimer;
Timer menuIdleTimer;
Timer learnTimer;

void setup() {
  // put your setup code here, to run once:
  blinkState = WAKE;
  idleTimer.set(60000);
}

void loop() {
  // first, the loop switch
  switch (blinkState) {
    case SLEEP:
      sleepLoop();
      break;
    case WAKE:
    case PROD:
      wakeLoop();
      break;
    case GAMEA:
    case GAMEB:
      gameLoop();
      break;
    case MENU:
      menuLoop();
      break;
    case PROGRAMA:
    case PROGRAMB:
      programLoop();
      break;
    case LEARNA:
    case LEARNB:
      learnLoop();
      break;
    case READY:
      readyLoop();
      break;
    case YAWN:
    case KO:
      yawnLoop();
      break;
  }

  //now some housekeeping, like making sure the sleep timer is reset after ANY interaction, and that you go to sleep if it expires
  if (buttonDown()) {
    idleTimer.set(120000);
  }

  if (idleTimer.isExpired() && blinkState != SLEEP) {
    changeState(SLEEP);
    buttonDoubleClicked();
    buttonMenuPressed();
  }

  setValueSentOnAllFaces(blinkState);
}

void sleepLoop() {
  if (buttonSingleClicked()) { //someone has hit the button. This is method 1 of waking up
    changeState(WAKE);
  }

  FOREACH_FACE(f) { //this looks for neighbors in WAKE state. Method 2 of waking up
    if (!isValueReceivedOnFaceExpired(f) && getLastValueReceivedOnFace(f) == PROD) {
      changeState(WAKE);
      idleTimer.set(60000);
    }
  }

  if (blinkState == WAKE) {
    animFrame = 0;
  }
}

void wakeLoop() {
  if (animTimer.isExpired()) { //do the wake animation.
    animFrame++;
    setColor(dim(YELLOW, animFrame * 17));
    animTimer.set(100);
  }

  if (animFrame > 3) { //you are halfway through wakeup. Start waking neighbors
    changeState(PROD);
  }

  if (animFrame == 15) { //last frame of animation
    changeState(currentGame);
  }
}

void gameLoop() {
  if (animTimer.isExpired()) {// do a basic game animation thing
    if (blinkState == GAMEA) {//just a simple animation that turns on and off faces randomly
      setColorOnFace(ORANGE, rand(5));
      setColorOnFace(OFF, rand(5));
      setColorOnFace(ORANGE, rand(5));
      animTimer.set(rand(1) * 100 + 100);
    } else if (blinkState == GAMEB) {//a simple animation that randomly fades a face
      setColorOnFace(dim(RED, rand(255)), rand(5));
      animTimer.set(150);
    }
  }

  //now we need to start looking out for menu press, but only when alone
  if (buttonMenuPressed()) {//you have to do this first to set the flag to false on every check
    if (isAlone()) {
      changeState(MENU);
      menuIdleTimer.set(10000);
    }
  }

  //we need to look out for a KO or programming signals from neighbors
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//you found a neighbor
      if (getLastValueReceivedOnFace(f) == KO) { //KO command
        changeState(YAWN);
        menuIdleTimer.set(3000);//secondhand sleep timer
      } else if (getLastValueReceivedOnFace(f) == PROGRAMA) {
        changeState(LEARNA);
        learnTimer.set(6000);
      } else if (getLastValueReceivedOnFace(f) == PROGRAMB) {
        changeState(LEARNB);
        learnTimer.set(6000);
      }
    }//end of found neighbor statement
  }//end of foreachface loop
}

void menuLoop() {
  if (buttonSingleClicked()) {
    menuToggle = !menuToggle;//reverse the toggle
    setColorOnFace(dim(GREEN, 16), 0);//reset all the dimness, let the next thing correct for choice
    setColorOnFace(dim(GREEN, 16), 1);
    setColorOnFace(dim(GREEN, 16), 2);
    setColorOnFace(dim(BLUE, 16), 3);
    setColorOnFace(dim(BLUE, 16), 4);
    setColorOnFace(dim(BLUE, 16), 5);
    menuIdleTimer.set(10000);
  }

  if (animTimer.isExpired()) {
    animFrame++;
    if (menuToggle) { //menu toggle true is in program choice
      setColorOnFace(GREEN, 0);//reset all the greens
      setColorOnFace(GREEN, 1);
      setColorOnFace(GREEN, 2);
      setColorOnFace(WHITE, animFrame % 3);//set one white
      animTimer.set(300);
    } else if (!menuToggle) { //menu toggle false is in sleep choice
      setColorOnFace(dim(BLUE, abs(animFrame - 15) * 17), 3);
      setColorOnFace(dim(BLUE, abs(animFrame - 15) * 17), 4);
      setColorOnFace(dim(BLUE, abs(animFrame - 15) * 17), 5);
      animTimer.set(100);
    }

    if (animFrame == 30) {
      animFrame = 0;
    }
  }

  if (buttonDoubleClicked()) { //type selected, move to appropriate mode
    if (menuToggle) {//head to the correct programming mode
      changeState(programType);
      programInitiator = true;
      menuIdleTimer.set(6000);
    } else if (!menuToggle) {
      changeState(YAWN);
      menuIdleTimer.set(3000);
    }
  }

  if (menuIdleTimer.isExpired()) {
    changeState(currentGame);
  }
}

void programLoop() {
  if (animTimer.isExpired()) {//just the animation
    animFrame++;
    setColor(GREEN);
    setColorOnFace(WHITE, animFrame % 3);
    setColorOnFace(WHITE, (animFrame % 3) + 3);
    animTimer.set(100);
  }

  if (buttonDoubleClicked()) {//return to menu if double clicked
    changeState(MENU);
    menuIdleTimer.set(10000);
    programInitiator = false;
  }

  //in order to move from program to ready mode
  //you need to check all neighbors
  //IF you have neighbors in LEARNA, LEARNB, GAMEA, or GAMEB, stay in program mode
  //only once you have no neighbors of that type can you move on
  if (!isAlone) {
    byte neighborsInNeed = 0;
    FOREACH_FACE(f) {
      byte neighborState = getLastValueReceivedOnFace(f);
      if (!isValueReceivedOnFaceExpired(f)) { //there's something here
        switch (neighborState) {
          case LEARNA:
          case LEARNB:
          case GAMEA:
          case GAMEB:
            neighborsInNeed++;
            break;
        }//end of switch
      }
    }

    if (neighborsInNeed == 0) { //no neighbors need programming anymore. Move to ready
      changeState(READY);
      if (programInitiator) {//this is where the program initiator actually changes game
        switch (programType) {
          case PROGRAMA:
            currentGame = GAMEA;
            break;
          case PROGRAMB:
            currentGame = GAMEB;
            break;
        }
      }
    }
  }//end of isAlone check

  if (menuIdleTimer.isExpired()) {
    changeState(currentGame);
    programInitiator = false;
  }
}

void learnLoop() {
  if (animTimer.isExpired()) {//a simple green fading animation
    animFrame ++;
    animTimer.set(100);
    setColor(dim(GREEN, abs(animFrame - 15) * 17));
  }

  if (animFrame == 30) {
    animFrame = 0;
  }

  if (learnTimer.isExpired()) {//this means the game has been "learned" and we can move to PROGRAM state
    if (blinkState == LEARNA) {
      currentGame = GAMEA;
      changeState(PROGRAMA);
    } else if (blinkState == LEARNB) {
      currentGame = GAMEB;
      changeState(PROGRAMB);
    }
  }
}

void readyLoop() {
  //all we gotta do here is move to game state when it's safe
  //mimicing the logic from program, we wait until no neighbors are programming or learning
//  if (!isAlone) {
//    byte neighborsInProgress = 0;
//    FOREACH_FACE(f) {
//      byte neighborState = getLastValueReceivedOnFace(f);
//      if (!isValueReceivedOnFaceExpired(f)) { //there's something here
//        switch (neighborState) {
//          case LEARNA:
//          case LEARNB:
//          case PROGRAMA:
//          case PROGRAMB:
//            neighborsInProgress++;
//            break;
//        }//end of switch
//      }
//    }
//
//    if (neighborsInProgress == 0) { //no neighbors need programming anymore. Move to ready
//      changeState(currentGame);
//    }
//  }//end of isAlone check
}

void yawnLoop() {
  if (animTimer.isExpired()) {
    animFrame++;
    setColor(dim(BLUE, abs(animFrame - 15) * 17));
    animTimer.set(100);
  }

  if (animFrame == 3) {
    changeState(KO);
  }

  if (animFrame == 30) {
    animFrame = 0;
  }

  if (buttonDoubleClicked()) {//return to menu if double clicked
    changeState(MENU);
    menuIdleTimer.set(10000);
  }

  if (menuIdleTimer.isExpired()) {
    changeState(SLEEP);
    buttonDoubleClicked();
    buttonMenuPressed();
  }
}

void changeState(byte newState) {
  switch (newState) {
    case SLEEP:
      setColor(OFF);
      break;
    case GAMEA:
      setColor(ORANGE);
      break;
    case GAMEB:
      setColor(RED);
      break;
    case MENU:
      setColorOnFace(dim(GREEN, 16), 0);
      setColorOnFace(dim(GREEN, 16), 1);
      setColorOnFace(dim(GREEN, 16), 2);
      setColorOnFace(dim(BLUE, 16), 3);
      setColorOnFace(dim(BLUE, 16), 4);
      setColorOnFace(dim(BLUE, 16), 5);
      animFrame = 0;
      break;
    case PROGRAMA:
    case PROGRAMB:
    case LEARNA:
    case LEARNB:
      setColor(GREEN);
      animFrame = 0;
      break;
    case READY:
      setColor(dim(GREEN, 16));
      break;
    case YAWN:
      setColor(BLUE);
      animFrame = 0;
      break;
  }

  blinkState = newState;
}

