# Architecture

## Overview
A first-person hallway minigame: a creepy toy creeps toward the player each time the action button is pressed. The player must stop it on a floor mark. Built with C++, freeglut, GLEW, and fixed-function OpenGL (legacy/compatibility profile). The character and all assets are original designs.

## Libraries
* freeglut: window, main loop, callbacks, bitmap-font text
* GLEW: access to buffer-object functions (VBOs)
* stb_image: PNG/JPG texture loading
* miniaudio (bonus): audio, not in the rubric
* glm (optional): vector math only, never for matrices passed to GL

## Window and main loop
* Window.h/.cpp is the only file that includes GLUT headers.
* glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH), then call glewInit() after glutCreateWindow().
* Callbacks registered: glutDisplayFunc, glutIdleFunc, glutReshapeFunc, glutKeyboardFunc, glutKeyboardUpFunc, glutMouseFunc.
* Delta time comes from glutGet(GLUT_ELAPSED_TIME). The idle callback updates game logic, then calls glutPostRedisplay().

## Input (GLUT callbacks)
* Mouse click (glutMouseFunc): the action button.
* Keyboard (glutKeyboardFunc): R = restart, ESC = quit, optional debug keys.
* Input.h/.cpp stores key/button state. game/ reads Input and never includes GLUT.

## Rendering
* 3D pass: gluPerspective + gluLookAt, depth test, backface culling.
  * Hallway: hand-built mesh with UVs, subdivided into a grid so per-vertex lighting forms a round flashlight beam. Stored in a VBO.
  * Character: simple OBJ model (or hand-built mesh), stored in a VBO.
  * Chair and floor mark: OBJ model / simple primitives.
* 2D pass (after 3D): switch to glOrtho, disable depth test and lighting, then draw
  * HUD text via glutBitmapCharacter
  * win/lose screens
  * vignette and flash overlay quads (alpha blended)
  * mark indicator
* Textures: stb_image + glTexImage2D for wallpaper, floor, and character.

## Transformations (all three used)
* Translation: the character moves down the hallway along progressToPosition().
* Rotation: camera head sway and a small character wobble.
* Scaling (glScalef): jumpscare lunge (character scales up), pulsing floor mark, and chair/prop sizing.

## Lighting
* Fixed-function: glLightfv with GL_SPOT_CUTOFF for the flashlight, point lights with attenuation, and dim ambient.
* glFog (GL_EXP2) for distance darkness.
* Flicker, flashlight sway, step dip, and blackout are driven by lighting parameters over time.
* Shadow mapping and framebuffer post-processing are dropped.

## Game state
* State machine: WAITING -> MOVING -> ON_MARK / PAST_MARK -> WIN / LOSE
* Step timer with randomness, a hit window around the mark, and progress as a 0-1 value.
* Pure logic: no GLUT, GL, or rendering includes.
* Interface: getState(), getCharacterProgress(), progressToPosition(), getMarkPulse(), and events onStep, onWin, onLose.

## Coordinates and units
* Right-handed, Y up, meters. Hallway runs along -Z. Progress is normalized 0-1.

## Audio (bonus)
* miniaudio for footsteps, music-box loop, ambient hum, and jumpscare sting, triggered by game events. Works the same under GLUT.

## Assets
* Original models exported as .obj + .mtl with textures (not .glb). Clear scale, origin at the base, triangulated.
* Placeholders first: box hallway, capsule character, cube chair.
* License log for every third-party asset.

## Requirements traceability
| # | Requirement | Where it is met |
|---|---|---|
| 1 | 3+ primitives/objects | Hallway, chair, character, floor mark |
| 2 | GLUT keyboard and mouse | Mouse = action, keyboard = restart/quit |
| 3 | 2D and 3D elements | 3D scene + glOrtho HUD/overlays |
| 4 | Translation, rotation, scaling | Character path, head sway, jumpscare/pulse/props scaling |
| 5 | Lighting | glLight spotlight and point lights, glFog |
| 6 | Image texture | Wallpaper, floor, and character via stb_image |
| 7 | VBO | Hallway and character meshes |

## Open questions for the professor
* Is the horror hallway concept approved? - Pwede na yan kay Sir (Cheska)
* Are GLSL shaders allowed with freeglut + GLEW? (This plan assumes no.)
* Are stb_image, GLEW, and miniaudio acceptable? Is audio expected? - Alam ko neutral naman diyan si Sir (Cheska)