ModuleLoader
===============

ModuleLoader 3.x which is implemented as a scripting addtion is deprecated.
Becasue scripting addtions don't works under macOS 10.14 or later.
For macOS 10.4 or later, ModuleLoader is reimplemented as an AppleScript library and a helpr application.
https://github.com/tkurita/ModuleLoader4

ModuleLoader is a system for managing and loading libraries(modules) of AppleScript.

In OS X 10.9, built-in support of libraries was introduced to AppleScript, which called as "AppleScript Libraries". ModuleLoader is a yet another library system, which has been developed from 2006 before release of OS X 10.9, and has been maintained without interruption until now.

ModuleLoader have a similar function to "AppleScript Libraries" as follows.
* Find a library from predefined locations and load the library as a script object.
* Libraries are searched from following sub-folders under user's home directory and the root directory.
  - Library/Scripts/Modules
  - Library/Script Libraries
* Sub-libraries required by the loaded library are automatically loaded.
* The version of a library to be loaded can be specified.

## Usage
English :
* https://www.script-factory.net/XModules/ModuleLoader/en/index.html

Japanese :
* https://www.script-factory.net/XModules/ModuleLoader/index.html

## Building
Reqirements :
* Mac OS X 10.7 or later.
* Xcode 6.3 or later.

## Licence

Copyright &copy; 2006-2016 Tetsuro Kurita
Licensed under the [GPL license][GPL].
 
[GPL]: http://www.gnu.org/licenses/gpl.html

