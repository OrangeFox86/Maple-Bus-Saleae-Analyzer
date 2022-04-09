#include "MapleBusAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


MapleBusAnalyzerSettings::MapleBusAnalyzerSettings()
    : mInputChannelA( UNDEFINED_CHANNEL ), mInputChannelB( UNDEFINED_CHANNEL ), mOutputStyle( 0 )
{
	mInputChannelAInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelAInterface->SetTitleAndTooltip( "SDCKA", "Serial Data and Clock Line A" );
	mInputChannelAInterface->SetChannel( mInputChannelA );

    mInputChannelBInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelBInterface->SetTitleAndTooltip( "SDCKB", "Serial Data and Clock Line B" );
    mInputChannelBInterface->SetChannel( mInputChannelB );

	mOutputStyleInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mOutputStyleInterface->SetTitleAndTooltip( "Output Style", "Bubble Text Output Style" );
    mOutputStyleInterface->AddNumber( OUTPUT_STYLE_EACH_BYTE, "Each Byte", "Show each byte" );
    mOutputStyleInterface->AddNumber( OUTPUT_STYLE_EACH_WORD, "Each Word (little endian)", "Show each word (little endian)" );
    mOutputStyleInterface->AddNumber( OUTPUT_STYLE_WORD_BYTES, "Word Bytes", "Show bytes, grouped by word" );
    mOutputStyleInterface->SetNumber( mOutputStyle );

	AddInterface( mInputChannelAInterface.get() );
    AddInterface( mInputChannelBInterface.get() );
    AddInterface( mOutputStyleInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
    AddChannel( mInputChannelA, "SDCKA", false );
    AddChannel( mInputChannelB, "SDCKB", false );
}

MapleBusAnalyzerSettings::~MapleBusAnalyzerSettings()
{
}

bool MapleBusAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannelA = mInputChannelAInterface->GetChannel();
    mInputChannelB = mInputChannelBInterface->GetChannel();
    mOutputStyle = mOutputStyleInterface->GetNumber();

	ClearChannels();
    AddChannel( mInputChannelA, "SDCKA", true );
    AddChannel( mInputChannelB, "SDCKB", true );

	return true;
}

void MapleBusAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelAInterface->SetChannel( mInputChannelA );
    mInputChannelBInterface->SetChannel( mInputChannelB );
    mOutputStyleInterface->SetNumber( mOutputStyle );
}

void MapleBusAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannelA;
    text_archive >> mInputChannelB;
    text_archive >> mOutputStyle;

	ClearChannels();
    AddChannel( mInputChannelA, "SDCKA", true );
    AddChannel( mInputChannelB, "SDCKB", true );

	UpdateInterfacesFromSettings();
}

const char* MapleBusAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannelA;
    text_archive << mInputChannelB;
    text_archive << mOutputStyle;

	return SetReturnString( text_archive.GetString() );
}
