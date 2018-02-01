defaults = {}
defaults.windowTitle = 'Hermit'
defaults.tabName = 'Terminal'
defaults.encoding = 'UTF-8'
defaults.wordChars = 'AA-Za-z0-9_'
defaults.scrollbackLines = 4096
defaults.font = 'Monospace 10'
defaults.geometry = '80x24'
defaults.hideSingleTab = true
defaults.showScrollbar = true
defaults.transparency = 0.25
defaults.imageFile = '/etc/xdg/hermit/messenger-1st-image-mercury.jpg'
defaults.fillTabbar = false
defaults.topMenu = true
defaults.hideMenubar = false
defaults.allowChangingTitle = false
defaults.visibleBeep = false
defaults.audibleBeep = false
defaults.urgencyOnBeep = false
setOptions(defaults)

setKbPolicy('keysym')
