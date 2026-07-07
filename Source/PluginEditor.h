#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "gui/CandleLookAndFeel.h"
#include "gui/CandleKnob.h"

class CandleAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit CandleAudioProcessorEditor (CandleAudioProcessor&);
    ~CandleAudioProcessorEditor() override;

    void paint           (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized         () override;

private:
    void timerCallback() override;

    // Paint helpers (all work in 600×260 reference coords)
    void drawBackground   (juce::Graphics&);
    void drawHeader       (juce::Graphics&, juce::Rectangle<float>);
    void drawCassetteWindow (juce::Graphics&, juce::Rectangle<float>);
    void drawVUMeter      (juce::Graphics&, juce::Rectangle<float>, float level);
    void drawPanelSection (juce::Graphics&, juce::Rectangle<float>, const juce::String& title);
    void drawSegmentMeter (juce::Graphics&, juce::Rectangle<float>, float normDb, const juce::String& label);
    void drawRubblesonicBadge (juce::Graphics&, juce::Rectangle<float>);

    CandleAudioProcessor& processor;
    CandleLookAndFeel     lookAndFeel;

    // Root container for scale-transform
    juce::Component root;

    // ── MIXER panel controls ──────────────────────────────────────────────
    std::unique_ptr<CandleKnob> knobEqHigh, knobEqLow, knobTrim;
    juce::ToggleButton micButton { "REALISTIC" };

    // ── TAPE panel controls ───────────────────────────────────────────────
    juce::ToggleButton dbxButton  { "DBX" };
    juce::ComboBox     speedCombo;
    juce::ComboBox     genCombo;
    std::unique_ptr<CandleKnob> knobSat, knobNoise, knobWow, knobAge;

    // ── OUTPUT panel controls ─────────────────────────────────────────────
    std::unique_ptr<CandleKnob> knobOutput;
    juce::ToggleButton           bypassButton { "BYPASS" };
    juce::ComboBox               presetCombo;
    juce::TextButton              presetPrevButton { "<" }, presetNextButton { ">" };

    juce::TextButton  savePresetButton { "SAVE PRESET" };
    juce::TextEditor  presetNameEditor;
    juce::TextButton  presetSaveConfirmButton { "OK" }, presetSaveCancelButton { "X" };
    juce::TextButton  presetDeleteButton { "DELETE" }, presetRenameButton { "RENAME" };

    juce::ToggleButton tooltipToggle { "TOOLTIPS" };

    // ── APVTS Attachments ─────────────────────────────────────────────────
    using BtnAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CmbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<BtnAtt> micAtt, dbxAtt, bypassAtt;
    std::unique_ptr<CmbAtt> speedAtt, genAtt;

    // ── Preset list (factory + user presets, unified for combo + arrows) ──
    struct PresetEntry { juce::String name; bool isFactory; int factoryIndex; int comboId; };
    std::vector<PresetEntry> presetEntries;
    int currentPresetIndex = -1;
    bool isRenamingPreset = false;

    void rebuildPresetList();
    void selectPresetEntry (int index, bool applyToProcessor);
    void stepPreset (int delta);
    void beginSavePresetFlow();
    void beginRenamePresetFlow();
    void confirmSavePreset();
    void cancelSavePreset();
    void updatePresetManageButtonsEnabled();

    // ── Tooltip toggle (persisted via a small properties file) ───────────
    std::unique_ptr<juce::TooltipWindow>  tooltipWindow;
    std::unique_ptr<juce::PropertiesFile> tooltipSettings;

    // ── Panel bounds (reference 600×260) ─────────────────────────────────
    juce::Rectangle<int> mixerPanel, tapePanel, outputPanel;

    // ── Meter state ───────────────────────────────────────────────────────
    float vuDisplayLevel = 0.0f;
    float inMeterNorm    = 0.0f;
    float outMeterNorm   = 0.0f;

    // ── Spool animation ───────────────────────────────────────────────────
    float spoolAngle = 0.0f;

    // ── Resizer ──────────────────────────────────────────────────────────
    std::unique_ptr<juce::ResizableCornerComponent> resizer;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CandleAudioProcessorEditor)
};
