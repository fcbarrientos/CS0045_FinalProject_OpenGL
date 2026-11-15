# CS0045_FinalProject_OpenGL

## Separation of tasks

#### Person 1: Engine and rendering (lead)
* GLFW/GLAD setup, the main loop, and delta time
* Sprite/quad renderer: shader loading, texture loading (stb_image), orthographic camera, and a drawSprite(texture, position, scale, rotation, alpha) function
* The depth-fake system: move and scale the sprite along a path based on a 0-1 “distance” value
* Project structure and build system (CMake), plus the repo setup
Final integration

#### Person 2: Gameplay logic
* State machine: WAITING → MOVING → ON_MARK / PAST_MARK → WIN / LOSE
* Step timer and randomness, difficulty tuning, and the win and lose conditions
* Input handling (button press, restart, quit)
* The “mark” position and the hit-detection window
* Expose a simple interface, such as getCharacterProgress(), getState(), and events like onStep, onWin, and onLose, so the others can hook into it

#### Person 3: Visual effects and atmosphere
* Post-processing shader: vignette, flicker, film noise, darkness overlay
* Render-to-texture (framebuffer) setup so effects apply to the whole scene
* Jumpscare visuals: screen shake, flash, and a fast zoom
* UI elements: the mark indicator, the win/lose screen, and a debug overlay
* Needs a stub from Person 1 (a framebuffer and a fullscreen quad) early on

#### Person 4: Audio and assets
* Audio system with miniaudio or OpenAL: playSound(), looping, volume, and positional fading if wanted
* Original sound effects: footsteps, a music-box-style loop, ambient hum, and the jumpscare sting
* Original art: hallway background, chair, the creepy toy character (sprite sheet or low-poly model), and the mark texture
* Asset folder conventions and naming, plus a license log for anything downloaded from itch.io or elsewhere
* Placeholder assets in the first hour, so everyone else can start