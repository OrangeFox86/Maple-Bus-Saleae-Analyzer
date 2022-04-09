#include "MapleBusAnalyzer.h"
#include "MapleBusAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

MapleBusAnalyzer::MapleBusAnalyzer()
:	Analyzer2(),  
	mSettings( new MapleBusAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

MapleBusAnalyzer::~MapleBusAnalyzer()
{
	KillThread();
}

void MapleBusAnalyzer::SetupResults()
{
	mResults.reset( new MapleBusAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannelA );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannelB );
}

void MapleBusAnalyzer::AlignSerialMarkers()
{
    // Align the two sample numbers
    U64 aSample = mSerialA->GetSampleNumber();
    U64 bSample = mSerialB->GetSampleNumber();
    if( aSample > bSample )
    {
        mSerialB->AdvanceToAbsPosition( aSample );
    }
    else if( bSample > aSample )
    {
        mSerialA->AdvanceToAbsPosition( bSample );
    }
}

void MapleBusAnalyzer::AdvanceToNeutral()
{
    AlignSerialMarkers();

    // Wait until both serial lines are high
    while( mSerialA->GetBitState() == BIT_LOW || mSerialB->GetBitState() == BIT_LOW )
    {
        if( mSerialA->GetBitState() == BIT_LOW )
        {
            mSerialA->AdvanceToNextEdge();
            mSerialB->AdvanceToAbsPosition( mSerialA->GetSampleNumber() );
        }

        if( mSerialB->GetBitState() == BIT_LOW )
        {
            mSerialB->AdvanceToNextEdge();
            mSerialA->AdvanceToAbsPosition( mSerialB->GetSampleNumber() );
        }
    }

}

void MapleBusAnalyzer::WorkerThread()
{
	mSerialA = GetAnalyzerChannelData( mSettings->mInputChannelA );
    mSerialB = GetAnalyzerChannelData( mSettings->mInputChannelB );

    bool errorDetected = false;
	while(true)
    {
        AdvanceToNeutral();

        if(errorDetected)
        {
            // These markers tend to collide with markers that matter
            // mResults->AddMarker( mSerialA->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mInputChannelA );
            // mResults->AddMarker( mSerialB->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mInputChannelB );
            errorDetected = false;
        }

		mSerialA->AdvanceToNextEdge(); // falling edge -- beginning of the start sequence
        U64 startSample = mSerialA->GetSampleNumber();
        U64 endSample = mSerialA->GetSampleOfNextEdge(); // rising edge -- end of the start sequence
		
		// Ensure B clocks 4 times within the start and end
		for (U32 i = 0; i < 8 && !errorDetected; ++i)
		{
            U64 bSample = mSerialB->GetSampleOfNextEdge();

            if( i==0 )
            {
                // Wait until B starts clocking within A
                while( bSample < startSample )
                {
                    mSerialB->AdvanceToNextEdge();
                    bSample = mSerialB->GetSampleOfNextEdge();
                }
            }

            if( bSample >= endSample )
            {
                // B clocking didn't fall within the expected start sequence
                // TODO: log this
                errorDetected = true;
            }
            else
            {
                mSerialB->AdvanceToNextEdge();
            }
		}

        if(errorDetected)
        {
            // try again
            continue;
        }
        // check if there is another clock before endSample
        else if( mSerialB->GetSampleOfNextEdge() < endSample )
        {
            // Too many clocks on B channel
            // try again
            errorDetected = true;
            // TODO: log this
            continue;
        }

        // If we made it here, a valid start sequence was detected
        mSerialA->AdvanceToAbsPosition( endSample );
        mSerialB->AdvanceToAbsPosition( endSample );
        mResults->AddMarker( startSample, AnalyzerResults::Start, mSettings->mInputChannelA );
        mResults->AddMarker( startSample, AnalyzerResults::Start, mSettings->mInputChannelB );

        bool endDetected = false;
        errorDetected = false;
        while( !endDetected && !errorDetected )
        {
            U64 startingSample = mSerialA->GetSampleNumber();
            bool aIsClock = true;
            U8 b = 0;
            U8 mask = 1 << 7;
            for( U32 i = 0; i < 8 && !endDetected && !errorDetected; i++, aIsClock = !aIsClock, mask = mask >> 1 )
            {
                AnalyzerChannelData* clock = nullptr;
                Channel clockChannel;
                AnalyzerChannelData* data = nullptr;
                Channel dataChannel;

                if( aIsClock )
                {
                    clock = mSerialA;
                    clockChannel = mSettings->mInputChannelA;
                    data = mSerialB;
                    dataChannel = mSettings->mInputChannelB;
                }
                else
                {
                    data = mSerialA;
                    dataChannel = mSettings->mInputChannelA;
                    clock = mSerialB;
                    clockChannel = mSettings->mInputChannelB;
                }

                if( clock->GetBitState() == BIT_LOW )
                {
                    // Need to wait for clock to transition to high first
                    clock->AdvanceToNextEdge();
                }

                clock->AdvanceToNextEdge();
                U64 clockEdgeSample = clock->GetSampleNumber();





                
                U32 numDataEdges = 0;
                if( i == 0 )
                {
                    // TODO: This needs to be cleaned up.
                    // This is very very rough. I wrote it in a way that helped me debug.

                    // Check for END
                    while( data->GetSampleOfNextEdge() < clockEdgeSample )
                    {
                        data->AdvanceToNextEdge();
                        numDataEdges++;
                    }
                    if( data->GetBitState() == BIT_LOW )
                    {
                        // Could be an end state
                        U64 markerBegin = data->GetSampleNumber();
                        U64 markerEnd = data->GetSampleOfNextEdge();
                        if (clock->GetSampleOfNextEdge() < markerEnd)
                        {
                            // This goes to rising edge
                            clock->AdvanceToNextEdge();
                            if( clock->GetSampleOfNextEdge() < markerEnd )
                            {
                                // Either error or end detected
                                // Goes to falling edgs
                                clock->AdvanceToNextEdge();
                                if( clock->GetSampleOfNextEdge() < markerEnd )
                                {
                                    clock->AdvanceToNextEdge();
                                    if( clock->GetSampleOfNextEdge() > markerEnd )
                                    {
                                        endDetected = true;
                                        mResults->AddMarker( markerEnd, AnalyzerResults::Stop, mSettings->mInputChannelA );
                                        mResults->AddMarker( markerEnd, AnalyzerResults::Stop, mSettings->mInputChannelB );
                                        continue;
                                    }
                                    else
                                    {
                                        errorDetected = true;
                                        continue;
                                    }
                                }
                                else
                                {
                                    errorDetected = true;
                                    continue;
                                }
                            }
                            else if( numDataEdges <= 1 )
                            {
                                // Mistakes were made - this looks like a normal bit, and it was 0
                                AlignSerialMarkers();
                                continue;
                            }
                            else
                            {
                                errorDetected = true;
                                continue;
                            }
                        }
                    }
                }







                while( numDataEdges < 2 && data->GetSampleOfNextEdge() < clockEdgeSample )
                {
                    if( numDataEdges < 1 )
                    {
                        data->AdvanceToNextEdge();
                    }
                    numDataEdges++;
                }

                if( numDataEdges > 1 )
                {
                    // More than 1 data edge before clock is not allowed
                    errorDetected = true;
                }
                else
                {
                    data->AdvanceToAbsPosition( clockEdgeSample );
                    clock->AdvanceToAbsPosition( clockEdgeSample );
                    if( data->GetBitState() == BIT_HIGH )
                    {
                        b |= mask;
                    }
                    // let's put markers exactly where we sample this bit:
                    // mResults->AddMarker( clockEdgeSample, AnalyzerResults::Dot, dataChannel );
                    mResults->AddMarker( clockEdgeSample, AnalyzerResults::DownArrow, clockChannel );
                }
            }

            if( !endDetected && !errorDetected )
            {
                // we have a byte to save.
                Frame frame;
                frame.mData1 = b;
                frame.mFlags = 0;
                frame.mStartingSampleInclusive = startingSample;
                frame.mEndingSampleInclusive = mSerialB->GetSampleNumber();

                mResults->AddFrame( frame );
                mResults->CommitResults();
                ReportProgress( frame.mEndingSampleInclusive );
            }
        }
	}
}

bool MapleBusAnalyzer::NeedsRerun()
{
	return false;
}

U32 MapleBusAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 MapleBusAnalyzer::GetMinimumSampleRateHz()
{
    // Somewhat arbitrary
	return 12000000;
}

const char* MapleBusAnalyzer::GetAnalyzerName() const
{
	return "Maple Bus";
}

const char* GetAnalyzerName()
{
	return "Maple Bus";
}

Analyzer* CreateAnalyzer()
{
	return new MapleBusAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}