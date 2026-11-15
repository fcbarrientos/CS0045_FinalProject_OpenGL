## Final Project in OpenGL

### Separation of Tasks

#### Person 1: Engine and rendering (lead)
* GLFW/GLAD setup, the main loop, and delta time
* Shader and texture loading (stb_image)
* Perspective camera with head sway, depth testing, and culling setup
* Mesh and model loading with Assimp, plus a Model::draw(shader, transform) function
* Scene graph: a simple list of objects, each with a model and a transform
* Project structure and build system (CMake), plus the repo setup
* Final integration

#### Person 2: Gameplay logic
* State machine: WAITING → MOVING → ON_MARK / PAST_MARK → WIN / LOSE
* Step timer and randomness, difficulty tuning, and the win and lose conditions
* Input handling (button press, restart, quit)
* The mark position along the hallway and the hit-detection window
* Interface: getCharacterProgress() (0-1), getState(), and events onStep, onWin, onLose
* Maps progress to a 3D position along the hallway path (a function progressToPosition())

#### Person 3: Lighting, effects, and UI
* Lit shader: Blinn-Phong, spotlight, point lights, and fog
* Render-to-texture (framebuffer) with depth attachment, and the post-process shader: vignette, flicker, noise, and darkness overlay
* Light behavior: flicker, flashlight sway, and lights reacting to game events
* Jumpscare visuals: screen shake, flash, and a fast camera push toward the character
* UI elements: win/lose screens and a debug overlay (text rendering with Dear ImGui or stb_easy_font)
* Stretch goal: shadow mapping
* Needs a stub from Person 1 (a framebuffer, a fullscreen quad, and a basic model draw) early on

#### Person 4: Audio and original 3D assets
* Audio system with miniaudio or OpenAL: playSound(), looping, volume, and optional 3D positional audio
* Original sound effects: footsteps, a music-box-style loop, ambient hum, and the jumpscare sting
* Original models in Blender: the toy character (low-poly, with a pose swap or two for the steps), the chair, and the hallway details (door frames, baseboards)
* Textures for all models, exported as .glb or .obj with clear scale and origin
* Placeholder assets in the first hour (box, capsule, cube), so everyone else can start
* Asset folder conventions and a license log

## Directory format
```text
CS0045_FinalProject_OpenGL/
├── CMakeLists.txt
├── README.md
├── LICENSES.md              # credits and licenses for every third-party asset
├── .gitignore
│
├── external/                # third-party libraries (vendored or submodules)
│   ├── glfw/
│   ├── glad/
│   ├── glm/
│   ├── stb/                 # stb_image.h
│   ├── assimp/              # model loading
│   ├── miniaudio/
│   └── imgui/               # optional: debug UI and text
│
├── src/
│   ├── main.cpp             # entry point: creates the window, runs the loop
│   ├── Config.h             # tunable constants (step timing, flicker, fog, FOV)
│   │
│   ├── engine/              # Person 1
│   │   ├── Window.h/.cpp
│   │   ├── Shader.h/.cpp
│   │   ├── Texture.h/.cpp
│   │   ├── Camera.h/.cpp        # perspective camera and head sway
│   │   ├── Mesh.h/.cpp
│   │   ├── Model.h/.cpp         # Assimp loading and drawing
│   │   ├── Scene.h/.cpp         # objects, transforms, and draw order
│   │   └── Time.h/.cpp
│   │
│   ├── game/                # Person 2
│   │   ├── GameState.h/.cpp     # state machine
│   │   ├── Character.h/.cpp     # progress, step logic, progressToPosition()
│   │   ├── Input.h/.cpp
│   │   └── Events.h             # onStep, onWin, onLose
│   │
│   ├── effects/             # Person 3
│   │   ├── Light.h              # directional, point, and spot light structs
│   │   ├── LightingSystem.h/.cpp
│   │   ├── Framebuffer.h/.cpp
│   │   ├── PostProcess.h/.cpp
│   │   ├── Jumpscare.h/.cpp
│   │   ├── ShadowMap.h/.cpp     # stretch goal
│   │   └── UI.h/.cpp            # win/lose screens and debug overlay
│   │
│   └── audio/               # Person 4
│       ├── AudioSystem.h/.cpp   # includes optional 3D positional audio
│       └── Sounds.h             # sound IDs and file paths
│
├── shaders/
│   ├── lit.vert
│   ├── lit.frag             # Blinn-Phong, spotlight, point lights, fog
│   ├── shadow_depth.vert    # stretch goal
│   ├── shadow_depth.frag    # stretch goal
│   ├── postprocess.vert
│   └── postprocess.frag     # vignette, noise, flicker, darkness
│
├── assets/
│   ├── models/
│   │   ├── hallway/
│   │   ├── character/
│   │   └── props/           # chair, mark
│   ├── textures/
│   │   ├── hallway/
│   │   ├── character/
│   │   ├── props/
│   │   └── ui/
│   ├── audio/
│   │   ├── sfx/             # footsteps, jumpscare sting
│   │   ├── music/           # music box loop
│   │   └── ambient/
│   └── placeholders/        # box hallway, capsule character, cube chair, temp sounds
│
├── assets_src/              # raw Blender files, layered textures, audio projects (Git LFS)
│
├── docs/
│   ├── interfaces.md        # agreed function signatures and coordinate conventions
│   ├── design.md            # character concept, game rules, tuning notes
│   ├── asset_pipeline.md    # model export rules, naming, scale, origin
│   └── workflow.md          # git rules, naming and style conventions
│
└── tests/                   # optional: unit tests for the state machine
```