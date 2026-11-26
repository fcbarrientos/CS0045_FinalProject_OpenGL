# Ghost Ward (OpenGL / freeglut)
Build: `cmake -B build && cmake --build build && ./build/hallway`
Or with g++: `g++ main.cpp -I. -o main.exe -lfreeglut -lopengl32 -lglu32 -lglew32`

Controls:
- Hold Left Mouse Button: Move the interactive line right
- Release Left Mouse Button: Move the interactive line left
- Keep the interactive line inside the moving green mark to fill the progress bar to 100% before the ghost reaches you at 60 seconds!
- R: Restart
- ESC: Quit
