#include "MapleBusAnalyzer.h"
#include "MapleBusAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <string>
#include <sstream>

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
    MapleBusAnalyzerResults::Type analyzerType;
    if( mSettings->mOutputStyle == 0 )
    {
        analyzerType = MapleBusAnalyzerResults::Type::BYTE;
    }
    else if( mSettings->mOutputStyle == 1 )
    {
        analyzerType = MapleBusAnalyzerResults::Type::WORD;
    }
    else if( mSettings->mOutputStyle == 2 )
    {
        analyzerType = MapleBusAnalyzerResults::Type::WORD_BYTES;
    }
    mResults.reset( new MapleBusAnalyzerResults( this, mSettings.get(), analyzerType ) );
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
        if( errorDetected )
        {
            // These markers tend to collide with markers that matter
            // mResults->AddMarker( mSerialA->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mInputChannelA );
            // mResults->AddMarker( mSerialB->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mInputChannelB );
        }

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
        S32 totalBytesExpected = -1;
        S32 totalWordsExpected = -1;
        S32 numBytesLeftExpected = -1;
        S32 numWordsLeftExpected = -1;
        U32 byteCount = 0;
        U32 word = 0;
        U64 wordStartingSample = 0;
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

                // Go to clock falling edge
                clock->AdvanceToNextEdge();
                U64 clockEdgeSample = clock->GetSampleNumber();

                U32 numDataEdges = 0;
                if( i == 0 )
                {
                    // Check for END
                    while( numDataEdges < 3 && data->GetSampleOfNextEdge() < clockEdgeSample )
                    {
                        if( numDataEdges < 2 )
                        {
                            data->AdvanceToNextEdge();
                        }
                        numDataEdges++;
                    }
                    if( numDataEdges == 2 )
                    {
                        // Either we reached the end or error detected
                        U64 markerBegin = data->GetSampleNumber();
                        U64 markerEnd = data->GetSampleOfNextEdge();
                        U32 numClockEdges = 0;

                        for( ; numClockEdges < 3 && clock->GetSampleOfNextEdge() < markerEnd; ++numClockEdges )
                        {
                            // Go to rising then falling then rising
                            clock->AdvanceToNextEdge();
                        }

                        if( numClockEdges == 3 && clock->GetSampleOfNextEdge() > markerEnd )
                        {
                            endDetected = true;
                            mResults->AddMarker( markerEnd, AnalyzerResults::Stop, mSettings->mInputChannelA );
                            mResults->AddMarker( markerEnd, AnalyzerResults::Stop, mSettings->mInputChannelB );
                            mSerialA->AdvanceToAbsPosition( markerEnd );
                            mSerialB->AdvanceToAbsPosition( markerEnd );
                        }
                        else
                        {
                            errorDetected = true;
                        }
                        continue;
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
                ++byteCount;

                if( byteCount == 1 )
                {
                    // This is the first byte which tells us how many extra 32-bit words to expect
                    totalBytesExpected = b * 4;
                    // Add 4 bytes for the first frame
                    totalBytesExpected += 4;
                    // Add 1 byte for the CRC value
                    totalBytesExpected += 1;

                    numBytesLeftExpected = totalBytesExpected - 1;
                }
                else if( numBytesLeftExpected > 0 )
                {
                    --numBytesLeftExpected;
                }

                if( mResults->mType == MapleBusAnalyzerResults::Type::BYTE )
                {
                    Frame frame;
                    frame.mData1 = b;
                    frame.mData2 = numBytesLeftExpected;
                    frame.mFlags = 0;
                    frame.mStartingSampleInclusive = startingSample;
                    frame.mEndingSampleInclusive = mSerialB->GetSampleNumber();

                    mResults->AddFrame( frame );
                    mResults->CommitResults();
                    ReportProgress( frame.mEndingSampleInclusive );
                }

                // Build word for little endian
                word = word >> 8;
                word |= (static_cast<U32>( b ) << 24);

                if( byteCount == 1 )
                {
                    totalWordsExpected = b + 1;
                    numWordsLeftExpected = totalWordsExpected;
                }
                else if( byteCount % 4 == 0 )
                {
                    // We have a word to save
                    if( numWordsLeftExpected > 0 )
                    {
                        --numWordsLeftExpected;
                    }

                    if( mResults->mType == MapleBusAnalyzerResults::Type::WORD || 
                        mResults->mType == MapleBusAnalyzerResults::Type::WORD_BYTES )
                    {
                        Frame frame;
                        frame.mData1 = word;
                        frame.mData2 = numWordsLeftExpected;
                        if( byteCount == 4 )
                        {
                            frame.mData2 |= ( static_cast<U64>( 1 ) << 32 );
                        }
                        frame.mStartingSampleInclusive = wordStartingSample;
                        frame.mEndingSampleInclusive = mSerialB->GetSampleNumber();

                        mResults->AddFrame( frame );
                        mResults->CommitResults();
                    }
                    word = 0;
                }
                else if( numBytesLeftExpected == 0 )
                {
                    if( mResults->mType == MapleBusAnalyzerResults::Type::WORD ||
                        mResults->mType == MapleBusAnalyzerResults::Type::WORD_BYTES )
                    {
                        // CRC byte
                        Frame frame;
                        frame.mData1 = b;
                        frame.mData2 = ( static_cast<U64>( 2 ) << 32 );
                        frame.mStartingSampleInclusive = startingSample;
                        frame.mEndingSampleInclusive = mSerialB->GetSampleNumber();

                        mResults->AddFrame( frame );
                        mResults->CommitResults();
                    }
                }

                if( byteCount % 4 == 1 )
                {
                    wordStartingSample = startingSample;
                }
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