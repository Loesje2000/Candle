#include "PluginEditor.h"

static float dbToNorm (float db) noexcept
{
    return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
}

static std::unique_ptr<juce::PropertiesFile> makeTooltipSettings()
{
    juce::PropertiesFile::Options opts;
    opts.applicationName     = "Settings";
    opts.filenameSuffix      = "xml";
    opts.folderName          = "Rubblesonic/Candle";
    opts.osxLibrarySubFolder = "Application Support";
    return std::make_unique<juce::PropertiesFile> (opts);
}

// ─────────────────────────────────────────────────────────────────────────────
CandleAudioProcessorEditor::CandleAudioProcessorEditor (CandleAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    root.setSize (780, 360);
    addAndMakeVisible (root);

    // ── MIXER knobs (cream labels — panels are now dark) ──────────────────
    knobTrim   = std::make_unique<CandleKnob> ("TRIM",    processor.apvts, "trim",    false);
    knobEqHigh = std::make_unique<CandleKnob> ("EQ HIGH", processor.apvts, "eq_high", false);
    knobEqLow  = std::make_unique<CandleKnob> ("EQ LOW",  processor.apvts, "eq_low",  false);
    for (auto* k : { knobTrim.get(), knobEqHigh.get(), knobEqLow.get() })
        root.addAndMakeVisible (*k);

    micButton.setClickingTogglesState (true);
    micButton.setTooltip ("RadioShack Realistic mic emulation (Elliott Smith vocal character)");
    micAtt = std::make_unique<BtnAtt> (processor.apvts, "mic_on", micButton);
    root.addAndMakeVisible (micButton);

    // ── TAPE controls ─────────────────────────────────────────────────────
    dbxButton.setClickingTogglesState (true);
    dbxAtt = std::make_unique<BtnAtt> (processor.apvts, "dbx", dbxButton);
    root.addAndMakeVisible (dbxButton);

    speedCombo.addItem ("HIGH  3-3/4 ips", 1);
    speedCombo.addItem ("NORM  1-7/8 ips", 2);
    speedCombo.addItem ("SLOW 15/16 ips",  3);
    speedAtt = std::make_unique<CmbAtt> (processor.apvts, "tape_speed", speedCombo);
    root.addAndMakeVisible (speedCombo);

    genCombo.addItem ("0 GEN", 1);
    genCombo.addItem ("1 GEN", 2);
    genCombo.addItem ("2 GEN", 3);
    genCombo.addItem ("3 GEN", 4);
    genCombo.addItem ("4 GEN", 5);
    genAtt = std::make_unique<CmbAtt> (processor.apvts, "generations", genCombo);
    root.addAndMakeVisible (genCombo);

    knobSat   = std::make_unique<CandleKnob> ("SATURATE", processor.apvts, "saturation", false);
    knobNoise = std::make_unique<CandleKnob> ("NOISE",    processor.apvts, "noise",       false);
    knobWow   = std::make_unique<CandleKnob> ("WOW/FLT",  processor.apvts, "wow_flutter", false);
    knobAge   = std::make_unique<CandleKnob> ("AGE",      processor.apvts, "age",         false);
    for (auto* k : { knobSat.get(), knobNoise.get(), knobWow.get(), knobAge.get() })
        root.addAndMakeVisible (*k);

    // ── OUTPUT ────────────────────────────────────────────────────────────
    knobOutput = std::make_unique<CandleKnob> ("OUTPUT", processor.apvts, "output_gain", false);
    root.addAndMakeVisible (*knobOutput);

    // ── BYPASS (header, top right) ────────────────────────────────────────
    bypassButton.setClickingTogglesState (true);
    bypassAtt = std::make_unique<BtnAtt> (processor.apvts, "global_bypass", bypassButton);
    root.addAndMakeVisible (bypassButton);

    // ── Preset combo + prev/next arrows ───────────────────────────────────
    rebuildPresetList();
    presetCombo.onChange = [this]
    {
        const int sel = presetCombo.getSelectedId();
        for (int i = 0; i < (int) presetEntries.size(); ++i)
        {
            if (presetEntries[(size_t) i].comboId == sel)
            {
                selectPresetEntry (i, true);
                break;
            }
        }
    };
    root.addAndMakeVisible (presetCombo);

    presetPrevButton.onClick = [this] { stepPreset (-1); };
    presetNextButton.onClick = [this] { stepPreset (+1); };
    root.addAndMakeVisible (presetPrevButton);
    root.addAndMakeVisible (presetNextButton);

    // ── Save preset flow (in-editor, no detached dialog) ─────────────────
    savePresetButton.onClick = [this] { beginSavePresetFlow(); };
    root.addAndMakeVisible (savePresetButton);

    presetNameEditor.setColour (juce::TextEditor::backgroundColourId, CandleColours::btnOff);
    presetNameEditor.setColour (juce::TextEditor::outlineColourId,    CandleColours::panelBorder.withAlpha (0.45f));
    presetNameEditor.setColour (juce::TextEditor::focusedOutlineColourId, CandleColours::amber);
    presetNameEditor.setColour (juce::TextEditor::textColourId,       CandleColours::amber);
    presetNameEditor.setFont (lookAndFeel.getGeorgiaFont (12.0f));
    presetNameEditor.onReturnKey = [this] { confirmSavePreset(); };
    presetNameEditor.onEscapeKey = [this] { cancelSavePreset(); };
    presetSaveConfirmButton.onClick = [this] { confirmSavePreset(); };
    presetSaveCancelButton.onClick  = [this] { cancelSavePreset(); };
    root.addAndMakeVisible (presetNameEditor);
    root.addAndMakeVisible (presetSaveConfirmButton);
    root.addAndMakeVisible (presetSaveCancelButton);
    presetNameEditor.setVisible (false);
    presetSaveConfirmButton.setVisible (false);
    presetSaveCancelButton.setVisible (false);

    presetDeleteButton.onClick = [this]
    {
        if (currentPresetIndex < 0 || currentPresetIndex >= (int) presetEntries.size())
            return;
        if (presetEntries[(size_t) currentPresetIndex].isFactory)
            return;
        if (processor.deleteUserPreset (presetEntries[(size_t) currentPresetIndex].name))
            rebuildPresetList();
    };
    presetRenameButton.onClick = [this] { beginRenamePresetFlow(); };
    root.addAndMakeVisible (presetDeleteButton);
    root.addAndMakeVisible (presetRenameButton);
    updatePresetManageButtonsEnabled();

    // ── Tooltip toggle (persisted globally, not per-project) ─────────────
    tooltipSettings = makeTooltipSettings();
    const bool tooltipsOn = tooltipSettings->getBoolValue ("tooltipsEnabled", true);
    tooltipToggle.setClickingTogglesState (true);
    tooltipToggle.setToggleState (tooltipsOn, juce::dontSendNotification);
    if (tooltipsOn)
        tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 500);
    tooltipToggle.onClick = [this]
    {
        const bool on = tooltipToggle.getToggleState();
        tooltipWindow = on ? std::make_unique<juce::TooltipWindow> (this, 500) : nullptr;
        tooltipSettings->setValue ("tooltipsEnabled", on);
        tooltipSettings->saveIfNeeded();
    };
    root.addAndMakeVisible (tooltipToggle);

    // ── Tooltips ─────────────────────────────────────────────────────────
    knobTrim->setTooltip   ("Input trim - set recording level before the tape chain (+/-20 dB)");
    knobEqHigh->setTooltip ("Mixer high shelf - boosts or cuts above 10 kHz before tape (+/-10 dB)");
    knobEqLow->setTooltip  ("Mixer low shelf - boosts or cuts below 100 Hz before tape (+/-10 dB)");
    micButton.setTooltip   ("RadioShack Realistic mic emulation - shapes tone like the cheap "
                            "dynamic mic Elliott Smith used for vocals on Candle");
    dbxButton.setTooltip   ("DBX Type II noise reduction - on = quieter hiss, off = raw open-reel "
                            "noise floor (Smith often bypassed or had a faulty DBX)");
    speedCombo.setTooltip  ("Tape speed - HIGH (3-3/4 ips) is cleanest, NORM (1-7/8 ips) is the "
                            "Candle speed, SLOW (15/16 ips) is maximum degradation");
    genCombo.setTooltip    ("Bounce generations - each ping-pong overdub adds noise and rolls off "
                            "high frequencies, as on multi-tracked cassette recordings");
    knobSat->setTooltip    ("Tape saturation - how hard the ferric oxide is driven; adds "
                            "2nd-harmonic warmth and soft limiting");
    knobNoise->setTooltip  ("Tape hiss + AC hum level - at zero the noise floor is fully suppressed; "
                            "Candle preset sits around 55%");
    knobWow->setTooltip    ("Wow & flutter - slow pitch drift (wow) and fast capstan wobble (flutter) "
                            "from the 424's transport mechanism");
    knobAge->setTooltip    ("Head wear - increases HF loss, dropout frequency, and print-through "
                            "post-echo; simulates a heavily used transport");
    knobOutput->setTooltip ("Output level after the full tape chain (+/-12 dB)");
    bypassButton.setTooltip ("Bypass the entire tape chain - dry signal passes through unchanged");
    presetCombo.setTooltip  ("Factory presets - Candle, Porta, Bounce x3, Late Night, "
                             "New Tape");
    presetPrevButton.setTooltip ("Previous preset");
    presetNextButton.setTooltip ("Next preset");
    savePresetButton.setTooltip ("Save the current settings as a new preset");
    presetDeleteButton.setTooltip ("Delete the selected user preset");
    presetRenameButton.setTooltip ("Rename the selected user preset");
    tooltipToggle.setTooltip    ("Show or hide mouse-over tooltips like this one");

    // ── Resizer ──────────────────────────────────────────────────────────
    constrainer.setSizeLimits (546, 252, 1170, 540);
    constrainer.setFixedAspectRatio (780.0 / 360.0);
    resizer = std::make_unique<juce::ResizableCornerComponent> (this, &constrainer);
    addAndMakeVisible (*resizer);

    setSize (780, 360);
    startTimerHz (24);
}

CandleAudioProcessorEditor::~CandleAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    stopTimer();
}

// ── Preset list (factory + user, unified for combo + arrows) ──────────────────
void CandleAudioProcessorEditor::rebuildPresetList()
{
    const auto previousName = (currentPresetIndex >= 0 && currentPresetIndex < (int) presetEntries.size())
                                   ? presetEntries[(size_t) currentPresetIndex].name : juce::String();

    presetCombo.clear (juce::dontSendNotification);
    presetEntries.clear();

    presetCombo.addItem ("-- select preset --", 1);

    int nextId = 2;
    presetCombo.addSectionHeading ("FACTORY");
    const auto factoryNames = processor.getPresetNames();
    for (int i = 0; i < factoryNames.size(); ++i)
    {
        presetCombo.addItem (factoryNames[i], nextId);
        presetEntries.push_back ({ factoryNames[i], true, i, nextId });
        ++nextId;
    }

    const auto userNames = processor.getUserPresetNames();
    if (! userNames.isEmpty())
    {
        presetCombo.addSectionHeading ("USER");
        for (int i = 0; i < userNames.size(); ++i)
        {
            presetCombo.addItem (userNames[i], nextId);
            presetEntries.push_back ({ userNames[i], false, i, nextId });
            ++nextId;
        }
    }

    currentPresetIndex = -1;
    if (previousName.isNotEmpty())
        for (int i = 0; i < (int) presetEntries.size(); ++i)
            if (presetEntries[(size_t) i].name == previousName)
                { currentPresetIndex = i; break; }

    presetCombo.setSelectedId (currentPresetIndex >= 0 ? presetEntries[(size_t) currentPresetIndex].comboId : 1,
                               juce::dontSendNotification);
    updatePresetManageButtonsEnabled();
}

void CandleAudioProcessorEditor::selectPresetEntry (int index, bool applyToProcessor)
{
    if (index < 0 || index >= (int) presetEntries.size())
        return;

    const auto& entry = presetEntries[(size_t) index];
    currentPresetIndex = index;
    presetCombo.setSelectedId (entry.comboId, juce::dontSendNotification);

    if (applyToProcessor)
    {
        if (entry.isFactory)
            processor.applyFactoryPreset (entry.factoryIndex);
        else
            processor.loadUserPresetByName (entry.name);
    }
    updatePresetManageButtonsEnabled();
}

void CandleAudioProcessorEditor::updatePresetManageButtonsEnabled()
{
    const bool isUserPreset = currentPresetIndex >= 0
                            && currentPresetIndex < (int) presetEntries.size()
                            && ! presetEntries[(size_t) currentPresetIndex].isFactory;
    presetDeleteButton.setEnabled (isUserPreset);
    presetRenameButton.setEnabled (isUserPreset);
}

void CandleAudioProcessorEditor::stepPreset (int delta)
{
    if (presetEntries.empty())
        return;

    const int total = (int) presetEntries.size();
    const int newIndex = (currentPresetIndex < 0) ? 0
                                                   : ((currentPresetIndex + delta) % total + total) % total;
    selectPresetEntry (newIndex, true);
}

// ── Save/rename-preset flow (plain in-editor child components, no detached dialog) ──
void CandleAudioProcessorEditor::beginSavePresetFlow()
{
    isRenamingPreset = false;
    savePresetButton.setVisible (false);
    presetDeleteButton.setEnabled (false);
    presetRenameButton.setEnabled (false);
    presetNameEditor.setText ({}, juce::dontSendNotification);
    presetNameEditor.setVisible (true);
    presetSaveConfirmButton.setVisible (true);
    presetSaveCancelButton.setVisible (true);
    presetNameEditor.grabKeyboardFocus();
}

void CandleAudioProcessorEditor::beginRenamePresetFlow()
{
    if (currentPresetIndex < 0 || currentPresetIndex >= (int) presetEntries.size())
        return;
    if (presetEntries[(size_t) currentPresetIndex].isFactory)
        return;

    isRenamingPreset = true;
    savePresetButton.setVisible (false);
    presetDeleteButton.setEnabled (false);
    presetRenameButton.setEnabled (false);
    presetNameEditor.setText (presetEntries[(size_t) currentPresetIndex].name, juce::dontSendNotification);
    presetNameEditor.setVisible (true);
    presetSaveConfirmButton.setVisible (true);
    presetSaveCancelButton.setVisible (true);
    presetNameEditor.grabKeyboardFocus();
    presetNameEditor.selectAll();
}

void CandleAudioProcessorEditor::confirmSavePreset()
{
    const auto name = presetNameEditor.getText().trim();
    if (name.isEmpty())
        return;

    bool ok = false;
    if (isRenamingPreset)
    {
        const auto oldName = (currentPresetIndex >= 0 && currentPresetIndex < (int) presetEntries.size())
                                  ? presetEntries[(size_t) currentPresetIndex].name : juce::String();
        ok = oldName.isNotEmpty() && processor.renameUserPreset (oldName, name);
    }
    else
    {
        ok = processor.saveUserPreset (name);
    }

    if (ok)
    {
        rebuildPresetList();
        for (int i = 0; i < (int) presetEntries.size(); ++i)
        {
            if (! presetEntries[(size_t) i].isFactory && presetEntries[(size_t) i].name == name)
            {
                selectPresetEntry (i, false); // state on disk already matches current parameters
                break;
            }
        }
        cancelSavePreset();
    }
}

void CandleAudioProcessorEditor::cancelSavePreset()
{
    isRenamingPreset = false;
    presetNameEditor.setVisible (false);
    presetSaveConfirmButton.setVisible (false);
    presetSaveCancelButton.setVisible (false);
    savePresetButton.setVisible (true);
    updatePresetManageButtonsEnabled();
}

// ── Timer ─────────────────────────────────────────────────────────────────────
void CandleAudioProcessorEditor::timerCallback()
{
    const float vu   = processor.getVULevel();
    vuDisplayLevel  += 0.25f * (vu - vuDisplayLevel);
    inMeterNorm     += 0.35f * (dbToNorm (processor.getInputMeterDb())  - inMeterNorm);
    outMeterNorm    += 0.35f * (dbToNorm (processor.getOutputMeterDb()) - outMeterNorm);

    // Rotate spools: base 1 rps, scaled by tape speed param
    const float speedIdx = processor.apvts.getRawParameterValue ("tape_speed")->load();
    const float rps = (speedIdx < 0.5f) ? 1.50f : (speedIdx > 1.5f) ? 0.50f : 1.00f;
    spoolAngle += rps * juce::MathConstants<float>::twoPi / 24.0f;
    if (spoolAngle > juce::MathConstants<float>::twoPi) spoolAngle -= juce::MathConstants<float>::twoPi;

    repaint();
}

// ── resized ───────────────────────────────────────────────────────────────────
void CandleAudioProcessorEditor::resized()
{
    const float sx = getWidth()  / 780.0f;
    const float sy = getHeight() / 360.0f;
    root.setTransform (juce::AffineTransform::scale (sx, sy));
    root.setBounds (0, 0, 780, 360);

    // ── Reference-space layout constants (all in 780×360 coords) ─────────
    constexpr int kHeaderH = 54;
    constexpr int kBodyY   = kHeaderH;
    constexpr int kBodyH   = 360 - kHeaderH;  // 306

    // Panel x-boundaries
    constexpr int kMixW = 195;   // MIXER panel width
    constexpr int kTapeX = kMixW;
    constexpr int kTapeW = 390;
    constexpr int kOutX  = kMixW + kTapeW;  // 585
    constexpr int kOutW  = 780 - kOutX;     // 195

    mixerPanel  = { 0,      kBodyY, kMixW, kBodyH };
    tapePanel   = { kTapeX, kBodyY, kTapeW, kBodyH };
    outputPanel = { kOutX,  kBodyY, kOutW,  kBodyH };

    // ── BYPASS in header (top right, left of badge) ────────────────────
    bypassButton.setBounds (710, 15, 58, 24);

    // ── MIXER (channel strip: TRIM top, EQ HIGH + EQ LOW below, REALISTIC foot)
    constexpr int kBigKnob = 68;   // TRIM (prominent, top of strip)
    constexpr int kSmKnob  = 54;   // EQ knobs

    const int trimX = (kMixW - kBigKnob) / 2;
    knobTrim->setBounds   (trimX, kBodyY + 38, kBigKnob, kBigKnob + 14);

    const int eqY   = kBodyY + 38 + kBigKnob + 14 + 8;
    const int eqGap = (kMixW - kSmKnob * 2) / 3;
    knobEqHigh->setBounds (eqGap,             eqY, kSmKnob, kSmKnob + 14);
    knobEqLow->setBounds  (eqGap * 2 + kSmKnob, eqY, kSmKnob, kSmKnob + 14);

    const int micY = eqY + kSmKnob + 14 + 10;
    micButton.setBounds (10, micY, kMixW - 20, 22);

    // ── TAPE controls row 1: DBX  SPEED  GEN ─────────────────────────────
    const int row1Y = kBodyY + 36;
    dbxButton.setBounds  (kTapeX + 8,   row1Y, 52, 22);
    speedCombo.setBounds (kTapeX + 68,  row1Y, 130, 22);
    genCombo.setBounds   (kTapeX + 208, row1Y, 108, 22);

    // TAPE knob row: SAT  NOISE  WOW/FLT  AGE  (evenly spaced in 390px)
    constexpr int kTapeKnob = 62;
    const int tapeKnobY  = kBodyY + 70;
    const int tapeKnobSp = (kTapeW - kTapeKnob * 4) / 5;
    knobSat->setBounds   (kTapeX + tapeKnobSp,                             tapeKnobY, kTapeKnob, kTapeKnob + 14);
    knobNoise->setBounds (kTapeX + tapeKnobSp * 2 + kTapeKnob,             tapeKnobY, kTapeKnob, kTapeKnob + 14);
    knobWow->setBounds   (kTapeX + tapeKnobSp * 3 + kTapeKnob * 2,         tapeKnobY, kTapeKnob, kTapeKnob + 14);
    knobAge->setBounds   (kTapeX + tapeKnobSp * 4 + kTapeKnob * 3,         tapeKnobY, kTapeKnob, kTapeKnob + 14);

    // ── OUTPUT: big knob + preset row + save row + tooltip toggle ────────
    constexpr int kOutKnob = 80;
    const int outKnobX = kOutX + (kOutW - kOutKnob) / 2;
    knobOutput->setBounds (outKnobX, kBodyY + 38, kOutKnob, kOutKnob + 14);

    constexpr int kArrowW = 20;
    constexpr int kRowGap = 4;
    const int presetRowY = kBodyY + 160;
    const int presetRowX = kOutX + 8;
    const int presetRowW = kOutW - 16;
    presetPrevButton.setBounds (presetRowX, presetRowY, kArrowW, 22);
    presetCombo.setBounds      (presetRowX + kArrowW + kRowGap, presetRowY,
                                 presetRowW - 2 * (kArrowW + kRowGap), 22);
    presetNextButton.setBounds (presetRowX + presetRowW - kArrowW, presetRowY, kArrowW, 22);

    const int saveRowY = presetRowY + 22 + 10;
    savePresetButton.setBounds (presetRowX, saveRowY, presetRowW, 20);
    presetNameEditor.setBounds        (presetRowX, saveRowY, presetRowW - 40, 20);
    presetSaveConfirmButton.setBounds (presetRowX + presetRowW - 40, saveRowY, 18, 20);
    presetSaveCancelButton.setBounds  (presetRowX + presetRowW - 20, saveRowY, 18, 20);

    const int tipsRowY = saveRowY + 20 + 14;
    tooltipToggle.setBounds (presetRowX, tipsRowY, presetRowW, 22);

    const int manageRowY = tipsRowY + 22 + 8;
    const int manageBtnW = (presetRowW - kRowGap) / 2;
    presetRenameButton.setBounds (presetRowX, manageRowY, manageBtnW, 20);
    presetDeleteButton.setBounds (presetRowX + manageBtnW + kRowGap, manageRowY,
                                   presetRowW - manageBtnW - kRowGap, 20);

    resizer->setBounds (getWidth() - 14, getHeight() - 14, 14, 14);
}

// ── paint ─────────────────────────────────────────────────────────────────────
// Draws all backgrounds — children render on top.
void CandleAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float sx = getWidth()  / 780.0f;
    const float sy = getHeight() / 360.0f;
    g.addTransform (juce::AffineTransform::scale (sx, sy));

    drawBackground (g);
    drawHeader     (g, { 0.0f, 0.0f, 780.0f, 54.0f });
    drawPanelSection (g, mixerPanel.toFloat(),  "MIXER");
    drawPanelSection (g, tapePanel.toFloat(),   "TAPE");
    drawPanelSection (g, outputPanel.toFloat(), "OUTPUT");
}

// Animated meters sit on top of everything (header zone — no controls there).
void CandleAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    const float sx = getWidth()  / 780.0f;
    const float sy = getHeight() / 360.0f;
    g.addTransform (juce::AffineTransform::scale (sx, sy));

    // Animated VU needle
    const float vuDb = vuDisplayLevel > 1.0e-6f
                     ? juce::Decibels::gainToDecibels (vuDisplayLevel) + 18.0f : -99.0f;
    drawVUMeter      (g, { 540.0f, 6.0f, 96.0f, 42.0f }, vuDb);
    drawSegmentMeter (g, { 642.0f, 10.0f, 28.0f, 30.0f }, inMeterNorm,  "IN");
    drawSegmentMeter (g, { 674.0f, 10.0f, 28.0f, 30.0f }, outMeterNorm, "OUT");
}

// ── Drawing helpers ────────────────────────────────────────────────────────────

void CandleAudioProcessorEditor::drawBackground (juce::Graphics& g)
{
    juce::ColourGradient linear (CandleColours::panelHi, 0.0f, 0.0f,
                                 CandleColours::panelLo, 0.0f, 360.0f, false);
    linear.addColour (0.45, CandleColours::panel);
    g.setGradientFill (linear);
    g.fillRoundedRectangle (0.0f, 0.0f, 780.0f, 360.0f, 6.0f);

    juce::ColourGradient radial (CandleColours::panelHi, 234.0f, 108.0f,
                                 CandleColours::panelLo, 741.0f, 306.0f, true);
    radial.addColour (0.55, CandleColours::panel);
    g.setGradientFill (radial);
    g.fillRoundedRectangle (0.0f, 0.0f, 780.0f, 360.0f, 6.0f);

    juce::ColourGradient spot (CandleColours::amber.withAlpha (0.18f), 165.0f, 120.0f,
                               juce::Colours::transparentBlack, 470.0f, 285.0f, true);
    spot.addColour (0.55, CandleColours::amber.withAlpha (0.08f));
    g.setGradientFill (spot);
    g.fillRect (0, 0, 780, 360);

    // Subtle warm grain
    juce::Random rng (77);
    g.setColour (juce::Colours::white.withAlpha (0.006f));
    for (int i = 0; i < 600; ++i)
        g.fillRect (rng.nextFloat() * 780.0f, rng.nextFloat() * 360.0f,
                    rng.nextFloat() * 1.5f + 0.5f, rng.nextFloat() * 1.5f + 0.5f);

    g.setColour (juce::Colours::black.withAlpha (0.04f));
    for (int y = 0; y < 360; y += 4)
        g.fillRect (0, y, 780, 1);
}

void CandleAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<float> r)
{
    // BuckleyHaze-style top rail over Candle's cassette/VU hardware.
    g.setColour (juce::Colour (0x8c160c07));
    g.fillRect (r);
    g.setColour (CandleColours::panelRule.withAlpha (0.35f));
    g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getBottom(), 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (r.withHeight (1.0f).reduced (1.0f, 0.0f));

    drawRubblesonicBadge (g, { 8.0f, 7.0f, 145.0f, 40.0f });

    g.setFont (lookAndFeel.getGeorgiaFont (30.0f, juce::Font::bold | juce::Font::italic));
    g.setColour (CandleColours::amber.withAlpha (0.20f));
    g.drawText ("CANDLE", 171, 5, 150, 31, juce::Justification::centredLeft);
    g.setColour (CandleColours::amberHi);
    g.drawText ("CANDLE", 169, 3, 150, 31, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.getGeorgiaFont (8.0f, juce::Font::bold));
    g.setColour (CandleColours::creamDim);
    g.drawText ("P O R T A  4 2 4  .  C A S S E T T E",
                171, 36, 188, 13, juce::Justification::centredLeft);

    g.setFont (lookAndFeel.getMonoFont (7.0f));
    g.setColour (CandleColours::creamDim.withAlpha (0.78f));
    g.drawText ("v" JucePlugin_VersionString,
                310, 36, 45, 12, juce::Justification::centredRight);

    // ── Cassette window (center of header) ────────────────────────────────
    drawCassetteWindow (g, { 360.0f, 4.0f, 174.0f, 46.0f });

    // ── VU + meters placeholders (filled in paintOverChildren) ────────────
    g.setColour (CandleColours::vuBezel);
    g.fillRoundedRectangle (540.0f, 6.0f, 96.0f, 42.0f, 3.0f);
}

void CandleAudioProcessorEditor::drawCassetteWindow (juce::Graphics& g, juce::Rectangle<float> r)
{
    // Outer bezel — slightly lighter than chassis, like the 424's plastic surround
    g.setColour (CandleColours::panelHi);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (CandleColours::panelBorder.withAlpha (0.7f));
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.2f);

    // Inner dark viewing window
    const auto inner = r.reduced (5.0f, 5.0f);
    g.setColour (CandleColours::bgDeep);
    g.fillRoundedRectangle (inner, 2.5f);
    g.setColour (CandleColours::panelBorder.withAlpha (0.4f));
    g.drawRoundedRectangle (inner.reduced (0.5f), 2.5f, 0.8f);

    // Guide marks at top of window
    g.setColour (CandleColours::creamDim.withAlpha (0.4f));
    for (int i = 0; i < 3; ++i)
    {
        const float tx = inner.getX() + 10.0f + i * (inner.getWidth() - 20.0f) * 0.5f;
        g.drawLine (tx, inner.getY() + 2.0f, tx, inner.getY() + 5.0f, 0.8f);
    }

    // Two spools
    const float spoolR = inner.getHeight() * 0.34f;
    const float s1x    = inner.getX() + inner.getWidth() * 0.27f;
    const float s2x    = inner.getX() + inner.getWidth() * 0.73f;
    const float sy     = inner.getCentreY() + 1.5f;

    auto drawSpool = [&] (float cx, float cy, float angleOffset)
    {
        g.setColour (CandleColours::creamDim.withAlpha (0.45f));
        g.drawEllipse (cx - spoolR, cy - spoolR, spoolR * 2.0f, spoolR * 2.0f, 1.0f);
        const float hr = spoolR * 0.30f;
        g.setColour (CandleColours::creamDim.withAlpha (0.30f));
        g.fillEllipse (cx - hr, cy - hr, hr * 2.0f, hr * 2.0f);
        g.setColour (CandleColours::bg.withAlpha (0.5f));
        g.drawEllipse (cx - hr, cy - hr, hr * 2.0f, hr * 2.0f, 0.7f);
        for (int s = 0; s < 3; ++s)
        {
            const float a = s * juce::MathConstants<float>::twoPi / 3.0f + angleOffset;
            g.setColour (CandleColours::creamDim.withAlpha (0.22f));
            g.drawLine (cx + hr * std::cos (a), cy + hr * std::sin (a),
                        cx + spoolR * 0.83f * std::cos (a),
                        cy + spoolR * 0.83f * std::sin (a), 0.7f);
        }
    };
    drawSpool (s1x, sy, spoolAngle);
    drawSpool (s2x, sy, spoolAngle + juce::MathConstants<float>::pi * 0.4f);

    // Tape guide line
    g.setColour (CandleColours::creamDim.withAlpha (0.45f));
    g.drawLine (s1x + spoolR * 0.88f, sy - spoolR + 2.0f,
                s2x - spoolR * 0.88f, sy - spoolR + 2.0f, 1.2f);

    // "TASCAM 424" label
    g.setFont (lookAndFeel.getGeorgiaFont (6.5f, juce::Font::bold));
    g.setColour (CandleColours::creamDim.withAlpha (0.50f));
    g.drawText ("TASCAM  424", (int) inner.getX(), (int) (inner.getBottom() - 9),
                (int) inner.getWidth(), 9, juce::Justification::centred);
}

void CandleAudioProcessorEditor::drawVUMeter (juce::Graphics& g,
                                               juce::Rectangle<float> r, float vuDb)
{
    // Bezel
    g.setColour (CandleColours::vuBezel);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (CandleColours::panelBorder.withAlpha (0.4f));
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 0.8f);

    const auto face = r.reduced (3.5f, 3.5f);
    juce::ColourGradient faceGrad (CandleColours::vuFace.brighter (0.06f),
                                    face.getCentreX(), face.getY(),
                                    CandleColours::vuFace.darker (0.10f),
                                    face.getX(), face.getBottom(), true);
    g.setGradientFill (faceGrad);
    g.fillRoundedRectangle (face, 1.5f);

    // ── Real VU meters swing a long needle pivoting well below the glass, so
    // the visible arc near the top is broad and shallow rather than a tight
    // fan. Pivot is placed off-screen below the face and clipped to it.
    const float px        = face.getCentreX();
    const float needleLen = face.getHeight() * 2.0f;
    const float topY      = face.getY() + 2.5f;
    const float py        = topY + needleLen;

    const float halfW    = face.getWidth() * 0.5f - 3.0f;
    const float maxAngle = std::asin (juce::jlimit (-0.98f, 0.98f, halfW / needleLen));
    const float leftAngle  = -maxAngle;
    const float rightAngle =  maxAngle;

    constexpr float kMinDb = -24.0f, kMaxDb = 6.0f;   // slightly wider than the ticks so +/- glyphs sit outboard
    auto vuToAngle = [&] (float db)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (db - kMinDb) / (kMaxDb - kMinDb));
        return leftAngle + t * (rightAngle - leftAngle);
    };
    auto pointOnArc = [&] (float angle, float radius)
    {
        return juce::Point<float> (px + radius * std::sin (angle), py - radius * std::cos (angle));
    };

    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (face.toNearestInt());

        const float outerR = needleLen - 1.5f;
        const float tickInnerR  = outerR - 4.5f;
        const float labelR      = tickInnerR - 5.5f;

        // Scale arc: black across the working range, red over the last few dB
        juce::Path scaleArc;
        scaleArc.addCentredArc (px, py, outerR, outerR, 0.0f,
                                 vuToAngle (-20.0f), vuToAngle (0.0f), true);
        g.setColour (CandleColours::vuNeedle.withAlpha (0.55f));
        g.strokePath (scaleArc, juce::PathStrokeType (0.7f));

        juce::Path redArc;
        redArc.addCentredArc (px, py, outerR, outerR, 0.0f,
                               vuToAngle (0.0f), vuToAngle (3.0f), true);
        g.setColour (CandleColours::vuRed.withAlpha (0.85f));
        g.strokePath (redArc, juce::PathStrokeType (0.9f));

        struct Tick { float db; const char* label; bool red; };
        const Tick ticks[] = {
            { -20.0f, "20", false }, { -10.0f, "10", false }, { -7.0f, "7", false },
            { -5.0f,  "5",  false }, { -3.0f,  "3",  false }, { -1.0f, "1", false },
            {  0.0f,  "",   false }, {  3.0f,  "3",  true  },
        };

        g.setFont (lookAndFeel.getMonoFont (4.6f));
        for (const auto& t : ticks)
        {
            const float a  = vuToAngle (t.db);
            const auto  p1 = pointOnArc (a, tickInnerR);
            const auto  p2 = pointOnArc (a, outerR);
            g.setColour (t.red ? CandleColours::vuRed : CandleColours::vuNeedle.withAlpha (0.60f));
            g.drawLine (p1.x, p1.y, p2.x, p2.y, t.db == 0.0f ? 1.0f : 0.7f);

            if (t.label[0] != '\0')
            {
                const auto lp = pointOnArc (a, labelR);
                g.setColour (t.red ? CandleColours::vuRed : CandleColours::vuNeedle.withAlpha (0.70f));
                g.drawText (t.label, (int) (lp.x - 7), (int) (lp.y - 4), 14, 8,
                            juce::Justification::centred);
            }
        }

        // "-" / "+" end glyphs, outboard of the numbered ticks
        const auto minusP = pointOnArc (vuToAngle (kMinDb + 2.0f), labelR);
        const auto plusP  = pointOnArc (vuToAngle (kMaxDb - 2.0f), labelR);
        g.setColour (CandleColours::vuNeedle.withAlpha (0.60f));
        g.drawText ("-", (int) (minusP.x - 5), (int) (minusP.y - 4), 10, 8, juce::Justification::centred);
        g.setColour (CandleColours::vuRed.withAlpha (0.85f));
        g.drawText ("+", (int) (plusP.x - 5), (int) (plusP.y - 4), 10, 8, juce::Justification::centred);

        // Needle
        const float na = vuToAngle (juce::jlimit (-20.0f, 3.0f, vuDb));
        const auto  tip = pointOnArc (na, needleLen - 3.0f);
        g.setColour (CandleColours::vuNeedle.withAlpha (0.15f));
        g.drawLine (px + 0.6f, py + 0.6f, tip.x + 0.6f, tip.y + 0.6f, 1.1f);
        g.setColour (CandleColours::vuNeedle);
        g.drawLine (px, py, tip.x, tip.y, 0.8f);
        g.fillEllipse (px - 1.3f, py - 1.3f, 2.6f, 2.6f);
    }

    g.setFont (lookAndFeel.getGeorgiaFont (7.0f, juce::Font::bold | juce::Font::italic));
    g.setColour (CandleColours::vuNeedle.withAlpha (0.75f));
    g.drawText ("VU", face.withY (face.getBottom() - 10.0f).withHeight (10.0f),
                juce::Justification::centred);
}

void CandleAudioProcessorEditor::drawSegmentMeter (juce::Graphics& g,
                                                     juce::Rectangle<float> r,
                                                     float normLevel,
                                                     const juce::String& label)
{
    constexpr int kSegs = 8;
    const float sh = (r.getHeight() - 11.0f) / kSegs;
    const float sw = r.getWidth() - 2.0f;

    for (int i = 0; i < kSegs; ++i)
    {
        const float frac = 1.0f - static_cast<float> (i) / kSegs;
        const float segY = r.getY() + (r.getHeight() - 11.0f) - (i + 1) * sh + 1.0f;
        const bool  lit  = normLevel >= frac;
        juce::Colour c;
        if      (i >= kSegs - 1) c = lit ? CandleColours::meterRed   : CandleColours::meterOff;
        else if (i >= kSegs - 3) c = lit ? CandleColours::meterAmber : CandleColours::meterOff;
        else                     c = lit ? CandleColours::meterGreen  : CandleColours::meterOff;
        g.setColour (c);
        g.fillRect (r.getX() + 1.0f, segY, sw, sh - 1.0f);
    }
    g.setFont (lookAndFeel.getMonoFont (6.5f));
    g.setColour (CandleColours::creamDim);
    g.drawText (label, (int) r.getX(), (int)(r.getBottom() - 10), (int) r.getWidth(), 10,
                juce::Justification::centred);
}

void CandleAudioProcessorEditor::drawPanelSection (juce::Graphics& g,
                                                     juce::Rectangle<float> r,
                                                     const juce::String& title)
{
    const auto rail = r.reduced (6.0f, 8.0f);
    juce::ColourGradient grad (juce::Colour (0x8c160c07), rail.getX(), rail.getY(),
                               juce::Colour (0xff0e0703), rail.getX(), rail.getBottom(), false);
    grad.addColour (0.45, CandleColours::panel);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (rail, 4.0f);

    g.setColour (CandleColours::panelRule.withAlpha (0.35f));
    g.drawRoundedRectangle (rail, 4.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (rail.withHeight (1.0f).reduced (1.0f, 0.0f));
    g.setColour (juce::Colours::black.withAlpha (0.50f));
    g.fillRect (rail.withY (rail.getBottom() - 1.0f).withHeight (1.0f).reduced (1.0f, 0.0f));

    const float textW = juce::jmax (58.0f, (float) title.length() * 13.0f);
    const float lineY = rail.getY() + 16.0f;
    const float leftEnd = rail.getCentreX() - textW * 0.5f;
    const float rightStart = rail.getCentreX() + textW * 0.5f;

    g.setColour (CandleColours::panelRule.withAlpha (0.60f));
    g.drawLine (rail.getX() + 10.0f, lineY, leftEnd, lineY, 1.0f);
    g.drawLine (rightStart, lineY, rail.getRight() - 10.0f, lineY, 1.0f);

    juce::Rectangle<float> plate (rail.getCentreX() - textW * 0.5f, lineY - 9.0f, textW, 18.0f);
    g.setColour (juce::Colour (0x8c160c07));
    g.fillRect (plate);

    g.setFont (lookAndFeel.getGeorgiaFont (11.0f, juce::Font::bold));
    g.setColour (CandleColours::amber);
    g.drawText (title, plate.toNearestInt(), juce::Justification::centred);
}

void CandleAudioProcessorEditor::drawRubblesonicBadge (juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::Graphics::ScopedSaveState saved (g);
    g.addTransform (juce::AffineTransform::translation (r.getCentreX(), r.getCentreY())
                        .rotated (juce::degreesToRadians (-5.0f)));

    const float bw = r.getWidth();
    const float bh = r.getHeight();
    g.setColour (CandleColours::badgeHi);
    g.fillRoundedRectangle (-bw * 0.5f, -bh * 0.5f, bw, bh, 4.0f);
    g.setColour (juce::Colours::black.withAlpha (0.20f));
    g.drawRoundedRectangle (-bw * 0.5f, -bh * 0.5f, bw, bh, 4.0f, 1.0f);

    g.setColour (juce::Colour (0xf2373d33));
    g.fillEllipse (-21.0f, -21.0f, 42.0f, 42.0f);
    g.setColour (CandleColours::badgeText);
    g.setFont (lookAndFeel.getGeorgiaFont (16.0f, juce::Font::bold));
    g.drawText ("rubblesonic", (int) (-bw * 0.5f), -9, (int) bw, 20,
                juce::Justification::centred);
}
