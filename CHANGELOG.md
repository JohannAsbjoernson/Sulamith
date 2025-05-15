#Sulamith Modules: Changelog

##version 2.1.0
- added Antifa Blank
- added Knobs - Const. volt source and Sequencer
- added Volt - Poly-Multi Voltage Meter (based on ComputerScare Debug)
- added Split and Merge
##Button:
- added random::uniform / random::normal option to context-menu

##version 2.0.9
##Button:
- updated context menu
- updated text-box (auto-naming button, font, readability)
- overhauled design with custom knobs and buttons
- added dedicated knob for constant voltage/range for B output
- updated mode switch (Toggle/RND CV)
- reworked library description

##version 2.0.2
##Button:
- fixed tag and description in plugin.json

##version 2.0.1
##Button:
- added more slew variations, decreased decay
- adjusted slew for fast gates (before it wouldn't reach 10v when shorter than 0.35s)
- set min gate length to 0.2

##version 2.0.0
##Button:
- added Slew for Gate and CV in context menu
- Custom Gate ReTrig blocks are extending to slew now
- added global ReTrig OFF to context menu for syncing custom Gates with the rest of the module