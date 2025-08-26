#include "plugin.hpp"

#include <string>
#include <sstream>
#include <iomanip>

// This Module is based off of NoteMeter by Chinenual ( https://github.com/chinenual/Chinenual-VCV )

#define NUM_CHANNELS 16

//
inline int voltageToPitch(float v)
{
    // based on VCV CORE CV_MIDI:
    return (int)std::round(v * 12.f + 60.f);
}

inline float voltageToMicroPitch(float v)
{
    // based on VCV CORE CV_MIDI:
    return (v * 12.f + 60.f);
}

/* Careful: notesDeviation is scaled by notes value - not by voltage */
inline void pitchToText(std::string &text, int notes, float notesDeviation)
{
    // warning: notes is not just in the MIDI range - might be much lower (-10v in v/oct == -60 "notes")

    // in case notesDeviation is larger than a semitone, apply it first and compute a new "smaller than semitone" deviation:
    int n = std::round(notes + notesDeviation);
    float nDeviation = notesDeviation - (n - notes);

    const char *notesNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    // ensure nameIndex is positive -- add several octaves to ensure we the numerator is > 0.  n could be as low
    // as -60.   But just in case we feed this something way out of range, add even more.  (Bogaudio LLFO seems to somehow be abnle to send +/- 12V?)
    int nameIndex = (1200 + n) % 12;

    int octave = (n / 12) - 1; // simple division produces 60=C5; we want to display that as C4, so subtract 1
    auto absDeviation = std::abs(nDeviation);
    if (absDeviation >= 0.01f)
    {
        text = rack::string::f("%s%d%s%d", notesNames[nameIndex], octave,
                               (nDeviation > 0 ? "+" : "-"),
                               (int)(absDeviation * 100));
    }

    else
    {
        text = rack::string::f("%s%d", notesNames[nameIndex], octave);
    }
}
struct Note : Module
{
    enum ParamIds
    {
        QUANT_SWITCH,
        NUM_PARAMS
    };
    enum InputIds
    {
        POLY_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        POLY_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        QUANT_LED,
        NUM_LIGHTS
    };

    Note()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(POLY_INPUT, "Polyphonic vOct Signal");
        configSwitch(QUANT_SWITCH, 0, 1, 0, "Chromatic Mode", {"Off", "On"});
        configOutput(POLY_OUTPUT, "Polyphonic vOct (Chromatic when turned on)");

        configBypass(POLY_INPUT, POLY_OUTPUT);
    }

    // a string (variable that contains letters) array called text. the array is declared using name[NUMBER]. in this instance we have 16 variables named text + 1, text +2 etc.
    std::string text[16];

    void process(const ProcessArgs &args) override
    {
        // args.frame % 100 reduces the times this function is called. % calls the modulus function, % 100 will only activate every 100 frames.
        if ((args.frame % 100) == 0)
        {
            int count = 0;
            auto chnr = inputs[POLY_INPUT].getChannels();
            for (int i = 0; i < 16; i++)
            {
                text[i] = "";
            }

            outputs[POLY_OUTPUT].setChannels(inputs[POLY_INPUT].getChannels());

            if (inputs[POLY_INPUT].isConnected())
            {
                for (int c = 0; c < chnr; c++)
                {
                    auto volts = inputs[POLY_INPUT].getVoltage(c);

                    if (params[QUANT_SWITCH].getValue() == 1)
                    {
                        auto voct = floor(volts);
                        auto vsem = ((volts - voct) / 0.083333f);
                        volts = ((round(vsem)) * 0.083333f) + voct;
                    }

                    // we clamp inputs to +/-10V. use voltageToPitch (1v/oct, i.e. 1v = C5) & voltageToMicroPitch (1v/1200c, i.e. 0.5v = 600cent) and the auto variable to get pitch information from cv.
                    // auto can be used in place of setting a variable type (like int, float, bool) and will be interpreted by the compiler/engine as fits when it is called.
                    // I use auto here because we convert voltages to pitch and pitch to text. just let the machine decide how to work with that.
                    auto in_v = clamp(volts, -10.f, 10.f);
                    auto n = voltageToPitch(in_v);
                    auto fn = voltageToMicroPitch(in_v);

                    // pitchToText writes the pitch and micropitch to our text array.
                    // n is our Pitch (1v/oct) and fn our MicroPitch (1v/1200cent). we fn - ((float)n) to get only the "excess" of voltage in cents that goes beyond the 1v/oct.
                    pitchToText(text[count], n, fn - ((float)n));

                    outputs[POLY_OUTPUT].setVoltage(volts, c);

                    count++;
                    if (count >= 16)
                    {
                        count = 0;
                        break; // inner loop
                    }
                }
            }
            float lit = params[QUANT_SWITCH].getValue() ? 1.f : 0.f;
            lights[QUANT_LED].setBrightnessSmooth(lit, args.sampleTime);
        }
    }
};

struct NoteWidget : ModuleWidget
{
    NoteWidget(Note *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        for (int ch = 0; ch < 16; ch++)
        {
            auto textDisplay = new TextDisplayWidget(module ? &module->text[ch] : NULL);
            textDisplay->box.size = Vec(40, 10);
            textDisplay->box.pos = Vec(1, 24 + (15 * ch));
            addChild(textDisplay);
        }
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(23, 290), module, Note::QUANT_SWITCH, Note::QUANT_LED));
        addInput(createInputCentered<PJ301Mvar2>(Vec(23, 320), module, Note::POLY_INPUT));
        addOutput(createOutputCentered<ThemedPJ301MPort>(Vec(23, 349.5), module, Note::POLY_OUTPUT));

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(11.5, 5);
        titleLabel->text = "NOTE";
        addChild(titleLabel);
    }
};

Model *modelNote = createModel<Note, NoteWidget>("Note");