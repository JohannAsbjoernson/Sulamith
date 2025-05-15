#include "plugin.hpp"

#define strNUM 16

struct VoltM : Module
{
    enum ParamIds
    {
        NUM_PARAMS
    };
    enum InputIds
    {
        vIN1,
        vIN2,
        vIN3,
        NUM_INPUTS
    };
    enum OutputIds
    {
        vOUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        NUM_LIGHTS
    };

    std::string strVAL = "";
    float valLines[strNUM] = {0.f};
    int scount = 0;

    VoltM()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configInput(vIN1, "Poly Voltage Signal: A");
        configInput(vIN2, "Poly Voltage Signal: B");
        configInput(vIN3, "Poly Voltage Signal: C");
        configOutput(vOUT, "Combined Poly Voltage Signal");
    }

    void process(const ProcessArgs &args) override;
};

void VoltM::process(const ProcessArgs &args)
{
    std::string strVAL2;
    float in1, in2, in3;
    int ch1, ch2, ch3, chab, chmix;

    // grab all channels
    ch1 = inputs[vIN1].getChannels(), ch2 = inputs[vIN2].getChannels(), ch3 = inputs[vIN3].getChannels();
    chab = clamp((ch1 + ch2), 0, 16);
    chmix = chab;
    chmix = clamp((ch1 + ch2 + ch3), 0, 16);
    outputs[vOUT].setChannels(chmix);
    if (chmix >= 1)
    {
        for (int i = 0; i < chmix; i++)
        {
            if (i < ch1)
                in1 = inputs[vIN1].getVoltage(i), valLines[i] = {in1};
            else if ((i >= ch1) && (i < chab))
                in2 = inputs[vIN2].getVoltage((i - ch1)), valLines[i] = {in2};
            else if ((i >= chab) && (i <= chmix))
                in3 = inputs[vIN3].getVoltage((i - chab)), valLines[i] = {in3};
            else
            {
                valLines[i] = 0;
                strVAL = "";
            }

            outputs[vOUT].setVoltage(valLines[i], i);
        }
    }

    scount++;

    if (scount > 2050)
    {
        scount = 0;
        strVAL2 = "";
        std::string thisLine = "";
        for (int a = 0; a < chmix; a = (a + 1))
        {
            thisLine = valLines[a] >= 0 ? "+" : "";
            thisLine += std::to_string(valLines[a]);
            thisLine = thisLine.substr(0, 5);
            strVAL2 += (a > 0 ? "\n" : "") + thisLine;
        }
        strVAL = strVAL2;
    }
}

struct StringDisplayWidget3 : Widget
{

    std::string value;
    std::shared_ptr<Font> font;
    VoltM *module;

    StringDisplayWidget3()
    {
        font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Oswald-Regular.ttf"));
    };

    void draw(const DrawArgs &ctx) override
    {
        nvgBeginPath(ctx.vg);
        nvgRoundedRect(ctx.vg, -1.0, -1.0, box.size.x, box.size.y + 2, 0.0);
        nvgBeginPath(ctx.vg);
        nvgRoundedRect(ctx.vg, 0.0, 0.0, box.size.x, box.size.y, 0.0);

        nvgFontSize(ctx.vg, 15);
        nvgFontFaceId(ctx.vg, font->handle);
        nvgTextLetterSpacing(ctx.vg, 1.5);

        std::string textToDraw = module ? module->strVAL : "";
        Vec textPos = Vec(3.5f, 10.0f);
        NVGcolor textColor = nvgRGB(0xff, 0xff, 0xff);
        nvgFillColor(ctx.vg, textColor);
        nvgTextBox(ctx.vg, textPos.x, textPos.y, 80, textToDraw.c_str(), NULL);
    }
};
struct VoltMWidget : ModuleWidget
{
    VoltMWidget(VoltM *module)
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Panel3hp-dark.svg")));

        StringDisplayWidget3 *stringDisplay = createWidget<StringDisplayWidget3>(Vec(6, 16));
        stringDisplay->box.size = Vec(80, 220);
        stringDisplay->module = module;
        addChild(stringDisplay);

        addInput(createInputCentered<PJ301Mvar2>(mm2px(Vec(6, 93)), module, VoltM::vIN1));
        addInput(createInputCentered<PJ301Mvar2>(mm2px(Vec(10.5, 101)), module, VoltM::vIN2));
        addInput(createInputCentered<PJ301Mvar2>(mm2px(Vec(6, 109)), module, VoltM::vIN3));
        addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(8, 118.5)), module, VoltM::vOUT));

        CenteredLabel *const titleLabel = new CenteredLabel;
        titleLabel->box.pos = Vec(11.5, 5);
        titleLabel->text = "VOLT";
        addChild(titleLabel);
    }
};

Model *modelVoltM = createModel<VoltM, VoltMWidget>("VoltM");