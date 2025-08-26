#include "plugin.hpp"

struct KnobX : Module
{
    enum ParamIds
    {
        ENUMS(CV_KNOBS, 8),
        CV_STEP,
        CV_RANGE,
        CV_MODE,
        CV_BT,
        CV_RND,
        NUM_PARAMS
    };
    enum InputIds
    {
        CV_CLK,
        CV_RES,
        NUM_INPUTS
    };
    enum OutputIds
    {
        CV_TRIG,
        CV_POLY,
        CV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        ENUMS(CV_LEDS, 8 * 2), // Bi-color LEDs have to be called twice, to drive 8 we need to declare 16. Same for setting their color and brightness.
        ENUMS(STEP_LEDS, 8),   // single color LEDs just need to be called once
        TRIG_LED,
        ENUMS(SEQ_LED, 1 * 2),
        CV_BTLED,
        NUM_LIGHTS
    };

    dsp::ClockDivider lightDivider;
    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger clockTrigger2;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger buttonTrigger;
    dsp::PulseGenerator outputTrigger;

    int steps = 0;
    float cvled = 0.f;
    float rangeknobold = -1.f;
    const float rangequantA[10] = {-10.f, -5.f, -3.f, -2.f, -1.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    const float rangequantB[10] = {10.f, 5.f, 3.f, 2.f, 1.f, 1.f, 2.f, 3.f, 5.f, 10.f};

    void onReset() override
    {
        doReset();
    }

    void doReset()
    {
        steps = 0;
    }

    KnobX()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 8; i++)
            configParam(CV_KNOBS + i, -10.f, 10.f, 0.f, string::f("CV %d", i + 1), "v");

        configParam(CV_BT, 0, 1, 0, "Trigger Button");
        configParam(CV_STEP, 1, 8, 8, "Sequence Size", " Steps/CHs");
        configSwitch(CV_RANGE, 0, 9, 1, "Range", {"-10v to +10v", "-5v to +5v", "-3v to +3v", "-2v to +2v", "-1v to +1v", "0v to 1v", "0v to 2v", "0v to 3v", "0v to 5v", "0v to 10v"});
        configSwitch(CV_MODE, 0, 2, 0, "Sequence Mode", {"Forwards", "Backwards", "Random"});
        configParam(CV_RND, 0, 1, 1, "Probability", "%", 0, 100);

        getParamQuantity(CV_STEP)->snapEnabled = true;
        getParamQuantity(CV_RANGE)->snapEnabled = true;
        getParamQuantity(CV_MODE)->snapEnabled = true;

        // set so that only the knobs will be randomized
        getParamQuantity(CV_STEP)->randomizeEnabled = false;
        getParamQuantity(CV_BT)->randomizeEnabled = false;
        getParamQuantity(CV_MODE)->randomizeEnabled = false;
        getParamQuantity(CV_RANGE)->randomizeEnabled = false;
        getParamQuantity(CV_RND)->randomizeEnabled = false;

        configInput(CV_CLK, "Clock");
        configInput(CV_RES, "Reset (if mono), Clone (if poly)");

        configOutput(CV_TRIG, "Trigger");
        configOutput(CV_POLY, "Polyphonic");
        configOutput(CV_OUTPUT, "Sequencer");

        configLight(TRIG_LED, "Trigger/Step");
        configLight(SEQ_LED, "Sequence");
        configBypass(CV_CLK, CV_TRIG);

        lightDivider.setDivision(512);
    }

    void process(const ProcessArgs &args) override
    {
        // set a bunch of variables to shortcut inputs/params
        int chan = params[CV_STEP].getValue();
        float clk = inputs[CV_CLK].getVoltage(), res = inputs[CV_RES].getVoltage(), mode = params[CV_MODE].getValue(), v = 0.f;
        bool cvbt = (buttonTrigger.process(params[CV_BT].getValue() > 0.f ? true : false, 0.1, 1.0)), cvbt2 = params[CV_BT].getValue();
        bool cvres = inputs[CV_RES].isConnected(), cvresp = inputs[CV_RES].isPolyphonic();
        bool cvclkIN = (clockTrigger2.process(clk ? true : false));
        int cvresCH = inputs[CV_RES].getChannels();

        outputs[CV_POLY].setChannels(chan);

        float deltaTime = args.sampleTime * lightDivider.getDivision();

        int rangeknob = params[CV_RANGE].getValue();
        if (rangeknobold != rangeknob)
        {
            rangeknobold = params[CV_RANGE].getValue();
            for (int i = 0; i < 8; i++)
            {
                getParamQuantity(CV_KNOBS + i)->minValue = rangequantA[rangeknob];
                getParamQuantity(CV_KNOBS + i)->maxValue = rangequantB[rangeknob];
                float knobval = params[CV_KNOBS + i].getValue();
                getParamQuantity(CV_KNOBS + i)->setValue(knobval);
            }
        }

        bool outTRG = outputTrigger.process(deltaTime) ? true : false;

        // SAMPLE & HOLD: COPY A POLYPHONIC INPUT
        // if A: clock disconnected and res input, OR B: clock connected, res input is polyphonic... pressing button will apply incoming (poly) voltage to the knobs.
        // ((button AND reset input NOT clock) OR (button AND reset input polyphonic AND clock input))
        if ((cvres && cvresp && (clockTrigger.process(clk, 0.1, 1.0))) || (cvres && cvresp && cvbt))
        {
            int ch = std::max(cvresCH, 1);
            ch = clamp(ch, 1, 8); // if cable has more than 8 signals, clamp them off.
            int ct = 0;

            while (ct < ch) // keeps looping until ct is bigger than total of poly channel input
            {
                float val = inputs[CV_RES].getVoltage(ct);
                getParamQuantity(CV_KNOBS + ct)->setValue(val);
                ct++;
            }

            if (ct >= ch)
                ct = 0;
        }

        // Controls the Button Light.
        if (cvbt2 || cvclkIN)
            cvled = 1.f;
        else
            cvled = cvled - 0.00005; // easily get attack, decay, slew, morphing, drift..
        lights[CV_BTLED].setBrightnessSmooth(cvled, deltaTime);

        // LOOPS & LIGHTS
        for (int i = 0; i < 8; i++) // 8 times loop (one for each LED).
        {
            for (int c = 0; c < chan; c++) // nested loop based on sequense length. we want to turn LEDs of inactive steps OFF.
            {
                float w = params[CV_KNOBS + c].getValue();
                v = w / 10.f;
                outputs[CV_POLY].setVoltage(w, c);
                lights[STEP_LEDS + c].setBrightnessSmooth(0.2f, deltaTime); // dim lit to mark active steps

                if (i > c) // i counts total steps, c counts active sequence. where i is bigger than c all LEDs can be turned off
                {
                    lights[CV_LEDS + i * 2 + 0].setBrightnessSmooth(0.f, deltaTime);
                    lights[CV_LEDS + i * 2 + 1].setBrightnessSmooth(0.f, deltaTime);
                }
                else // simple inversion of the brightness value.. if v is positive, green lights up - if v is negative -v will turn positive. Mathematically its: -1 * -v = v.
                {
                    lights[CV_LEDS + c * 2 + 0].setBrightnessSmooth(v, deltaTime);
                    lights[CV_LEDS + c * 2 + 1].setBrightnessSmooth(-v, deltaTime);
                }
            }

            // turns off the inactive single color steps. this is outside the nested loop to save processing power.
            if (i >= chan)
                lights[STEP_LEDS + i].setBrightnessSmooth(0.f, deltaTime);
        }

        if (steps >= chan) // fix for a problem where the stepLight would disappear when changing sequencing modes
            steps = chan - 1;

        // SEQUENCING
        if ((clockTrigger.process(clk, 0.1, 1.0)) || (cvbt))
        {
            if (random::uniform() < params[CV_RND].getValue())
            {
                outputTrigger.trigger(1e-3f);
                lights[CV_BTLED].setBrightnessSmooth(1.f, deltaTime); // turning on the Button LED when either clock and/or button signals are incoming
                outputs[CV_TRIG].setVoltage(10.f);
                if (mode == 0) // forwards sequencing
                {
                    steps++;
                    if (steps >= chan)
                        steps = 0;
                }
                if (mode == 1) // backwards sequencing
                {
                    steps--;
                    if (steps < 0)
                        steps = (chan - 1);
                }
                if (mode == 2) // random sequencing
                    steps = std::floor(random::uniform() * chan);
            }
        }
        else if (resetTrigger.process(res))
        {
            if (mode == 0 || mode == 2)
                steps = 0;
            if (mode == 1)
                steps = (chan - 1);
        }
        float current = params[CV_KNOBS + steps].getValue();
        lights[STEP_LEDS + steps].setBrightnessSmooth(1.f, deltaTime); // steps is set to count in a way that always reflects the currently active step
        lights[TRIG_LED].setBrightnessSmooth(outTRG ? 1.f : 0.f, deltaTime / 300.f);
        if (current >= 0)
        {
            lights[SEQ_LED + 0].setBrightnessSmooth((current / (rangeknob * 2.f)), deltaTime);
            lights[SEQ_LED + 1].setBrightness(0.f);
        }
        else
        {
            lights[SEQ_LED + 0].setBrightness(0.f);
            lights[SEQ_LED + 1].setBrightnessSmooth(clamp(-current / rangeknob, -1.f, 1.f), deltaTime);
        }
        outputs[CV_TRIG].setVoltage(outTRG ? 10.f : 0.f);
        outputs[CV_OUTPUT].setVoltage(current);
    }
};

struct KnobXWidget : ModuleWidget
{
    KnobXWidget(KnobX *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        float col = 23, dis = 24, mul = 4.8;
        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(Vec(col - 8, dis * 1), module, KnobX::CV_BT, KnobX::CV_BTLED));
        addParam(createParamCentered<ModeSwitch>(Vec(col + 13, dis * 1), module, KnobX::CV_MODE));
        addInput(createInputCentered<PJ301MPort>(Vec(col - 7, dis * 2), module, KnobX::CV_CLK));
        addParam(createParamCentered<TrimpotSG>(Vec(col + 13, dis * 1.7), module, KnobX::CV_RND));
        addInput(createInputCentered<PortDark>(Vec(col + 8, dis * 2.85), module, KnobX::CV_RES));

        addParam(createParamCentered<TrimpotSB>(Vec(col - 11.5, dis * 3.85), module, KnobX::CV_STEP));
        addParam(createParamCentered<TrimpotSR>(Vec(col + 11.5, dis * 3.85), module, KnobX::CV_RANGE));

        for (int i = 0; i < 8; i++)
        {
            addParam(createParamCentered<TrimpotW>(Vec(col, dis * (i + mul)), module, KnobX::CV_KNOBS + i));
            addChild(createLightCentered<TinySimpleLight<GreenRedLight>>(Vec(37, dis * (i + mul) + 4), module, KnobX::CV_LEDS + i * 2));
            addChild(createLightCentered<TinySimpleLight<WhiteLight>>(Vec(9, dis * (i + mul) + 4), module, KnobX::STEP_LEDS + i));
        }
        // addParam(createParamCentered<ModeSwitch>(Vec(col - 13, dis * 12.9 - 15), module, KnobX::CV_MODE));

        addOutput(createOutputCentered<PJ301Mvar2>(Vec(col, dis * 12.9), module, KnobX::CV_TRIG));
        addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(39, dis * 12.9 + 4), module, KnobX::TRIG_LED));
        addOutput(createOutputCentered<ThemedPJ301MPort>(Vec(col, dis * 14), module, KnobX::CV_POLY));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(col, dis * 15.1), module, KnobX::CV_OUTPUT));
        addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(39, dis * 15.1 + 4), module, KnobX::SEQ_LED + 0 * 2));

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(11.5, 5);
        titleLabel->text = "KNOBS";
        addChild(titleLabel);
    }
};

Model *modelKnobX = createModel<KnobX, KnobXWidget>("KnobX");