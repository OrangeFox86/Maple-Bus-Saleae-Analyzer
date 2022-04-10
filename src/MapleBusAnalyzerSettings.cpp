#include "MapleBusAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


MapleBusAnalyzerSettings::MapleBusAnalyzerSettings()
    : mInputChannelA(UNDEFINED_CHANNEL), mInputChannelB(UNDEFINED_CHANNEL), mOutputStyle(OUTPUT_STYLE_WORD_BYTES_LE)
{
    mInputChannelAInterface.reset(new AnalyzerSettingInterfaceChannel());
    mInputChannelAInterface->SetTitleAndTooltip("SDCKA", "Serial Data and Clock Line A");
    mInputChannelAInterface->SetChannel(mInputChannelA);

    mInputChannelBInterface.reset(new AnalyzerSettingInterfaceChannel());
    mInputChannelBInterface->SetTitleAndTooltip("SDCKB", "Serial Data and Clock Line B");
    mInputChannelBInterface->SetChannel(mInputChannelB);

    mOutputStyleInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mOutputStyleInterface->SetTitleAndTooltip("Output Style", "Bubble Text Output Style");
    mOutputStyleInterface->AddNumber(OUTPUT_STYLE_EACH_BYTE, "Each Byte", "Show each byte");
    mOutputStyleInterface->AddNumber(OUTPUT_STYLE_EACH_WORD, "Each Word (little endian)", "Show each word (little endian)");
    mOutputStyleInterface->AddNumber(OUTPUT_STYLE_WORD_BYTES, "Word Bytes", "Show bytes, grouped by word");
    mOutputStyleInterface->AddNumber(OUTPUT_STYLE_WORD_BYTES_LE, "Word Bytes (little endian)",
                                     "Show bytes, grouped by word, little endian sorted");
    mOutputStyleInterface->SetNumber(mOutputStyle);

    AddInterface(mInputChannelAInterface.get());
    AddInterface(mInputChannelBInterface.get());
    AddInterface(mOutputStyleInterface.get());

    AddExportOption(0, "Export as text/csv file");
    AddExportExtension(0, "text", "txt");
    AddExportExtension(0, "csv", "csv");

    ClearChannels();
    AddChannel(mInputChannelA, "SDCKA", false);
    AddChannel(mInputChannelB, "SDCKB", false);
}

MapleBusAnalyzerSettings::~MapleBusAnalyzerSettings()
{
}

MapleBusAnalyzerSettings::OutputStyleNumber MapleBusAnalyzerSettings::NumberToOutputStyle(double num)
{
    OutputStyleNumber returnValue = OUTPUT_STYLE_WORD_BYTES_LE;
    // Round to nearest integer and cast to enum
    U32 outputStyleInt = int(num + 0.5);
    if (outputStyleInt < OUTPUT_STYLE_COUNT)
    {
        returnValue = static_cast<OutputStyleNumber>(outputStyleInt);
    }
    return returnValue;
}

bool MapleBusAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannelA = mInputChannelAInterface->GetChannel();
    mInputChannelB = mInputChannelBInterface->GetChannel();
    mOutputStyle = NumberToOutputStyle(mOutputStyleInterface->GetNumber());

    ClearChannels();
    AddChannel(mInputChannelA, "SDCKA", true);
    AddChannel(mInputChannelB, "SDCKB", true);

    return true;
}

void MapleBusAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelAInterface->SetChannel(mInputChannelA);
    mInputChannelBInterface->SetChannel(mInputChannelB);
    mOutputStyleInterface->SetNumber(mOutputStyle);
}

void MapleBusAnalyzerSettings::LoadSettings(const char* settings)
{
    SimpleArchive text_archive;
    text_archive.SetString(settings);

    text_archive >> mInputChannelA;
    text_archive >> mInputChannelB;
    U32 outputStyleInt = 0;
    text_archive >> outputStyleInt;
    mOutputStyle = NumberToOutputStyle(outputStyleInt);

    ClearChannels();
    AddChannel(mInputChannelA, "SDCKA", true);
    AddChannel(mInputChannelB, "SDCKB", true);

    UpdateInterfacesFromSettings();
}

const char* MapleBusAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mInputChannelA;
    text_archive << mInputChannelB;
    text_archive << mOutputStyle;

    return SetReturnString(text_archive.GetString());
}
