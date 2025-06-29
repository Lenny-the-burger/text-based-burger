# Text based burger
This is the engine source code for the text based burger engine, used by the game CENTRALEYE. This repo is missing the game files requitred for the game, but otherwise contains all the source code for the game.

## Building
By default I reccomend using visual studio, but you should be able to build without problems with any reasonable setup. Most required libraries are included in the third party directory, with the exception of glfw and glm, these are handled by NuGet so you may have to figure that out yourself. Currently this project is set up as a visual studio solution but that is because that is what I use and no one else is working on this. Dont know why you are even reading this. I reccomend using C+\+20 or higher is avilable, but the minimum is C+\+17.

## Dependencies
This project uses several third party libraries:
- [GLFW](https://www.glfw.org/) - For glfw things. Managed by NuGet.
- [GLM](https://github.com/g-truc/glm) - Not sure. Some things in the project use it for some reason. Managed by NuGet.
- [GLAD](https://github.com/Dav1dde/glad) - More OpenGL stuff.
- [Dear ImGui](https://github.com/ocornut/imgui) - Used for debug UI. Will likely be deprecated in the future.
- [ankerl::unordered_dense](https://github.com/martinus/unordered_dense) - Fast unordered map implementation. Used mostly for storing mesh data.
- [BS::thread_pool](https://github.com/bshoshany/thread-pool) - A better implementation of a thread pool than I will ever write. This is used in performance sensitive areas so went for an existing library.

## Licensing
This program is licensed under the MIT license. You can find the license in the `LICENSE.txt` file.

Third party libraries have their own licenses, which you can find in `thid_party_licenses.txt`, but in general are all compatable with the MIT license.

The Stock font is licensed under the CC-BY-SA 4.0 license and obtained via https://github.com/susam/pcface

Content relating to the SCP Foundation, including the SCP Foundation logo, is licensed under Creative Commons Sharealike 3.0 and all concepts originate from https://scpwiki.com/ and its authors. CENTRALEYE, being derived from this content, is hereby also released under Creative Commons Sharealike 3.0.

## Notes on design conventions
### Handlers vs Controllers vs Managers
- Handlers are the weakest type of meta class, they basically just hold onto their respective subclasses and interaction with those subclasses usually bypasses them completly, and usually through another "io" class.
- Controllers are the next level up, they hold onto their respective subclasses, but also coordinate things between them above just calling constructors.
- Managers are the strongest type of meta class, and are really not even the meta class. They hold onto and manage the data directly, and all interaction involves them.

These distinctions should help you understand roughly what each class does and what patterns to expect from them.