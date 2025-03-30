# SULAMITH
------------

### USER MANUAL FOR VCV-RACK MODULE COLLECTION
<p style="text-align: center;"> SULAMITH IS FOCUSED ON PROVIDING VERSATILE SMALL UTILITIES & MICRO SEQUENCERS.</p>

------------

## BUTTON
![Button](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/001%20Button.jpg "Button")

Button is a multi-functional trig-driven utility module.
-  **Manual TRIG** button
- 2 **CLK/TRG/GT** inputs
- **Probability **(applied globally to all outputs)
- **TRIG** output
- **GATE** output
- **GATE length **knob (0.1s-10s)
- **A & B** outputs act as **Toggle**, **Constant Voltage** Source or **Random Voltage**
- **Constant Voltage** or **Random Voltage Range** knob (1 to 10/-1v+1v to -5v+5v/0v to 10v)
- **Mode Switch** for A & B: **Toggle, RND bipolar, RND unipolar**
- Text input for **LABELING**.
- **RETRIG** options in right-click menu*
- **SLEW** for **custom length Gates **in right-click menu **
- **SLEW** for **bi- and unipolar Random Voltages **in right-click menu **

**Buttons** can be used in many capacities:
As a **Manual** and/or **CLK/TRG/GT **driven **Trigger generator, manual Gate, Trigger to Gate **converter, **Clock randomizer** (similar to a Bernoulli Gate), **constant Voltage **source, **on/off Toggle**, **Random Voltage generator** & **random trig/gate/cv/note Sequencer**, **slewed Gates** for opening VCAs, **slewed RND** for smoother modulation.

**Inputs** 1 & 2 & custom Button work as **OR logic circuit**;
Polyphonic signals to input are converted to mono.

**Probability** goes from 0% (never) to 100% (always).
Lower probability will let fewer input triggers through.

When the **Gate Length** knob is turned all the way CCW (0 (default), Gate will go high and low analog to the inputs and probability.
Turning **Gate Length **up will set a **custom Gate. Custom Gates** are ignoring Re-Triggers and run the set duration (0.1s - 10s).

**A/B Mode 1**:
**Toggle** is a simple** A/B Switch **with a knob setting its Output Voltage (1-10v, 10v default).
Just ON/OFF or OFF/ON. Using the Voltage knob turns it into a** Constant Voltage **source or a Tool for Transposition of Sequences.

**A/B Mode 2 **(bi) + 3 (uni):
**Random** can be either bi- or unipolar and will generate two random CV signals on A and B output each. The Voltage knob sets the range. Bipolar 10 will be -5v to +5v whereas unipolar 10 is 0v to +10v.
Can be used as** CV Modulators** or **Random Note Sequencers**.  The Trig/Gate outs can trigger envelopes on changing notes, probability makes this a generative sequencer.

Text-Input for **Label** shows 6 characters. Its shoddy coding (sorry) but works and can be useful when using multiple instances of Button in your patch. i.e.: for Muting.

****ReTrig** can be enabled/disabled globally via the **context menu**. Disabling ReTrig will only apply if a custom Gate Length is set. Then: incoming Clock signals will be ignored until Gate is low again. Disabling ReTrig globally comes in handy when using custom Gates with the Random Voltages for sequencing (to sync vOct generation & Trig to Gate)*

****SLEW for Random CV** (context menu). A fixed slew amount applied for a fast Attack-Decay style envelope. Slew is slightly influenced by Range (the higher the range, the slower the slew and vice versa).*

****SLEW for Gates**  (context menu). Fixed slew amount for a fast Attack-Decay style envelope. Slew is slightly influenced by Gate length (the longer the gate, the slower the slew and vice versa).
*Gate lengths below 0.35s will go low before slew can reach 10v.Everything above will reach 10v before slewing down again.*

<p style="text-align: center;">![Slew Curves](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/002%20Button.jpg "Slew Curves")</p>


------------
##### KNOWN PROBLEMS
**Slewed Gates out of Sync with regular Gates** (see Image B):
ReTrig is off by default for custom length Gates: and that extends to slew.
A (custom) Gate can only be triggered when it is low (0v).
Decaying slew is applied to the trailing edge, extending the gates length.
Depending on the clock input: gates of the same length will go out of sync when one module uses Slew and the other doesn't (image B).

![Slewed and Unslewed Gate fix](https://github.com/JohannAsbjoernson/Sulamith/blob/main/manual/003%20Button.jpg "Slewed and Unslewed Gate fix")

To fix this just send the Slewed Gate output to an Input of the other Button module.
The OR-Logic inputs will ignore incoming Triggers as long as one input is high.
This enforces RETRIG OFF on the left Button, which might cause problems in other places (if a clock that should generate CV is ignored). Just add a third module if that should happen.

------------

##### TO-DO
	Context-Menu: set Input 2 to be a Poly CV input sent to A/B Toggle outputs
	Context-Menu: set A/B Toggle to a Bogaudio-Style temporary Switch Toggles A as long as input is high, Toggles to B as soon as input is low
	Context-Menu: add Slew to A/B Toggle (to add a crossfade effect)
	Context-Menu: Slew short / medium option
	Context-Menu: Slew all on/off
	Panel: write whats what on it
	Context-Menu: merge ON OFF options into Toggles (ON/OFF)
	Context-Menu: Add Global Settings
