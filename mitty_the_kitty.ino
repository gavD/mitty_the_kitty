#include <Arduboy2.h>
#include "sprites.h"

BeepPin1 beep;     // Simple sound effects instance
Arduboy2 arduboy;  // The main Arduboy library

// Constants - these are defined to give contextual information instead of "magic numbers"

// Constants - enemies
constexpr uint8_t BAT_Y_TO_PIXEL_MULTIPLER = 10;  // Multiplier from bat's per-pixel position to hi-res position
constexpr uint8_t BAT_Y_INITIAL = 10;
constexpr uint16_t BAT_Y_MAX = 530;                     // Hi-res max y-position of the bat
constexpr uint16_t BIRD_INITIAL_Y = 710;                // Hi-res initial y-position of the bird - to perch it at the top of a tree
constexpr uint8_t BIRD_X_TO_PIXEL_MULTIPLIER = 4;      // Multiplier from bird's per-pixel x position to hi-res position
constexpr uint8_t BIRD_Y_TO_PIXEL_MULTIPLIER = 23;     // Multiplier from bird's per-pixel y position to hi-res position
constexpr uint16_t SQUIRREL_START_X = 6600;             // Start the squirrel way off screen. This is a hi-res position, which divides down to a pixel position.
constexpr uint8_t SQUIRREL_X_TO_PIXEL_MULTIPLER = 30;  // Multiplier from squirrel's per-pixel position to hi-res position

// Constants - player
constexpr uint8_t PLAYER_DUFF_COOLDOWN_X = 22;            // How many frames between Mitty's attacks
constexpr uint8_t PLAYER_X_INITIAL = 1;                   // Position Mitty starts at
constexpr uint8_t PLAYER_LIVES_INITIAL = 3;               // How many lives Mitty starts with
constexpr uint8_t PLAYER_SPRITE_FACING_LEFT_OFFSET=  4;   // Frame offset of the player sprite if facing left
constexpr uint8_t PLAYER_SPRITE_FACING_RIGHT_OFFSET = 0;  // Frame offset of the player sprite if facing right
constexpr uint8_t PLAYER_X_TO_PIXEL_MULTIPLIER = 20;      // Multiplier from the player's per-pixel position to hi-res Xposition

// Constants - game state
constexpr uint8_t GAME_STATE_INTRO = 0;
constexpr uint8_t GAME_STATE_LEVEL_TRANSITION = 1;
constexpr uint8_t GAME_STATE_GAME = 2;
constexpr uint8_t GAME_STATE_GAME_OVER = 3;
constexpr uint8_t GAME_STATE_WON = 4;

// BARE globals fam

// player state
bool player_ducking = false;                  // whether the player is ducking right now
uint8_t player_facing;                        // Corresponds to offset of the player sprite for facing right and left.
uint8_t player_lives = PLAYER_LIVES_INITIAL;  // How many lives Mitty starts with
uint16_t player_x;                            // Player's horizontal position
uint8_t player_y = (64 - 17);                 // Player's vertical position; 17px from the bottom. The sprite is 16px tall so the kitty "walks" on the baseline.
uint8_t player_duff_cooldown;                 // Cooldown from making duffs
uint16_t player_duff_count = 0;               // how many enemies the player has duffed
uint8_t player_jump_frame_index = 0;          // the current frame the player's jump is in, out of player_jump_pixel_offsets
uint8_t player_jump_mod;                      // TODO document
uint8_t player_jump_pixel_offsets[] = {       // The offsets in pixels of the jump curve for the player. This is done by eye until it felt OK.
  1, 4, 9, 15,
  22, 30, 30, 32,
  30, 29, 28, 24,
  19, 14, 9, 4
};
bool player_jumping;    // TODO document
bool player_scurrying;  // True if the player has been hit and is scurrying back to the save zone on the left

// enemy state
uint8_t bat_x_pixels[] = { 0, 0, 0, 0 };                  // Horizontal locations of the bats. Randomised in the next_level function.
uint16_t bat_y[] = { 0, 0, 0, 0 };                        // Vertical hi-res positions of the bats
bool bat_flying_down[] = { false, false, false, false };  // Used for reversing the direction of bat travel as they get to the highest/lowest points of their cycle.
bool bird_triggered;                                      //  In daytime, if the player jumps, the bird flies out of a tree
int16_t bird_x;                                           // horizontal position of the bird. Not in pixels; in a larger integer type that gets scaled to integers. See file header comment.
int16_t bird_y;                                           // vertical position of the bird. Not in pixels; in a larger integer type that gets scaled to integers. See file header comment.
uint8_t minion_x_pixels[] = { 0, 0, 0, 0 };               // max 4
bool minion_duffed[] = { false, false, false, false };
uint16_t squirrel_x;  // horizontal position of the squirrel. Not in pixels; in a larger integer type that gets scaled to integers. See file header comment.


// Other UI elements
uint8_t cloud_xs[] = { 0, 0, 0 };      // horizontal position of clouds
uint8_t cloud_ys[] = { 0, 0, 0 };      // vertical position of clouds
uint8_t tree_x[] = { 0, 0, 0, 0, 0 };  // horizontal positions of trees
uint8_t tree_y = (player_y - 17);      // Tree's vertical position; 17px from the bottom. The sprite is 16px tall so the kitty "walks" on the baseline.

// Game state
uint8_t frame_counter = 0;              // Cycling frame counter. I started off doing all calculations based on pixels
                                        // per frame rate but quickly realised it was better to move fractions of a pixel
                                        // each frame and then map that to actual pixels because moving 60 pixels in a
                                        // second is WAY too fast!
uint8_t game_state = GAME_STATE_INTRO;  // The state of the game - intro, level screen, gameplay, game over, game won
bool is_night = true;                   // If it's night, we invert the UI and some creepies come out!
uint8_t level = 0;                      // Level the player is on

/**
 * Inverts day/night, including inverting the display
 */
void invert_bg() {
  is_night = !is_night;
  arduboy.invert(!is_night);
}

/**
 * @return The number of critters that should be rendered into the level. Goes up as the levels go up.
 */
uint8_t max_critter_count() {
  return min(4, uint8_t(level / 2));
}

/**
 * Load the next level
 */
void next_level() {
  invert_bg();
  level++;

  // Go through each critter and randomise the positions
  for (uint8_t i = 0; i < max_critter_count(); i++) {
    // bats
    bat_flying_down[i] = false;
    bat_x_pixels[i] = random(30, 110);  // TODO ensure they don't overlap?
    // set random start Y point for each bat
    bat_y[i] = random(BAT_Y_INITIAL, BAT_Y_MAX);

    // minions
    minion_duffed[i] = false;
    minion_x_pixels[i] = random(30, 111);
  }
  bird_y = BIRD_INITIAL_Y;
  bird_triggered = false;
  squirrel_x = 120 * 30;

  // randomise onscreen "furniture"
  for (uint8_t i = 0; i < 3; i++) {
    cloud_xs[i] = random(0, 107);
    cloud_ys[i] = random(0, 30);
  }
  // put the bird in a random tree
  uint8_t random_tree_for_bird_to_be_in = random(0, 3);
  for (uint8_t i = 0; i < 5; i++) {  // todo magic number
    // perch the bird on the first tree
    tree_x[i] = (i * 25) + random(1, 11);
    if (i == random_tree_for_bird_to_be_in) {
      bird_x = (tree_x[i] * BIRD_X_TO_PIXEL_MULTIPLIER) + BIRD_X_TO_PIXEL_MULTIPLIER;
    }
  }

  // reset the player
  player_x = PLAYER_X_INITIAL;
  player_duff_cooldown = 0;
  player_jumping = false;
  player_jump_mod = 0;
  player_jump_frame_index = 0;
  player_scurrying = false;

  // handle the level transition by changing to the appropriate game state
  if (level >= 9) {
    game_state = GAME_STATE_WON;
  } else {
    game_state = GAME_STATE_LEVEL_TRANSITION;
  }
}

// Called when a squirrel gets hit by Mitty
void duffSquirrel() {
  squirrel_x = SQUIRREL_START_X;
  player_duff_count++;
}

/**
 * Called when a minion gets hit by Mitty
 * @param i Index of the minion in the list of minions
 */
void duffMinion(uint8_t i) {
  minion_duffed[i] = true;
  player_duff_count++;
}

/**
 * Called when the player gets hit. Sends Mitty scurrying back
 * to the safe zone on the left of the screen!
 *
 * If the player is out of lives, the game is over!
 */
void loseLife() {
  player_scurrying = true;
  player_lives--;
  if (player_lives == 0) {
    game_state = GAME_STATE_GAME_OVER;
  }
}

/**
 * Initial set up of the game
 * Arduino standard function
 */
void setup() {
  arduboy.begin();       // Arduboy - go!
  arduboy.clear();       // Clear the output
  arduboy.invert(true);  // Set the initial state of the screen to inverted
  beep.begin();          // set up the hardware for playing tones
}

// display methods

inline void displayScreenIntro() {
  level = 0;
  player_lives = PLAYER_LIVES_INITIAL;
  arduboy.setCursor(1, 5);
  arduboy.print("Mitty the Kitty 0.2");
  arduboy.setCursor(2, 18);
  arduboy.print("A game by");
  arduboy.setCursor(2, 28);
  arduboy.print("Bean (5) & Button (4)");
  arduboy.setCursor(1, 40);
  arduboy.print("April '23 gavd.co.uk");

  arduboy.setCursor(1, 53);
  arduboy.print("Press a button!");

  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    game_state = GAME_STATE_LEVEL_TRANSITION;
    next_level();
  }
  arduboy.display(CLEAR_BUFFER);
}

inline void displayScreenLevelTransition() {
  arduboy.setCursor(1, 5);
  arduboy.print("Mitty the Kitty");
  arduboy.setCursor(10, 20);
  arduboy.print("Level");
  arduboy.setCursor(60, 20);
  arduboy.print(String(level));

  arduboy.setCursor(10, 30);
  arduboy.print("Duffs");
  arduboy.setCursor(60, 30);
  arduboy.print(String(player_duff_count));

  arduboy.setCursor(1, 50);
  arduboy.print("Press a button!");
  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    game_state = GAME_STATE_GAME;
  }
  arduboy.display(CLEAR_BUFFER);
}

inline void displayScreenGameOver() {
  arduboy.setCursor(1, 10);
  arduboy.print("Game over!");
  arduboy.setCursor(1, 30);
  arduboy.print("You got to level ");
  arduboy.setCursor(100, 30);
  arduboy.print(String(level));

  arduboy.setCursor(1, 50);
  arduboy.print("Press a button!");
  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    game_state = GAME_STATE_INTRO;
  }
  arduboy.display(CLEAR_BUFFER);
}

inline void displayScreenGameWon() {
  arduboy.setCursor(1, 10);
  arduboy.print("A winner is you!");

  arduboy.setCursor(1, 50);
  arduboy.print("Press a button!");
  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    game_state = GAME_STATE_INTRO;
  }
  arduboy.display(CLEAR_BUFFER);
}

inline void renderStars() {
  arduboy.drawPixel(4, 4);
  arduboy.drawPixel(2, 19);
  arduboy.drawPixel(8, 9);
  arduboy.drawPixel(13, 13);
  arduboy.drawPixel(16, 31);
  arduboy.drawPixel(21, 20);
  arduboy.drawPixel(26, 9);
  arduboy.drawPixel(30, 30);
  arduboy.drawPixel(50, 10);
  arduboy.drawPixel(52, 19);
  arduboy.drawPixel(70, 12);
  arduboy.drawPixel(90, 22);
  arduboy.drawPixel(93, 3);
  arduboy.drawPixel(110, 12);
  arduboy.drawPixel(110, 32);
}

inline void renderLives() {
  for (uint8_t i = 0; i < player_lives; i++) {
    Sprites::drawPlusMask((i * 8) + 1, 1, health, 0);
  }
}

inline void renderTrees() {
  Sprites::drawPlusMask(tree_x[0], tree_y, tree, 0);
  Sprites::drawPlusMask(tree_x[1], tree_y, tree, 0);
  Sprites::drawPlusMask(tree_x[2], tree_y, tree, 0);
  Sprites::drawPlusMask(tree_x[3], tree_y, tree, 0);
  Sprites::drawPlusMask(tree_x[4], tree_y, tree, 0);
}

inline void renderMinions() {
  for (uint8_t i = 0; i < max_critter_count(); i++) {
    if (!minion_duffed[i]) {
      Sprites::drawPlusMask(minion_x_pixels[i], player_y, minion, 0);
    }
  }
}

inline void renderSky() {
  if (is_night) {
    Sprites::drawPlusMask(115, 2, moon, 0);
  } else {
    Sprites::drawPlusMask(115, 2, sun, 0);
    Sprites::drawPlusMask(cloud_xs[0], cloud_ys[0], cloud, 0);
    Sprites::drawPlusMask(cloud_xs[1], cloud_ys[0], cloud, 0);
    Sprites::drawPlusMask(cloud_xs[2], cloud_ys[2], cloud, 0);

    uint8_t bird_frame = 1;
    if (bird_triggered) {
      if (frame_counter < 64) {
        bird_frame = 0;
      } else if (frame_counter < 128) {
        bird_frame = 1;
      } else if (frame_counter < 192) {
        bird_frame = 2;
      }
    }
    uint8_t bird_x_pixel = bird_x / BIRD_X_TO_PIXEL_MULTIPLIER;
    uint8_t bird_y_pixel = bird_y / BIRD_Y_TO_PIXEL_MULTIPLIER;
    Sprites::drawPlusMask(bird_x_pixel, bird_y_pixel, bird, bird_frame);
  }
}

inline void readInputs() {
  player_ducking = false;
  if (player_duff_cooldown == 0 && player_scurrying == false) {
    if (arduboy.justPressed(B_BUTTON) && !player_jumping) {
      beep.tone(beep.freq(440), PLAYER_DUFF_COOLDOWN_X);
      player_duff_cooldown = PLAYER_DUFF_COOLDOWN_X;
    } else {
      if (arduboy.justPressed(A_BUTTON) && !player_jumping) {
        player_jumping = true;
        bird_triggered = true;
        beep.tone(beep.freq(1000), 20);
      }

      if (player_jumping && (frame_counter % 16 == 0)) {
        if (++player_jump_frame_index >= 16) {
          player_jump_frame_index = 0;
          player_jumping = false;
          player_jump_mod = 0;
        } else {
          player_jump_mod = player_jump_pixel_offsets[player_jump_frame_index];
        }
      }

      player_ducking = arduboy.pressed(DOWN_BUTTON);

      if (arduboy.pressed(LEFT_BUTTON)) {
        player_x = player_x - (player_ducking ? 1 : 2) - (player_jumping ? 2 : 0);
        if (player_x < 4) {
          player_x = 4;
        }
        player_facing = PLAYER_SPRITE_FACING_LEFT_OFFSET;
      }
      if (arduboy.pressed(RIGHT_BUTTON)) {
        if (player_x >= 2550) {
          next_level();
        } else {
          player_x = player_x + (player_ducking ? 1 : 2) + (player_jumping ? 2 : 0);
        }
        player_facing = PLAYER_SPRITE_FACING_RIGHT_OFFSET;
      }
    }
  }
}


/**
 * Animation frame of the player sprite that should be rendered
 */
inline uint8_t getPlayerFrame() {
  uint8_t player_frame;
  if (player_duff_cooldown > 0) {
    player_frame = player_facing + 3;
  } else if (!player_jumping && player_ducking) {
    player_frame = player_facing + 2;
  } else {
    player_frame = player_facing + (frame_counter > 127 ? 0 : 1);
  }
  return player_frame;
}

/**
 * If Mitty the Kitty has been hit, she will be scurrying back to a safe place
 */
inline void scurry() {
  if (player_x <= PLAYER_X_INITIAL + 20) {  // todo magic number
    player_scurrying = false;
    player_x = PLAYER_X_INITIAL;
    player_facing = 0;
    player_jump_mod = 0;
    player_jump_frame_index = 0;
    player_jumping = false;
  } else {
    player_x -= 5;
    player_facing = 4;
    // scroll back through the jump frames
    if ((frame_counter % 16 == 0)) {
      if (player_jump_frame_index > 0) {
        player_jump_frame_index--;
        player_jump_mod = player_jump_pixel_offsets[player_jump_frame_index];
      }
    }
  }
}

inline void detectCritterCollisions(uint8_t player_x_pixel, uint8_t player_y_pixel, uint8_t bat_y_pixels[]) {
  if (is_night && !player_ducking) {
    for (uint8_t i = 0; i < max_critter_count(); i++) {
      uint8_t bat_delta_x = abs(player_x_pixel - bat_x_pixels[i]);
      uint8_t bat_delta_y = abs(player_y_pixel - bat_y_pixels[i]);

      if (bat_delta_y < 6) {
        if (bat_delta_x < 10) {
          beep.tone(beep.freq(1500), 1200);
          loseLife();
        }
      }
    }
  }

  for (uint8_t i = 0; i < max_critter_count(); i++) {
    if (minion_duffed[i]) {
      continue;
    }
    uint8_t minion_delta_x = abs(player_x_pixel - minion_x_pixels[i]);
    uint8_t minion_delta_y = abs(player_y_pixel - player_y);
    if (minion_delta_x < 8 && minion_delta_y < 8) {
      beep.tone(beep.freq(1800), 1200);
      loseLife();
    }
  }
}

inline void renderSquirrel(uint8_t squirrel_x_pixel) {
  if (squirrel_x_pixel <= 127) {
    Sprites::drawPlusMask(squirrel_x_pixel, player_y + 8, squirrel, (frame_counter > 127 ? 0 : 1));
  }
}

/**
 * Detect if mitty is duffing and if so run collision detection and duff
 * up and enemies that are duffable
 */
inline void handleDuffs(uint8_t squirrel_x_pixel, uint8_t player_x_pixel) {
  if (player_duff_cooldown == 0) {
    return;
  }
  if (player_facing == 0) {
    uint8_t delta_squirrel = squirrel_x_pixel - player_x_pixel;
    if (delta_squirrel < 15 && delta_squirrel > 0) {
      duffSquirrel();
    }
    for (uint8_t i = 0; i < max_critter_count(); i++) {
      if (!minion_duffed[i]) {
        uint8_t delta_minion = minion_x_pixels[i] - player_x_pixel;
        if (delta_minion < 15 && delta_minion > 0) {
          duffMinion(i);
        }
      }
    }

  } else {
    int delta = player_x_pixel - squirrel_x_pixel;
    if (delta < 8 && delta > -8) {
      duffSquirrel();
    }
    for (uint8_t i = 0; i < max_critter_count(); i++) {
      if (!minion_duffed[i]) {
        uint8_t delta_minion = player_x_pixel - minion_x_pixels[i];
        if (delta_minion < 13 && delta_minion > -12) {
          duffMinion(i);
        }
      }
    }
  }
  --player_duff_cooldown;
}

/**
 * Move the various critters around
 */
inline void moveCritters() {
  squirrel_x--;
  if (squirrel_x < 1) {
    squirrel_x = 120 * 30;  // TODO macro?
  }
  if (bird_triggered) {
    if (bird_y > -100) {  // todo magic numbers
      bird_x++;
      bird_y--;
      // TODO remove bird?
    }
  }
  if (is_night) {
    for (uint8_t i = 0; i < max_critter_count(); i++) {
      if (bat_flying_down[i]) {
        bat_y[i]++;
        if (bat_y[i] >= BAT_Y_MAX) {
          bat_flying_down[i] = false;
        }
      } else {
        bat_y[i]--;
        if (bat_y[i] <= BAT_Y_INITIAL) {
          bat_flying_down[i] = true;
        }
      }
    }
  }
}

/**
 * Main game loop
 * Arduino standard function
 */
void loop() {
  beep.timer();
  arduboy.pollButtons();

  if (game_state == GAME_STATE_INTRO) {
    displayScreenIntro();
    return;
  }
  if (game_state == GAME_STATE_LEVEL_TRANSITION) {
    displayScreenLevelTransition();
    return;
  }
  if (game_state == GAME_STATE_GAME_OVER) {
    displayScreenGameOver();
    return;
  }
  if (game_state == GAME_STATE_WON) {
    displayScreenGameWon();
    return;
  }

  readInputs();

  // Calculate frames and render sprites
  if (++frame_counter == 255) {
    frame_counter = 0;
  }

  moveCritters();

  // pixel position calculations - convert "hi res" c-oredinates to "pixel-res" ones
  uint8_t squirrel_x_pixel = squirrel_x / SQUIRREL_X_TO_PIXEL_MULTIPLER;
  uint8_t player_x_pixel = player_x / PLAYER_X_TO_PIXEL_MULTIPLIER;
  uint8_t player_y_pixel = player_y - player_jump_mod;
  uint8_t bat_y_pixels[] = { 0, 0, 0, 0 };
  for (uint8_t i = 0; i < max_critter_count(); i++) {
    bat_y_pixels[i] = bat_y[i] / BAT_Y_TO_PIXEL_MULTIPLER;
  }

  if (player_scurrying) {
    scurry();
  } else {
    detectCritterCollisions(player_x_pixel, player_y_pixel, bat_y_pixels);
    handleDuffs(squirrel_x_pixel, player_x_pixel);
  }

  // render the game
  arduboy.drawLine(0, 63, 127, 63);
  renderLives();
  if (is_night) {
    renderStars();
  }
  renderTrees();
  renderMinions();
  renderSky();
  renderSquirrel(squirrel_x_pixel);
  if (is_night) {
    for (uint8_t i = 0; i < max_critter_count(); i++) {
      Sprites::drawPlusMask(bat_x_pixels[i], bat_y_pixels[i], bat, (frame_counter > 127 ? 0 : 1));
    }
  }
  Sprites::drawPlusMask(player_x_pixel, player_y_pixel, player, getPlayerFrame());
  arduboy.display(CLEAR_BUFFER);
}
