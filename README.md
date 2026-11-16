## Final Project in OpenGL

### Separation of Tasks

#### Person 1: Engine and rendering (lead)
* freeglut + GLEW setup, callbacks, main loop, delta time
* Camera: gluPerspective + gluLookAt, head sway applied as rotation
* Simple OBJ loader and Mesh class that uploads to a VBO (glGenBuffers, glBufferData)
* Texture loading (stb_image + glTexImage2D) and UVs on the hallway mesh
* Scene list and draw order, applying glTranslatef / glRotatef / glScalef (chair and prop sizing)
* Hallway mesh built as a subdivided grid
* CMake rewrite (freeglut, GLEW, stb_image only), repo setup, final integration
* Requirements: #1, #2 (callbacks), #3 (3D side), #4, #6, #7

#### Person 2: Gameplay logic
* State machine: WAITING -> MOVING -> ON_MARK / PAST_MARK -> WIN / LOSE
* Step timer and randomness, difficulty tuning, hit window, mark position
* Input class fed by the glutKeyboardFunc / glutMouseFunc callbacks: mouse = action, R = restart, ESC = quit
* Interface: getState(), getCharacterProgress(), progressToPosition(), onStep, onWin, onLose
* getMarkPulse(): a 0-1 value that Person 1 and 3 turn into scaling of the floor mark
* Requirements: #2 (input hookup), #4 (scaling values)

#### Person 3: Lighting, effects, 2D UI
* Lighting: glLightfv spotlight (GL_SPOT_CUTOFF), point lights with attenuation, ambient, glFog
* Light behavior: flicker, flashlight sway, step dip, blackout
* 2D layer: glOrtho setup, HUD text (glutBitmapCharacter), win/lose screens, mark indicator
* Overlays: vignette quads, flash, and camera jitter
* Jumpscare: timing and logic, plus the lunge scaling (glScalef)
* Dropped: shaders, framebuffer, shadow mapping, ImGui
* Requirements: #3 (2D side), #4 (jumpscare scaling), #5

#### Person 4: Audio and original assets
* Audio (bonus): miniaudio, playSound(), looping, volume
* Sounds: footsteps, music-box loop, ambient hum, jumpscare sting
* Original models in Blender: character, chair, hallway details
* Export as .obj + .mtl with textures (not .glb), with clear scale and origin
* Textures: wallpaper, floor, character, chair, mark
* Placeholders in the first hour, license log, asset naming conventions
* Requirements: #1, #6 (assets)

#### Things needed to agree on
1. Folder structure and naming.
2. Interface headers written before anyone codes behind them.
3. Coordinates: right-handed, Y up, meters, hallway along -Z.
4. OBJ export rules (scale, origin, triangulated, .mtl paths).
5. Git workflow: one branch per person, small pull requests, Person 1 merges.
6. Get answers to the professor questions as early as possible.

#### Timeline
* Phase 1 (setup): P1 gets a window with a lit, textured, rotating cube drawn from a VBO. P2 builds the state machine as a console prototype. P3 tests the spotlight, fog, and a glOrtho text overlay on test geometry. P4 delivers placeholder assets and sounds.
* Phase 2 (integration): connect game state to the character transform, add lighting and overlays, trigger audio from events.
* Phase 3 (polish): swap in final assets, tune timing and lighting, use scaling for the jumpscare, pulse, and props, and fix bugs.

#### Requirements checklist (from the professor's brief)
- [ ] 1. At least 3 primitives/objects (P1, P4)
- [ ] 2. GLUT keyboard and mouse callbacks (P1, P2)
- [ ] 3. 2D and 3D elements (P1, P3)
- [ ] 4. Translation, rotation, scaling (P1, P2, P3)
- [ ] 5. Lighting (P3)
- [ ] 6. Image texture (P1, P4)
- [ ] 7. VBO for at least one object (P1)## Directory format

### Directory format

```text
project-name/
├── CMakeLists.txt           # freeglut, GLEW, stb_image (+ miniaudio)
├── README.md
├── ARCHITECTURE.md
├── LICENSES.md              # credits and licenses for every third-party asset
├── .gitignore
│
├── external/
│   ├── freeglut/
│   ├── glew/
│   ├── stb/                 # stb_image.h
│   ├── glm/                 # optional: vector math only
│   └── miniaudio/           # bonus
│
├── src/
│   ├── main.cpp             # entry point: init, register callbacks, glutMainLoop()
│   ├── Config.h             # tunable constants (step timing, flicker, fog, FOV)
│   │
│   ├── engine/              # Person 1
│   │   ├── Window.h/.cpp        # only file that includes GLUT; callback wiring
│   │   ├── Time.h/.cpp          # wraps glutGet(GLUT_ELAPSED_TIME)
│   │   ├── Camera.h/.cpp        # gluPerspective, gluLookAt, head sway
│   │   ├── Texture.h/.cpp       # stb_image + glTexImage2D
│   │   ├── ObjLoader.h/.cpp     # simple .obj/.mtl loader
│   │   ├── Mesh.h/.cpp          # VBO upload and draw
│   │   ├── HallwayBuilder.h/.cpp  # subdivided hallway grid with UVs
│   │   └── Scene.h/.cpp         # objects, transforms (translate/rotate/scale), draw order
│   │
│   ├── game/                # Person 2 (no GLUT or GL includes)
│   │   ├── GameState.h/.cpp     # state machine
│   │   ├── Character.h/.cpp     # progress, step logic, progressToPosition()
│   │   ├── Input.h/.cpp         # key/mouse state fed by GLUT callbacks
│   │   └── Events.h             # onStep, onWin, onLose
│   │
│   ├── effects/             # Person 3
│   │   ├── Lighting.h/.cpp      # glLight spotlight, point lights, glFog
│   │   ├── LightBehavior.h/.cpp # flicker, flashlight sway, step dip, blackout
│   │   ├── Overlay2D.h/.cpp     # glOrtho layer: vignette, flash, screens
│   │   ├── Text.h/.cpp          # glutBitmapCharacter helper
│   │   ├── Jumpscare.h/.cpp     # timing, lunge scaling, camera jitter
│   │   └── UI.h/.cpp            # HUD, mark indicator, win/lose screens
│   │
│   └── audio/               # Person 4 (bonus)
│       ├── AudioSystem.h/.cpp
│       └── Sounds.h             # sound IDs and file paths
│
├── assets/
│   ├── models/              # .obj + .mtl
│   │   ├── hallway/
│   │   ├── character/
│   │   └── props/           # chair, mark
│   ├── textures/
│   │   ├── hallway/         # wallpaper, floor
│   │   ├── character/
│   │   ├── props/
│   │   └── ui/
│   ├── audio/
│   │   ├── sfx/
│   │   ├── music/
│   │   └── ambient/
│   └── placeholders/        # box hallway, capsule character, cube chair, temp sounds
│
├── assets_src/              # raw Blender files, layered textures, audio projects (Git LFS)
│
├── docs/
│   ├── interfaces.md        # agreed function signatures and coordinate conventions
│   ├── design.md            # character concept, game rules, tuning notes
│   ├── asset_pipeline.md    # OBJ export rules, naming, scale, origin
│   ├── professor_questions.md  # open questions and answers
│   └── workflow.md          # git rules, naming and style conventions
│
└── tests/                   # optional: unit tests for the state machine
```