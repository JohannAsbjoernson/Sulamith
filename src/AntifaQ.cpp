#include "plugin.hpp"

struct AntifaQ : Module
{
    enum ParamIds
    {
        NUM_PARAMS
    };
    enum InputIds
    {
        NUM_INPUTS
    };
    enum OutputIds
    {
        NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS
    };

    AntifaQ()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }
    void process(const ProcessArgs &args) override {}
};
struct AntifaQWidget : ModuleWidget
{
    AntifaQWidget(AntifaQ *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Antifa6hp-dark.svg")));
    }
};

Model *modelAntifaQ = createModel<AntifaQ, AntifaQWidget>("AntifaQ");