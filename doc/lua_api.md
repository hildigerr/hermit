h4. Functions

*activateTab(_tab_index_)* - Activates tab by index.
_tab_index_ - index of tab to activate. Index of the first tab is 1.


*addMenu(_menu_)* - Adds menu in menubar.
_menu_ - table of TermitMenuItem type.


*addPopupMenu(_menu_)* - Adds menu in popup menu, similar to addMenu.
_menu_ - table of TermitMenuItem type.


*bindKey(_keys_, _luaFunction_)* - Sets new keybinding. If luaFunction is nil removes keybinding.
_keys_ - string with keybinding. Available modifiers are Alt Ctrl Shift Meta Super Hyper.
_luaFunction_ - callback function

pre. Example: don't close tab with Ctrl-w, use CtrlShift-w
    bindKey('Ctrl-w', nil)
    bindKey('CtrlShift-w', closeTab)


*bindMouse(_event_, _luaFunction_)* - Sets new mouse-binding. If luaFunction is nil removes mouse-binding.
_event_ - string with one of those values: DoubleClick
_luaFunction_ - callback function


*closeTab()* - Closes active tab.


*copy()* - Copies selection into tab's buffer.


*currentTab()* - Returns current tab of TermitTabInfo.


*currentTabIndex()* - Returns current tab index.


*feed(data)* - Interprets data as if it were data received from a terminal process.


*feedChild(data)* - Sends a data to the terminal as if it were data entered by the user at the keyboard. 


*findDlg()* - Opens find entry. (Enabled when build with vte version >= 0.26)


*findNext()* - Searches the next string matching search regex.


*findPrev()* - Searches the previous string matching search regex.


*forEachRow(_func_)* - For each terminal row in entire scrollback buffer executes function passing row as argument.
_func_ - function to be called.


*forEachVisibleRow(_func_)* - For each visible terminal row executes function passing row as argument.
_func_ - function to be called.


*loadSessionDlg()* - Displays "Load session" dialog.


*nextTab()* - Activates next tab.


*openTab(_tabInfo_)* - Opens new tab.
_tabinfo_ - table of TermitTabInfo with tab settings.

pre. Example:
    tabInfo = {}
    tabInfo.title = 'Zsh tab'
    tabInfo.command = 'zsh'
    tabInfo.encoding = 'UTF-8'
    tabInfo.workingDir = '/tmp'
    openTab(tabInfo)


*paste()* - Pastes tab's buffer.


*preferencesDlg()* - Displays "Preferences" dialog.


*prevTab()* - Activates previous tab.


*quit()* - Quit.


*reconfigure()* - Sets all configurable variables to defaults and forces rereading rc.lua.


*saveSessionDlg()* - Displays "Save session" dialog.


*selection()* - Returns selected text from current tab.


*setColormap(_colormap_)* - Changes colormap for active tab.
_colormap_ - array with 8 or 16 or 24 colors.


*setEncoding(_encoding_)* - Changes encoding for active tab.
_encoding_ - string with encoding name.

pre. Example:
    setEncoding('UTF-8')


*setKbPolicy(_kb_policy_)* - Sets keyuboard policy for shortcuts.
_kb_policy_ - string with one of those values:

* keycode - use hardware keyboard codes (XkbLayout-independent)
* keysym - use keysym values (XkbLayout-dependent)

pre. Example: treat keys via current XkbLayout
    setKbPolicy('keysym')

*setOptions(_opts_)* - Changes default options.
_opts_ - table of TermitOptions type with new options:


*setTabBackgroundColor(_color_)* - Changes background color for active tab.
_color_ - string with new color.


*setTabFont(_font_)* - Changes font for active tab.
_font_ - string with new font.


*setTabForegroundColor(_color_)* - Changes foreground (e.g. font) color for active tab.
_color_ - string with new color.


*setTabTitle(_tabTitle_)* - Changes title for active tab.
_tabTitle_ - string with new tab title.


*setTabTitleDlg()* - Displays "Set tab title" dialog.


*setWindowTitle(_title_)* - Changes termit window title.
_title_ - string with new title.


*spawn(_command_) - Spawns command (works via shell).
_command_ - string with command and arguments.

*toggleMenubar()* - Displays or hides menubar.

*toggleTabbar()* - Displays or hides tabbar.

h4. Types

*TermitCursorBlinkMode* - one of those string values:

* System - Follow GTK+ settings for cursor blinking
* BlinkOn - Cursor blinks
* BlinkOff - Cursor does not blink

*TermitCursorShape* - one of those string values:

* Block - Draw a block cursor
* Ibeam - Draw a vertical bar on the left side of character
* Underline - Draw a horizontal bar below the character

*TermitEraseBinding* - one of those string values:

* Auto - VTE_ERASE_AUTO
* AsciiBksp - VTE_ERASE_ASCII_BACKSPACE
* AsciiDel - VTE_ERASE_ASCII_DELETE
* EraseDel - VTE_ERASE_DELETE_SEQUENCE
* EraseTty - VTE_ERASE_TTY

For detailed description look into Vte docs.

*TermitKeybindings* - table with predefined keybindings.

* closeTab - 'Ctrl-w'
* copy - 'Ctrl-Insert'
* nextTab - 'Alt-Right'
* openTab - 'Ctrl-t'
* paste - 'Shift-Insert'
* prevTab - 'Alt-Left'

pre. Example: enable Gnome-like tab switching
    keys = {}
    keys.nextTab = 'Ctrl-Page_Down'
    keys.prevTab = 'Ctrl-Page_Up'
    setKeys(keys)


*TermitMenuItem* - table for menuitems.

* accel - accelerator for menuitem. String with keybinding
* action - lua-function to execute when item activated
* name - name for menuitem


*TermitOptions* - table with termit options.

* allowChangingTitle - auto change title (similar to xterm)
* audibleBell - enables audible bell
* backgroundColor - background color
* backspaceBinding - reaction on backspace key (one of TermitEraseBinding)
* colormap - array with 8 or 16 or 24 colors
* cursorBlinkMode - cursor blink mode (one of TermitCursorBlinkMode)
* cursorShape - cursor shape (one of TermitCursorShape)
* deleteBinding - reaction on delete key (one of TermitEraseBinding)
* encoding - default encoding
* fillTabbar - expand tabs' titles to fill whole tabbar
* font - font name
* foregroundColor - foreground color
* geometry - cols x rows to start with
* getTabTitle - lua function to generate new tab title
* getWindowTitle - lua function to generate new window title
* hideMenubar - hide menubar
* hideTabbar - hide tabbar
* hideSingleTab - hide tabbar when only 1 tab present
* hideTitlebarWhenMaximized - hide window titlebar when mazimized
* matches - table with items of TermitMatch type
* scrollbackLines - the length of scrollback buffer
* scrollOnKeystroke - enables scroll to the bottom when the user presses a key
* scrollOnOutput - enables scroll to the bottom when new data is received
* setStatusbar - lua function to generate new statusbar message
* showBorder - show notebook border
* showScrollbar - display scrollbar
* startMaximized - maximize window on start
* tabCloseButton - add a close button to each tab
* tabName - default tab name
* tabPos - tabbar position (Top, Bottom, Left, Right)
* tabs - table with items of TermitTabInfo type
* topMenu - place the menubar at the top of the window
* urgencyOnBell - set WM-hint 'urgent' on termit window when bell
* wordCharExceptions - additional word characters (double click selects word)


*TermitTabInfo* - table with tab settings:

* command
* encoding
* font - font string
* fontSize - font size
* pid - process id
* title
* workingDir


h4. Globals

*tabs* - Readonly table with tab settings, access specific tabs by index.


h4. Bugs

After start sometimes there is black screen. Resizing termit window helps.

In options table 'tabs' field should be the last one. When loading all settings are applied in the same order as they are set in options table. So if you set tabs and only then colormap, these tabs would have default colormap.


