
#ifndef __JUCE_PLUGIN_CHARACTERISTICS_H__
#define __JUCE_PLUGIN_CHARACTERISTICS_H__

#define JUCE_VST3_CAN_REPLACE_VST2 0

#define JucePlugin_Name                     "Vitality"
#define JucePlugin_Desc                     ""
#define JucePlugin_Manufacturer             "KB1RD"
#define JucePlugin_ManufacturerCode         'KBRD'
#define JucePlugin_ManufacturerEmail        "kb1rd@kb1rd.net"
#define JucePlugin_ManufacturerWebsite      "kb1rd.net"
#define JucePlugin_PluginCode               'Vitl'

#define JucePlugin_VersionCode              0x000100
#define JucePlugin_VersionString            "0.1.0"

#define JucePlugin_MaxNumInputChannels              0
#define JucePlugin_MaxNumOutputChannels             2
#define JucePlugin_PreferredChannelConfigurations   { 0, 2 }
#define JucePlugin_IsSynth                          1
#define JucePlugin_WantsMidiInput                   1
#define JucePlugin_ProducesMidiOutput               0
#define JucePlugin_SilenceInProducesSilenceOut      0
#define JucePlugin_EditorRequiresKeyboardFocus      1 // pluginEditorRequiresKeys
#define JucePlugin_IsMidiEffect                     0

// aaxIdentifier="audio.vial.synth"
// pluginAUExportPrefix="vial"
// pluginRTASCategory=""
// pluginAAXCategory="2048"

#define JucePlugin_VSTUniqueID              JucePlugin_PluginCode
#define JucePlugin_VSTCategory              kPlugCategSynth

#define JucePlugin_LV2URI                   "urn:net:kb1rd:vitality"
#define JucePlugin_LV2Category              "InstrumentPlugin"
#define JucePlugin_WantsLV2Latency          0
#define JucePlugin_WantsLV2State            1
#define JucePlugin_WantsLV2TimePos          1
#define JucePlugin_WantsLV2Presets          0

#endif
