#include "MapleBusAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


MapleBusAnalyzerSettings::MapleBusAnalyzerSettings()
:	mInputChannelA( UNDEFINED_CHANNEL ), mInputChannelB( UNDEFINED_CHANNEL )
{
	mInputChannelAInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelAInterface->SetTitleAndTooltip( "SDCKA", "Serial Data and Clock Line A" );
	mInputChannelAInterface->SetChannel( mInputChannelA );

    mInputChannelBInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelBInterface->SetTitleAndTooltip( "SDCKB", "Serial Data and Clock Line B" );
    mInputChannelBInterface->SetChannel( mInputChannelB );

	AddInterface( mInputChannelAInterface.get() );
    AddInterface( mInputChannelBInterface.get() );

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

	ClearChannels();
    AddChannel( mInputChannelA, "SDCKA", true );
    AddChannel( mInputChannelB, "SDCKB", true );

	return true;
}

void MapleBusAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelAInterface->SetChannel( mInputChannelA );
    mInputChannelBInterface->SetChannel( mInputChannelB );
}

void MapleBusAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannelA;
    text_archive >> mInputChannelB;

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

	return SetReturnString( text_archive.GetString() );
}
