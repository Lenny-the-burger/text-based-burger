# Text based burger
This is the engine source code for the text based burger engine, used by the game CENTRALEYE. This repo is missing the game files requitred for the game, but otherwise contains all the source code for the game.

## Liscencing
This program is licensed under the MIT license. You can find the license in the LICENSE file.

Third party libraries are licensed under their respective licenses.

The Stock font is liscenced under the CC-BY-SA 4.0 license and obtained via https://github.com/susam/pcface

## Notes on design conventions
### Handlers vs Controllers vs Managers
- Handlers are the weakest type of meta class, they basically just hold onto their respective subclasses and interaction with those subclasses usually bypasses them completly, and usually through another "io" class.
- Controllers are the next level up, they hold onto their respective subclasses, but also coordinate things between them above just calling constructors.
- Managers are the strongest type of meta class, and are really not even the meta class. They hold onto and manage the data directly, and all interaction involves them.

These distinctions should help you understand roughly what each class does and what patterns to expect from them.