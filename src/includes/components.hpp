#pragma once

using namespace rack;
extern Plugin *pluginInstance;

// A components header. Putting all ports/knobs/sliders/buttons/switches/color schemes into here and globally including these via plugin.hpp
////////////////////////////////////////////////////////////////////////////////////
// CONTENTS
// 1. PORTS | 2. KNOBS | 3. SWITCHES | 4. BUTTONS
// 5. LIGHTS & COLOR SCHEMES | 6. TEXT & CV DISPLAYS
//
////////////////////////////////////////////////////////////////////////////////////
// PORTS
struct PJ301Mvar : app::SvgPort
{
    PJ301Mvar()
    {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/PJ301Mvar.svg")));
        shadow->opacity = 0.1f;
    }
};
struct PJ301Mvar2 : app::SvgPort
{
    PJ301Mvar2()
    {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/PJ301Mvar2.svg")));
        shadow->opacity = 0.1f;
    }
};
struct PJ301Mvar3 : app::SvgPort
{
    PJ301Mvar3()
    {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/PJ301Mvar3.svg")));
        shadow->opacity = 0.1f;
    }
};
struct PJ301Mvar4 : app::SvgPort
{
    PJ301Mvar4()
    {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/PJ301Mvar4.svg")));
        shadow->opacity = 0.1f;
    }
};
struct PortDark : app::SvgPort
{
    PortDark()
    {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ports/PortDark.svg")));
        shadow->opacity = 0.f;
    }
};

////////////////////////////////////////////////////////////////////////////////////
// KNOBS
struct TrimpotW : app::SvgKnob
{
    widget::SvgWidget *bg;

    TrimpotW()
    {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotW.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotW_bg.svg")));
    }
};
struct TrimpotS : app::SvgKnob
{
    widget::SvgWidget *bg;

    TrimpotS()
    {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotS.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotS_bg.svg")));
    }
};
struct TrimpotSR : app::SvgKnob
{
    widget::SvgWidget *bg;

    TrimpotSR()
    {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotS.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotSR_bg.svg")));
    }
};
struct TrimpotSB : app::SvgKnob
{
    widget::SvgWidget *bg;

    TrimpotSB()
    {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotS.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotSB_bg.svg")));
    }
};
struct TrimpotSG : app::SvgKnob
{
    widget::SvgWidget *bg;

    TrimpotSG()
    {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotS.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotSG_bg.svg")));
    }
};
struct TrimpotSY : app::SvgKnob
{
    widget::SvgWidget *bg;

    TrimpotSY()
    {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotS.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/TrimpotSY_bg.svg")));
    }
};
////////////////////////////////////////////////////////////////////////////////////
// SWITCHES
struct ModeSwitch : app::SvgSwitch
{
    ModeSwitch()
    {
        shadow->opacity = 0.0;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/ModeSwitch0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/ModeSwitch1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/ModeSwitch2.svg")));
    }
};
struct BTSwitch : app::SvgSwitch
{
    BTSwitch()
    {
        shadow->opacity = 0.0;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/BS0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/BS1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/BS2.svg")));
    }
};
struct SGB : app::SvgSwitch
{
    SGB()
    {
        momentary = true;
        shadow->opacity = 0.0;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/SGB0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/SGB1.svg")));
    }
};
////////////////////////////////////////////////////////////////////////////////////
// BUTTONS
struct WhiteButton : app::SvgSwitch
{
    WhiteButton()
    {
        shadow->opacity = 0.0;
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/whitebutton0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/buttons/whitebutton1.svg")));
    }
};

////////////////////////////////////////////////////////////////////////////////////
// LIGHTS & COLOR SCHEMES
template <typename Base = GrayModuleLightWidget>
struct TWhiteBlueLight : Base
{
    TWhiteBlueLight()
    {
        this->addBaseColor(SCHEME_WHITE);
        this->addBaseColor(SCHEME_BLUE);
    }
};
typedef TWhiteBlueLight<> WhiteBlueLight;

// TEXT WIDGETS
///////////////////////////////////////////////////////////////////
// #include <sstream>
// #include <string>
// for text widgets its usually one of those two you need to include. in this tutorial we have already included them in the plugin.hpp
// Put this into the Widget section below your components to make use of the label:
//
//      CenteredLabel *const titleLabel = new CenteredLabel;
//      titleLabel->box.pos = Vec(11.5, 5);
//      titleLabel->text = "TEXT HERE";
//      addChild(titleLabel);
//
// I disabled the box.size because I use only 1-liners with this.

struct CenteredLabel : Widget
{
    std::string text;
    // int fontSize;
    // CenteredLabel(int _fontSize = 9)
    //{
    //     fontSize = _fontSize;

    //    // box.size.y = 60.f;
    //    // use to define height of your box
    //}
    void draw(const DrawArgs &args) override
    {
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
        nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
        nvgFontSize(args.vg, 9.f);
        nvgText(args.vg, box.pos.x, box.pos.y, text.c_str(), NULL);
    }
};
