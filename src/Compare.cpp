#include "plugin.hpp"

struct Compare : Module
{
    enum ParamIds
    {
        THRESHOLD_PARAM,
        ATTEN_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        THRESHCV_INPUT,
        POLY_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        GT1_OUTPUT,
        GT2_OUTPUT,
        CV1_OUTPUT,
        CV2_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        OVER_LIGHT,
        UNDER_LIGHT,
        NUM_LIGHTS
    };

    Compare()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(THRESHOLD_PARAM, -10.0, 10.0, 0.0, "Threshold", " v");
        configParam(ATTEN_PARAM, 0.f, 1.f, 1.f, "Attenuate Threshold CV in", "%", 0, 100);
        configInput(THRESHCV_INPUT, "Threshold CV");
        configInput(POLY_INPUT, "Polyphonic Signal");
        configOutput(GT1_OUTPUT, "Gate (over Threshold)");
        configOutput(GT2_OUTPUT, "Gate (below Threshold)");
        configOutput(CV1_OUTPUT, "input Signal (over Threshold)");
        configOutput(CV2_OUTPUT, "input Signal (below Threshold)");
        configLight(OVER_LIGHT, "Over Threshold");
        configLight(UNDER_LIGHT, "Below Threshold");
    }

    void process(const ProcessArgs &args) override
    {
        // Get input channel# and clone to outputs
        float chnr_a = inputs[POLY_INPUT].getChannels();
        outputs[GT1_OUTPUT].setChannels(chnr_a);
        outputs[GT2_OUTPUT].setChannels(chnr_a);
        outputs[CV1_OUTPUT].setChannels(chnr_a);
        outputs[CV2_OUTPUT].setChannels(chnr_a);

        // Loop for Polyphonic inputs, Threshold param + cv input * attenuator into a boolean 'state' that compares the Input with the Threshold (bool C = A > B)
        // Output 10v if A > B on GT1 out; if B > A on GT2 out; if A > B output input signal to CV1 out; if B > A output input signal to CV2 out
        for (int i = 0; i < chnr_a; i++)
        {
            float threshold = clamp((params[THRESHOLD_PARAM].getValue() + (inputs[THRESHCV_INPUT].getVoltage(i) * params[ATTEN_PARAM].getValue())), -10.f, 10.f);
            bool state = inputs[POLY_INPUT].getVoltage(i) > threshold;

            outputs[GT1_OUTPUT].setVoltage((state ? 10.0 : 0.0), i);
            outputs[GT2_OUTPUT].setVoltage(!state ? 10.0 : 0.0, i);
            outputs[CV1_OUTPUT].setVoltage((state ? inputs[POLY_INPUT].getVoltage(i) : 0.0), i);
            outputs[CV2_OUTPUT].setVoltage((!state ? inputs[POLY_INPUT].getVoltage(i) : 0.0), i);

            lights[OVER_LIGHT].setBrightness(state ? 1.f : 0.f);
            lights[UNDER_LIGHT].setBrightness(state ? 0.f : 1.f);
        }
    }
};

struct CompareWidget : ModuleWidget
{

    CompareWidget(Compare *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        float x = 23.f;
        float y = 24.f;

        addParam(createParamCentered<RoundBlackKnob>(Vec(x, y * 1.4f), module, Compare::THRESHOLD_PARAM));
        addParam(createParamCentered<TrimpotSG>(Vec(13.5, y * 3.45f), module, Compare::ATTEN_PARAM));
        addInput(createInputCentered<PortDark>(Vec(x, y * 2.6f), module, Compare::THRESHCV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(x, y * 5.1f), module, Compare::POLY_INPUT));
        addOutput(createOutputCentered<ThemedPJ301MPort>(Vec(x, y * 6.4f), module, Compare::GT1_OUTPUT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(x + 13, y * 6.9f), module, Compare::OVER_LIGHT));
        addOutput(createOutputCentered<ThemedPJ301MPort>(Vec(x, y * 7.8f), module, Compare::GT2_OUTPUT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(x + 13, y * 8.3f), module, Compare::UNDER_LIGHT));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(x, y * 9.8f), module, Compare::CV1_OUTPUT));
        addOutput(createOutputCentered<PJ301Mvar>(Vec(x, y * 11.1f), module, Compare::CV2_OUTPUT));

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(11.5, 5);
        titleLabel->text = "COMP";
        addChild(titleLabel);
    }
};

Model *modelCompare = createModel<Compare, CompareWidget>("Compare");