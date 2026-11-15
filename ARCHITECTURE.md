* Window and context: GLFW and GLAD, with OpenGL 3.3 core.
* Rendering: draw textured quads with an orthographic camera. The hallway is a background image, and Plushtrap is a sprite. To fake depth, scale and move the sprite along a path as it gets closer.
* Game state: a small state machine with WAITING → MOVING → ON_MARK / PAST_MARK → WIN / LOSE. Add a timer and a bit of randomness to his step timing.
* Lighting and atmosphere: a fragment shader with a vignette, slight noise or flicker, and a darkness overlay.
* Input: one key or mouse click for the action button.
* Audio: miniaudio or OpenAL for footsteps, the music box, and the jumpscare.
* Assets: make our own sprites and sounds, or use placeholders. The original FNAF assets are copyrighted.