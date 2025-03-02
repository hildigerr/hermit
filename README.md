Termit
======

Simple terminal emulator forked from [nonstop/termit](https://github.com/nonstop/termit).

## Fork and Customization Rationale

 The primary motivation for this fork is to provide users with a configurable option to position the application's menu bar at the top of the window, aligning with established Human-Computer Interaction (HCI) principles and common application layout conventions.

**Background:**

A pull request (#118: [topMenu option to place menu bar at top of window](https://github.com/nonstop/termit/pull/118)) was submitted to the upstream repository to introduce a `topMenu` option. The intent was to enhance usability by allowing users to arrange the menu bar in a familiar and predictable location, adhering to the principle of **consistency** in user interface design.

The upstream author responded with a patch that moved both the menu and status bar to the top, renaming the option to `topMenubar`. This deviated from the intended design of maintaining the status bar at the bottom, consistent with typical application layouts. Despite my request to maintain the status bar's default location and a suggestion for a more flexible `menuPos` option, no further response was received.

**HCI Principles and User Expectations:**

* **Consistency:** Users expect menu bars to be located at the top of application windows, following established conventions across operating systems and applications. This consistency reduces cognitive load and improves usability.
* **Mental Model:** Placing the menu bar at the top aligns with users' mental models of application structure, making it easier to find and access commands.
* **Separation of Concerns:** Maintaining the status bar at the bottom allows for clear separation of concerns, providing status information without interfering with menu navigation.

**Implementation in this Fork:**

This fork retains the content of the original pull request #118, providing the `topMenu` option to position the menu bar at the top while retaining the status bar at the bottom. This adheres to the principles of consistency and separation of concerns, providing a more user-friendly interface.

By providing this configurable option, this fork aims to enhance the usability and user experience of Termit, making it more adaptable to individual preferences and established HCI best practices.

## Features:

* Configurable menu bar placement (top or bottom)
* multiple tabs
* switching encodings
* sessions
* configurable keybindings
* embedded Lua

Configuration can be changed via $HOME/.config/termit/rc.lua file.
Example configurations can be found in doc/rc.lua.example.

Command-line arguments:

    termit --help
    Options:
      -h, --help             - print this help message
      -v, --version          - print version number
      -e, --execute          - execute command
      -i, --init=init_file   - use init_file instead of standard rc.lua
      -n, --name=name        - set window name hint
      -c, --class=class      - set window class hint
      -r, --role=role        - set window role (Gtk hint)
      -T, --title=title      - set window title

Keybindings can be configured via rc.lua.
Defaults are:

    Alt-Left     -  previous tab
    Alt-Right    -  next tab
    Ctrl-t       -  open tab
    Ctlr-w       -  close tab
    Ctrl-Insert  -  copy
    Shift-Insert -  paste

Lua API is described in doc/lua_api.md or in man page.

