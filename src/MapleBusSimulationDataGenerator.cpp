#include "MapleBusSimulationDataGenerator.h"
#include "MapleBusAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

MapleBusSimulationDataGenerator::MapleBusSimulationDataGenerator()
:	mSerialText( "My first analyzer, woo hoo!" ),
	mStringIndex( 0 )
{
}

MapleBusSimulationDataGenerator::~MapleBusSimulationDataGenerator()
{
}

void MapleBusSimulationDataGenerator::Initialize( U32 simulation_sample_rate, MapleBusAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mSerialASimulationData.SetChannel( mSettings->mInputChannelA );
	mSerialASimulationData.SetSampleRate( simulation_sample_rate );
    mSerialASimulationData.SetInitialBitState( BIT_HIGH );

    mSerialBSimulationData.SetChannel( mSettings->mInputChannelB );
    mSerialBSimulationData.SetSampleRate( simulation_sample_rate );
    mSerialBSimulationData.SetInitialBitState( BIT_HIGH );
}

U32 MapleBusSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mSerialASimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		CreateSerialByte();
	}

	*simulation_channel = &mSerialASimulationData;
    *( simulation_channel + 1 ) = &mSerialBSimulationData;
	return 1;
}

void MapleBusSimulationDataGenerator::CreateSerialByte()
{
#if 0
	U32 samples_per_bit = mSimulationSampleRateHz / mSettings->mBitRate;

	U8 byte = mSerialText[ mStringIndex ];
	mStringIndex++;
	if( mStringIndex == mSerialText.size() )
		mStringIndex = 0;

	//we're currenty high
	//let's move forward a little
	mSerialSimulationData.Advance( samples_per_bit * 10 );

	mSerialSimulationData.Transition();  //low-going edge for start bit
	mSerialSimulationData.Advance( samples_per_bit );  //add start bit time

	U8 mask = 0x1 << 7;
	for( U32 i=0; i<8; i++ )
	{
		if( ( byte & mask ) != 0 )
			mSerialSimulationData.TransitionIfNeeded( BIT_HIGH );
		else
			mSerialSimulationData.TransitionIfNeeded( BIT_LOW );

		mSerialSimulationData.Advance( samples_per_bit );
		mask = mask >> 1;
	}

	mSerialSimulationData.TransitionIfNeeded( BIT_HIGH ); //we need to end high

	//lets pad the end a bit for the stop bit:
	mSerialSimulationData.Advance( samples_per_bit );
#endif
}
