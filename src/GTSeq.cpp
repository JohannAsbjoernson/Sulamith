#include "plugin.hpp"

// some numbers you need everywhere throughout the patch. Its good form to centralize these. A static integer is easy on processing, as it has to be read only once. Also if you want to change or expand on a module its easy to do here. const (constant) means the value is fixed (and cannot be changed unless directly in the source code). static means the integer persists in memory throughout repeated executions of functions. Usually a variable is erased from memory with each cycle. static or const can be used independently.
static const int gtMAX = 6;

struct GTSeq : Module
{
    enum ParamIds
    {
        ENUMS(GATE_LEN, gtMAX),
        GATE_MODE,
        GATE_PROB,
        GATE_STEP,
        GATE_BT,
        NUM_PARAMS
    };
    enum InputIds
    {
        GATE_IN,
        GATE_RES,
        NUM_INPUTS
    };
    enum OutputIds
    {
        GATE_OUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        ENUMS(GATE_LED, gtMAX),
        GATE_BLED,
        NUM_LIGHTS
    };

    float gtlength[gtMAX];
    int counter = -1;
    float countmode, countmode2;

    dsp::ClockDivider lightDivider;
    dsp::SchmittTrigger clockTrig;
    dsp::SchmittTrigger resetTrig;
    dsp::SchmittTrigger buttonTrig;
    dsp::PulseGenerator gateGenerator;

    GTSeq()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < gtMAX; i++)
        {
            configParam(GATE_LEN + i, 0.1f, 10.0f, 0.1f, "Length of gate", " sec");
        }
        configParam(GATE_BT, 0, 1, 0, "Manual Trigger", "If RES POLY IN & CLK NO IN btn clones IN to GATE_LEN Knobs");
        configParam(GATE_PROB, 0, 1, 1, "Probability", "%", 0, 100);
        configParam(GATE_STEP, 1, gtMAX, gtMAX, "Sequence Length", " Steps");
        configSwitch(GATE_MODE, 0, 2, 0, "Sequencing Mode", {"Forward", "Backward", "Random"});
        getParamQuantity(GATE_MODE)->randomizeEnabled = false;
        getParamQuantity(GATE_STEP)->snapEnabled = true;

        configInput(GATE_IN, "Clock/Trigger/Gate");
        configInput(GATE_RES, "Reset __|if RES POLY IN & CLK NO IN btn clones IN to GATE_LEN Knobs");
        configOutput(GATE_OUT, "Gate Sequence");
        configBypass(GATE_IN, GATE_OUT);

        lightDivider.setDivision(512);
    }

    void process(const ProcessArgs &args) override;
};

void GTSeq::process(const ProcessArgs &args)
{

    // You will find args in a ton of Source Codes. args are 'Arguments'. Arguments are broadly the values output by functions. sampleTime is a function that measures the step of process (the section we are currently looking at). Process runs in sample rate speeds, sampleTime translates this into seconds. Args are extremely flexible arrays that have no size restrictions, whos size need not pre set and args can store float, integer, string/char etc. values all in the same array.
    // Args are called via args.function, *function or function(val, val.. , args) [as far as I know]. They work in a front to back/top to bottom order.
    // Here args is continously calling on the sampleTime function and is multiplied by a clock divider (lightDivider: declared at the top, set up in the config).
    // This is a good method to get a stable Clock.
    float sample_time = args.sampleTime;
    float deltaTime = args.sampleTime * lightDivider.getDivision();

    int mode = params[GATE_MODE].getValue();
    int gtMAX2 = params[GATE_STEP].getValue();
    float gtPROB = params[GATE_PROB].getValue();

    for (int i = 0; i < gtMAX; i++)
    {
        gtlength[i] = clamp(params[GATE_LEN + i].getValue(), 0.0f, 10.0f);
        lights[GATE_LED + i].setBrightnessSmooth(0.f, deltaTime);
    }

    // checks push button using a schmitttrigger. if we just params.getValue() the gtBT would stay true as long as the button is pressed. a press < 1 second already amounts to a ton of cycles.
    // any if (gtBT = true) statement would run repeatedly during that time (we would advance the sequence by 5-200 steps). we: schmittTrigger.process(getParamValue ? true when button pressed : false when button not pressed) instead. This will send a 0.1 sec trigger once per button press, no matter how long or short it is held.
    bool gtBT = (buttonTrig.process(params[GATE_BT].getValue()) ? true : false);
    bool gtCLK = (clockTrig.process(inputs[GATE_IN].getVoltage()) ? true : false);
    // if a polyphonic cable is connected to the reset input, clock or button press clone poly input to the knobs. all gate length knobs will be set according to the input
    if ((clockTrig.process(inputs[GATE_IN].getVoltage()) || gtBT) && inputs[GATE_RES].isConnected() && inputs[GATE_RES].isPolyphonic())
    {
        int ch = std::max(inputs[GATE_RES].getChannels(), 1); // std::max(a, b) will retain the higher value. it is a sort of comparator.
        ch = clamp(ch, 1, gtMAX);                             // limit the channel number to our amount of gate length knobs
        int ct = 0;

        while (ct < ch) // this is a loop. unlike the for loop which has a set amount of runs, while (condition){} will keep running until the condition is fulfilled.
        {
            float val = inputs[GATE_RES].getVoltage(ct);
            getParamQuantity(GATE_LEN + ct)->setValue(val);
            ct++;
        }
        // change a knob to input value, then count up, if count < than total amount of knobs, set the next knobs to input value. etc.
        if (ct >= ch)
            ct = 0;
    }

    bool gtbool = (gateGenerator.process(sample_time) ? true : false);
    lights[GATE_BLED].setBrightnessSmooth(gtbool ? 1.f : 0.f, deltaTime / 300.f);

    if (resetTrig.process(inputs[GATE_RES].getVoltage()))
    {
        counter = -1;
        outputs[GATE_OUT].setVoltage(0.f);
    }

    if ((gtCLK || gtBT) && !gtbool && (random::uniform() < gtPROB))
    {
        // if (random::uniform() < gtPROB) // forces random
        //{
        if (mode == 0)
        {
            counter++;
            if (counter >= gtMAX2)
                counter = 0;
        }

        else if (mode == 1)
        {
            counter--;
            if (counter < 0)
                counter = gtMAX2 - 1;
        }

        else if (mode == 2)
            counter = std::floor(random::uniform() * gtMAX2);

        gateGenerator.trigger(gtlength[counter]);
        //}
    }

    // To keep the Step LED on the active Gate, but move LED to the next step when Gate is finished while incorporating the Sequencing Modes
    // and also correctly switch to the next step when the end of the Sequence is reached. The light is one step ahead when Gate is inactive.

    if (mode == 0) // FWD; countmode1 applies when stepcount is below 0, we want to raise it here
        countmode = counter + 1, countmode2 = counter + 1;
    else if (mode == 1) // BWD; here we want to step back to the last step of the sequence, countmode2 switches to the next step after END OF GATE
        countmode = gtMAX2 - 1, countmode2 = counter - 1;
    else if (mode == 2) // RND; random cannot be anticipated so we just stay where we are
        countmode = counter, countmode2 = counter;

    if (gtbool && counter < 0) // if gate is active but counter below zero
        lights[GATE_LED + countmode].setBrightnessSmooth(1.f, deltaTime);
    else if (!gtbool && counter < 0) // if gate is inactive but counter below zero
        lights[GATE_LED + countmode].setBrightnessSmooth(0.3f, deltaTime);
    else if (gtbool) // if gate is active (highlight the current step for duration of the gate)
        lights[GATE_LED + counter].setBrightnessSmooth(1.f, deltaTime);
    else
    {
        if (((counter + 1) >= gtMAX2) && mode == 0) // if current step is EOC, show LED next step on step1 (if mode1 is active)
            lights[GATE_LED + 0].setBrightnessSmooth(0.3f, deltaTime);
        else if (((counter - 1) < 0) && mode == 1) // if current step is below zero, show LED next step on last step (if mode2 is active)
            lights[GATE_LED + (gtMAX2 - 1)].setBrightnessSmooth(0.3f, deltaTime);
        else // if gate is inactive highlight next step
            lights[GATE_LED + countmode2].setBrightnessSmooth(0.3f, deltaTime);
    }

    // if gateGenerator is triggered, output 10v
    outputs[GATE_OUT].setVoltage((gateGenerator.process(sample_time) ? 10.0f : 0.0f));
}

struct GTSeqWidget : ModuleWidget
{
    GTSeqWidget(GTSeq *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        float col = 23, dis = 24, dis2 = 30, col2 = 25;

        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(Vec(col, dis * 1), module, GTSeq::GATE_BT, GTSeq::GATE_BLED));
        addInput(createInputCentered<PJ301MPort>(Vec(col, dis * 2.2), module, GTSeq::GATE_IN));
        addInput(createInputCentered<PortDark>(Vec(col, dis * 3.5), module, GTSeq::GATE_RES));
        addParam(createParamCentered<ModeSwitch>(Vec(col, dis * 4.5), module, GTSeq::GATE_MODE));
        addParam(createParamCentered<TrimpotSB>(Vec(col - 11.5, dis * 5.2), module, GTSeq::GATE_STEP));
        addParam(createParamCentered<TrimpotSG>(Vec(col + 11.5, dis * 5.2), module, GTSeq::GATE_PROB));
        for (int i = 0; i < gtMAX; i++)
        {
            addParam(createParamCentered<RoundSmallBlackKnob>(Vec(col2, dis2 * (5 + i) + 6), module, GTSeq::GATE_LEN + i));
            addChild(createLightCentered<SmallSimpleLight<WhiteLight>>(Vec(7, dis2 * (5 + i) + 8), module, GTSeq::GATE_LED + i));
        }
        addOutput(createOutputCentered<PJ3410Port>(Vec(col, dis * 14.2), module, GTSeq::GATE_OUT));

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(11.5, 5);
        titleLabel->text = "GTSeq";
        addChild(titleLabel);
    }
};

Model *modelGTSeq = createModel<GTSeq, GTSeqWidget>("GTSeq");