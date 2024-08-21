/*
  ==============================================================================

   Main symbol/entry for juce plugins

  ==============================================================================
*/

#include "AppConfig.h"

#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
 #define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#endif
#include "modules/juce_gui_basics/juce_gui_basics.h"
#undef None

#include "JucePluginMain.h"

#if JucePlugin_Build_AU
 #include "modules/juce_audio_plugin_client/AU/juce_AU_Wrapper.mm"

#elif JucePlugin_Build_LV2
 #if JUCE_MAJOR_VERSION >= 7
  #include "modules/juce_audio_plugin_client/juce_audio_plugin_client_LV2.cpp"
 #else
  #include "modules/juce_audio_plugin_client/LV2/juce_LV2_Wrapper.cpp"
 #endif

#elif JucePlugin_Build_RTAS
 #include "modules/juce_audio_plugin_client/RTAS/juce_RTAS_Wrapper.cpp"

#elif JucePlugin_Build_VST
 // we need to include 'juce_VSTMidiEventList' before 'juce_VST_Wrapper'
 namespace Vst2 {
 #include "modules/juce_audio_processors/format_types/juce_VSTInterface.h"
 }
 #include "modules/juce_audio_processors/format_types/juce_VSTMidiEventList.h"
 #if JUCE_MAJOR_VERSION >= 7
  #include "modules/juce_audio_plugin_client/juce_audio_plugin_client_VST2.cpp"
 #else
  #include "modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.cpp"
 #endif

#elif JucePlugin_Build_VST3
 #if JUCE_MAJOR_VERSION >= 7
  #include "modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp"
 #else
  #include "modules/juce_audio_plugin_client/VST3/juce_VST3_Wrapper.cpp"
 #endif

#elif JucePlugin_Build_Standalone
 #include "modules/juce_audio_plugin_client/Standalone/juce_StandaloneFilterApp.cpp"

  // I couldn't figure out the JUCE way of doing this... Many of their macros just error out
  // Anyway, this works, so whatever
  juce::JUCEApplicationBase* juce_CreateApplication() {
      return new juce::StandaloneFilterApp();
  }

  JUCE_MAIN_FUNCTION_DEFINITION
#else
 #error Invalid configuration
#endif
