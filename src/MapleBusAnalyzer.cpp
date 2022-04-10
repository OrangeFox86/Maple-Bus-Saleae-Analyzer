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
    if( mSettings->mOutputStyle == MapleBusAnalyzerSettings::OUTPUT_STYLE_EACH_WORD )
    {
        analyzerType = MapleBusAnalyzerResults::Type::WORD;
    }
    else if( mSettings->mOutputStyle == MapleBusAnalyzerSettings::OUTPUT_STYLE_WORD_BYTES )
    {
        analyzerType = MapleBusAnalyzerResults::Type::WORD_BYTES;
    }
    else if( mSettings->mOutputStyle == MapleBusAnalyzerSettings::OUTPUT_STYLE_WORD_BYTES_LE )
    {
        analyzerType = MapleBusAnalyzerResults::Type::WORD_BYTES_LE;
    }
    else
    {
        analyzerType = MapleBusAnalyzerResults::Type::BYTE;
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

void MapleBusAnalyzer::AdvanceToNextStart()
{
    bool startFound = false;
    while (!startFound)
    {
        AdvanceToNeutral();

        mSerialA->AdvanceToNextEdge(); // falling edge -- beginning of the start sequence
        U64 startSample = mSerialA->GetSampleNumber();
        U64 endSample = mSerialA->GetSampleOfNextEdge(); // rising edge -- end of the start sequence

        // Advance B to just before the start
        mSerialB->AdvanceToAbsPosition(startSample - 1);

        // Ensure B clocks 4 times within the start and end without advancing more than we need to
        bool errorDetected = false;
        for (U32 i = 0; i < 8 && !errorDetected; ++i)
        {
            U64 bSample = mSerialB->GetSampleOfNextEdge();

            if (bSample >= endSample)
            {
                // B clocking didn't fall within the expected start sequence
                errorDetected = true;
            }
            else
            {
                mSerialB->AdvanceToNextEdge();
            }
        }

        // make sure there isn't another clock before endSample
        if (!errorDetected && !mSerialB->WouldAdvancingToAbsPositionCauseTransition(endSample))
        {
            // If we made it here, a valid start sequence was detected
            mSerialA->AdvanceToAbsPosition(endSample);
            mSerialB->AdvanceToAbsPosition(endSample);
            mResults->AddMarker(startSample, AnalyzerResults::Start, mSettings->mInputChannelA);
            mResults->AddMarker(startSample, AnalyzerResults::Start, mSettings->mInputChannelB);
            startFound = true;
        }
        else
        {
            // TODO: Log error
        }
    }
}

S32 MapleBusAnalyzer::CheckForEnd(AnalyzerChannelData* clock, AnalyzerChannelData* data)
{
    U64 clockEdgeSample = clock->GetSampleNumber();
    U32 numDataEdges = data->AdvanceToAbsPosition(clockEdgeSample);
    bool endDetected = false;
    bool errorDetected = false;

    if (numDataEdges == 2)
    {
        // Either we are reaching the end or error detected

        U64 markerEnd = data->GetSampleOfNextEdge();
        U32 numClockEdges = 0;

        for (; numClockEdges < 3 && clock->GetSampleOfNextEdge() < markerEnd; ++numClockEdges)
        {
            // Go to rising then falling then rising
            clock->AdvanceToNextEdge();
        }

        if (numClockEdges == 3 && !clock->WouldAdvancingToAbsPositionCauseTransition(markerEnd))
        {
            endDetected = true;
            mResults->AddMarker(markerEnd, AnalyzerResults::Stop, mSettings->mInputChannelA);
            mResults->AddMarker(markerEnd, AnalyzerResults::Stop, mSettings->mInputChannelB);
            mSerialA->AdvanceToAbsPosition(markerEnd);
            mSerialB->AdvanceToAbsPosition(markerEnd);
        }
        else
        {
            errorDetected = true;
        }
    }
    else if (numDataEdges > 2)
    {
        errorDetected = true;
    }

    if (errorDetected)
    {
        return -1;
    }
    else if (endDetected)
    {
        return 1;
    }
    else
    {
        return 0;
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
            // TODO: log error
            errorDetected = false;
        }

        AdvanceToNextStart();

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

                if( i == 0 )
                {
                    U32 checkStatus = CheckForEnd(clock, data);
                    if (checkStatus < 0)
                    {
                        errorDetected = true;
                    }
                    else if (checkStatus > 0)
                    {
                        endDetected = true;
                    }
                }
                else
                {
                    U32 numDataEdges = 0;
                    while (numDataEdges < 2 && data->GetSampleOfNextEdge() < clockEdgeSample)
                    {
                        if (numDataEdges < 1)
                        {
                            data->AdvanceToNextEdge();
                        }
                        numDataEdges++;
                    }

                    if (numDataEdges > 1)
                    {
                        // More than 1 data edge before clock is not allowed
                        errorDetected = true;
                    }
                }

                if (!endDetected && !errorDetected)
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
                    MapleBusAnalyzerResults::WordType wordType = MapleBusAnalyzerResults::WORD_TYPE_DATA;
                    if( byteCount < 5 )
                    {
                        wordType = MapleBusAnalyzerResults::WORD_TYPE_FRAME;
                    }
                    else if( numBytesLeftExpected == 0 )
                    {
                        wordType = MapleBusAnalyzerResults::WORD_TYPE_CRC;
                    }
                    frame.mData2 = MapleBusAnalyzerResults::EncodeData2( numBytesLeftExpected, wordType );
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
                        mResults->mType == MapleBusAnalyzerResults::Type::WORD_BYTES ||
                        mResults->mType == MapleBusAnalyzerResults::Type::WORD_BYTES_LE )
                    {
                        Frame frame;
                        frame.mData1 = word;
                        MapleBusAnalyzerResults::WordType wordType = MapleBusAnalyzerResults::WORD_TYPE_DATA;
                        if( byteCount == 4 )
                        {
                            wordType = MapleBusAnalyzerResults::WORD_TYPE_FRAME;
                        }
                        frame.mData2 = MapleBusAnalyzerResults::EncodeData2( numWordsLeftExpected, wordType );
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
                        mResults->mType == MapleBusAnalyzerResults::Type::WORD_BYTES ||
                        mResults->mType == MapleBusAnalyzerResults::Type::WORD_BYTES_LE )
                    {
                        // CRC byte
                        Frame frame;
                        frame.mData1 = b;
                        frame.mData2 = MapleBusAnalyzerResults::EncodeData2( 0, MapleBusAnalyzerResults::WORD_TYPE_CRC );
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