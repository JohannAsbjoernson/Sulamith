# SULAMITH
### USER MANUAL FOR VCV-RACK MODULE COLLECTION
<p style="text-align: center;"> SULAMITH IS FOCUSED ON PROVIDING VERSATILE SMALL UTILITIES & MICRO SEQUENCERS.</p>

------------
### TABLE OF CONTENTS:
- [SULAMITH](#sulamith)
		- [USER MANUAL FOR VCV-RACK MODULE COLLECTION](#user-manual-for-vcv-rack-module-collection)
		- [TABLE OF CONTENTS:](#table-of-contents)
	- [BUTTON](#button)
	- [CLOCK](#clock)
	- [KNOBS](#knobs)
	- [GTSEQ](#gtseq)
	- [P2SEQ](#p2seq)
	- [VOLT](#volt)
	- [NOTE](#note)
	- [S\&H](#sh)
	- [COMPARATOR](#comparator)
	- [MERGE \& SPLIT](#merge--split)
	- [TO-DO](#to-do)


![Sulamith](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/SulamithOverview.jpg "Sulamith")

## BUTTON
![Button](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/001%20Button.jpg "Button")

**Button is a multi-functional trig-driven utility module.**
- **Manual TRIG** button
- **CLK/TRG/GT** inputs
- **Probability** (applied globally to all outputs)
- **TRIG** output & **GATE** output
- **GATE length** knob (0 = Auto | Range: 0.2s-10s)
- **A & B** outputs act as **Toggle**, **Constant Voltage** Source or **Random Voltage**
- **A & B Constant Voltage** or **Random Voltage Range** knobs (1-10)
- **Mode Switch** for A & B: **Toggle, RND bipolar, RND unipolar**
- Text input for **LABELING**
- **RETRIG** options in right-click menu
- **SLEW** for **custom length Gates** in right-click menu
- **SLEW** for **bi- and unipolar Random Voltages** in right-click menu
- **RANDOM** mode (uniform, normal) in right-click menu

**Buttons** can be used in many capacities:
As a **Manual** and/or **CLK/TRG/GT** driven **Trigger generator, manual Gate, Trigger to Gate** converter, **Clock randomizer** (similar to a Bernoulli Gate), **constant Voltage** source, **on/off Toggle**, **Random Voltage generator** & **random trig/gate/cv/note Sequencer**, **slewed Gates** for opening VCAs, **slewed RND** for smoother modulation.

**Inputs** 1 & 2 & button work as **OR logic circuit**;
Polyphonic signals to input are converted to mono.

**Probability** goes from 0% (never) to 100% (always).
Lower probability will let fewer input triggers through.

When the **Gate Length** knob is turned all the way CCW (0 (default)), Gate will go high and low analog to the inputs and probability.
Turning **Gate Length** up will set a **custom Gate. Custom Gates** are ignoring Re-Triggers and run the set duration (0.1s - 10s).

**A/B Mode 1**:
**Toggle** is a simple **A/B Switch** (when triggered). Dedicated knobs set output Voltage for A/B when high (1-10v, 10v default).
Using the Voltage knob turns it into a **Constant Voltage** source. Mainly used for transposition of v/Oct signals.

**A/B Mode 2 and 3**:
**Random** bi- or unipolar.
generates two random CV signals on A and B output each. Dedicated knobs set the range. Bipolar 10 will be -10v to +10v.
Can be used as **CV Modulators** or **Random Note Sequencers**.
**random::uniform** (default) and **random::normal** option in context menu.
**adaptive SLEW** option in context menu. (slew intensifies at higher and shortens at lower ranges)

Text-Input for **Label** shows 6 characters. Its shoddy coding (sorry) but works and can be useful when using multiple instances of Button in your patch. i.e.: for Muting.

**ReTrig** * can be enabled/disabled globally via the **context menu**. Disabling ReTrig will only apply if a custom Gate Length is set. Then: incoming Clock signals will be ignored until Gate is low again. Disabling ReTrig globally comes in handy when using custom Gates with the Random Voltages for sequencing (to sync vOct generation & Trig to Gate)

**SLEW for Gates** ** (context menu).  Fixed slew amount for a fast Attack-Release style envelope. Slew is tied to Gate length. The longer the gate, the slower the slew. Decay is noticably shorter than Attack.

![Slew Curves](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/002%20Button.jpg "Slew Curves")

------------
## CLOCK
![Clock](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/004%20Clock.jpg "Clock")

**Clock is a Master BPM Clock with 3 Divisions/Multipliers**
- **ON/OFF** and **RESET** inputs
- **BPM** 15-300
- **Div** /32 to **Mult** x32
- Four **Clock** outputs

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
## GTSEQ
![GTSeq](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/005%20GTSeq.jpg "GTSeq")

**Gate Length Sequencer with 6 0.1-10s gates, probability and playmodes.**
- **TRIGGER** and **RESET** inputs
- **PLAYMODES** forwards, backwards and random
- **SEQUENCE LENGTH** dial (1-6)
- **STEP PROBABILITY** (0%-100%)
- 6 **GATES** from 0.1s to 10.0s

------------
## P2SEQ
![P2Seq](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/006%20P2Seq.jpg "P2Seq")

**Polyphonic Signal to Sequence. Step Probability and Probability to add random CV with range dial.**
- up to 16 **POLY CHANNELS** to mono sequence
- **CLOCK** and **RESET** inputs
- **PLAYMODES** forwards, backwards and random
- **STEP PROBABILITY** (0%-100%)
- **ADD RND CV PROBABILITY**
- **RND CV RANGE**
- **LED DISPLAY** if step is skipped, rnd cv added, 16 LED Matrix showing input channels and active step

------------

## VOLT
![Volt](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/004%20Volt.jpg "Volt")

**Volt is a polyphonic Multi-Volt-Meter**
- **Polyphonic inputs**
- **Display** for up to 16 Voltages
- **Merging output** stacks incoming poly Signals

------------

## NOTE
![Note](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/007%20Note.jpg "Note")

**Note is a polyphonic Note and Cent Display**
- **Polyphonic input**
- **Chromatic** mode quantizes signals to nearest note (to display and output)
- **Display** for up to 16 notes

------------

## S&H
![SH](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/008%20SH.jpg "SH")

**Polyphonic Sample and Hold or Random CV Generator with 3 sections. **
- **CLOCK/TRIG** and **SIGNAL** inputs
- **BUTTON** to manually trigger S&H or RNG
- **QUANTIZE** chromatic quantization (applies only to generated CV)
- **RANGE A+B** set a min/max voltage for generated CV
- Number of **CHANNELS**. Auto detects incoming Polyphony, dial can be set to custom 1-16 channels.
- **PROBABILITY** 0-100% dictates if an incoming trigger is used to sample/generate or skipped.
- **BOTTOM SECTION** copies settings of second section (quant, range, chan, prob)

------------

## COMPARATOR
![Comp](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/009%20Comp.jpg "Comp")

**Polyphonic Comparator compares input to a threshold. A outputs high when input is above, B when input is below threshold. Bottom outputs split the input signal into A: above and B: below threshold. Works at audio rate for hard limiting and splitting. Great for modulation of CV**
- **THRESHOLD** with **CV** and **ATTENUATOR**
- **A/B GATE** outputs
- **A/B CV** outputs

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

#### TO-DO
	Modules to add
	Clock Divider
	Logic
	Preset/Clone

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