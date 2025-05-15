# SULAMITH
### USER MANUAL FOR VCV-RACK MODULE COLLECTION
<p style="text-align: center;"> SULAMITH IS FOCUSED ON PROVIDING VERSATILE SMALL UTILITIES & MICRO SEQUENCERS.</p>

------------
### TABLE OF CONTENT:
	- [BUTTON](#button)
	- [KNOBS](#knobs)
	- [VOLT](#volt)
	- [MERGE \& SPLIT](#merge--split)
	- [BLANK](#blank)
	- [TO-DO](#to-do)

![Sulamith](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/SulamithOverview.jpg "Sulamith")

## BUTTON
![Button](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/001%20Button.jpg "Button")

(NOTE: Button manual is out of date, changes will be incorporated sometime soon)

**Button is a multi-functional trig-driven utility module.**
-  **Manual TRIG** button
- 2 **CLK/TRG/GT** inputs
- **Probability** (applied globally to all outputs)
- **TRIG** output
- **GATE** output
- **GATE length** knob (0.1s-10s)
- **A & B** outputs act as **Toggle**, **Constant Voltage** Source or **Random Voltage**
- **Constant Voltage** or **Random Voltage Range** knob (1 to 10/-1v+1v to -5v+5v/0v to 10v)
- **Mode Switch** for A & B: **Toggle, RND bipolar, RND unipolar**
- Text input for **LABELING**.
- **RETRIG** options in right-click menu*
- **SLEW** for **custom length Gates** in right-click menu**
- **SLEW** for **bi- and unipolar Random Voltages** in right-click menu**

**Buttons** can be used in many capacities:
As a **Manual** and/or **CLK/TRG/GT** driven **Trigger generator, manual Gate, Trigger to Gate** converter, **Clock randomizer** (similar to a Bernoulli Gate), **constant Voltage** source, **on/off Toggle**, **Random Voltage generator** & **random trig/gate/cv/note Sequencer**, **slewed Gates** for opening VCAs, **slewed RND** for smoother modulation.

**Inputs** 1 & 2 & button work as **OR logic circuit**;
Polyphonic signals to input are converted to mono.

**Probability** goes from 0% (never) to 100% (always).
Lower probability will let fewer input triggers through.

When the **Gate Length** knob is turned all the way CCW (0 (default)), Gate will go high and low analog to the inputs and probability.
Turning **Gate Length** up will set a **custom Gate. Custom Gates** are ignoring Re-Triggers and run the set duration (0.1s - 10s).

**A/B Mode 1**:
**Toggle** is a simple **A/B Switch** with a knob setting its Output Voltage (1-10v, 10v default).
Just ON/OFF or OFF/ON. Using the Voltage knob turns it into a **Constant Voltage** source or a Tool for Transposition of Sequences.

**A/B Mode 2** (bi) + 3 (uni):
**Random** can be either bi- or unipolar and will generate two random CV signals on A and B output each. The Voltage knob sets the range. Bipolar 10 will be -5v to +5v whereas unipolar 10 is 0v to +10v.
Can be used as** CV Modulators** or **Random Note Sequencers**.  The Trig/Gate outs can trigger envelopes on changing notes, probability makes this a generative sequencer.

Text-Input for **Label** shows 6 characters. Its shoddy coding (sorry) but works and can be useful when using multiple instances of Button in your patch. i.e.: for Muting.

**ReTrig** * can be enabled/disabled globally via the **context menu**. Disabling ReTrig will only apply if a custom Gate Length is set. Then: incoming Clock signals will be ignored until Gate is low again. Disabling ReTrig globally comes in handy when using custom Gates with the Random Voltages for sequencing (to sync vOct generation & Trig to Gate)

**SLEW for Random CV** ** (context menu). Fixed slew amount for a fast Attack-Release style envelope. Slew amount is tied to CV Range setting. The bigger the range the slower the Slew. Decay is slightly shorter than Attack.

**SLEW for Gates** ** (context menu).  Fixed slew amount for a fast Attack-Release style envelope. Slew is tied to Gate length. The longer the gate, the slower the slew. Decay is noticably shorter than Attack.

![Slew Curves](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/002%20Button.jpg "Slew Curves")

------------
## KNOBS
![Knobs](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/003%20Knobs.jpg "Knobs")

**Knobs is an 8 channel poly voltage source & 8 step sequencer**
-  global **Probability** knob
- **Forward/Backward/Random** sequencing mode
- **Poly Steal** - when a poly cable is plugged into reset in a button/clock trig will copy inputs to knobs (Sample & Hold style)
- **CH/Step** can be set 1-8
- **Range** can be adjusted (-10+10,-5+5, -3+3, -2+2, -1+1, 0-1, 0-2, 0-3, 0-5, 0-10)
- **8 Knobs** for poly cv or mono modulations/sequencing
- **Trigger**, **Poly** and **Sequence** outputs
- manual **Button**, **CLK/TRG/GT** and **Reset** inputs

**Knobs** is a Constant Voltage Source with added sequencing features.
**Voltage Source**:
With adjustable range, channel number, control LEDs and poly input steal.
Similar to Bogaudio Polycon but with a few more features.

**Sequencer**:
There are surprisingly few light-weight, small scale sequencers in VCV Rack, so this is supposed to fill the gap a little.
Clock & Reset, Sequencing Mode, Max Steps, Range, Probability and Step LEDs - all with Trigger, Sequence and Poly out.
Randomization (CTRL+R) only affects the Knobs.
Use Stoermelder 8Face mk2 (preset sequences) and Strip (randomization) and apply Probability to turn Knobs into a generative Sequencer.

------------
## VOLT
![Volt](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/004%20Volt.jpg "Volt")

**Volt is a polyphonic Multi-Volt-Meter**
- **Polyphonic inputs**
- **Display** for up to 16 Voltages
- **Merging output** stacks incoming poly Signals

------------
## MERGE & SPLIT
![MergeSplit](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/005%20MergeSplit.jpg "MergeSplit")

**A 16 channel Merge & 16 channel Split**
- **Compact Layout**
- **Control LEDs**
ZigZagged ports & minimalistic design in 3hp, since something like this was missing.

Note: Merge -> Context Menu offers channel selection: -1 (Auto) & 0 to 16. For now you can only automate this using Stoermelders 8Face.
Since the inputs are still monophonic at this point, the feature doesn't make much sense.

------------
## BLANK
![Blank](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/006%20Blank.jpg "Blank")

A **6hp Blank Panel** using an antifacist meme.
Found in the **antifacist logo archive** on instagram, sold as stickers and buttons via **blackmosquito** and as hardware blank panel by **tangible waves**.
[Tangible Waves Shop](https://www.tangiblewaves.com/store/p228/Blank_module_2U_%22MODULAR_ANTIFA%22.html "Tangible Waves")


------------

#### TO-DO
	Modules to add:
	Comparator/Poly Signal Splitter
	Poly 2 Mono Sequencer
	Gate Length Sequencer

	Comparator:
	Context Menu: Polyphony sorting
	Second Poly Signal in for swapping

	Knobs:
	Context-Menu: Quantisation
	Context-Menu: Replace Reset with Random input

	Button:
	Context-Menu: set A/B Toggle to a Bogaudio-Style temporary Switch Toggles A as long as input is high, Toggles to B as soon as input is low
	Context-Menu: add Slew to A/B Toggle (to add a crossfade effect)
	Context-Menu: Slew short / medium option
	Context-Menu: Slew all on/off
	Panel: write whats what on it
	Context-Menu: Add Global Settings

MORE TO COME!