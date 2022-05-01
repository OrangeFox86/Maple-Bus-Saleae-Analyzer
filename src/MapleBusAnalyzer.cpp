#include "MapleBusAnalyzer.h"
#include "MapleBusAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <string>
#include <sstream>

MapleBusAnalyzer::MapleBusAnalyzer() : Analyzer2(), mSettings(new MapleBusAnalyzerSettings()), mSimulationInitilized(false)
{
    SetAnalyzerSettings(mSettings.get());
    ResetPacketData();
}

MapleBusAnalyzer::~MapleBusAnalyzer()
{
    KillThread();
}

void MapleBusAnalyzer::SetupResults()
{
    MapleBusAnalyzerResults::DataFormat analyzerType;
    switch (mSettings->mOutputStyle)
    {
    case MapleBusAnalyzerSettings::OUTPUT_STYLE_EACH_BYTE:
        analyzerType = MapleBusAnalyzerResults::DataFormat::BYTE;
        break;

    case MapleBusAnalyzerSettings::OUTPUT_STYLE_EACH_WORD:
        analyzerType = MapleBusAnalyzerResults::DataFormat::WORD;
        break;

    case MapleBusAnalyzerSettings::OUTPUT_STYLE_WORD_BYTES:
        analyzerType = MapleBusAnalyzerResults::DataFormat::WORD_BYTES;
        break;

    default:
    case MapleBusAnalyzerSettings::OUTPUT_STYLE_WORD_BYTES_LE:
        analyzerType = MapleBusAnalyzerResults::DataFormat::WORD_BYTES_LE;
        break;
    }
    mResults.reset(new MapleBusAnalyzerResults(this, mSettings.get(), analyzerType));
    SetAnalyzerResults(mResults.get());
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannelA);
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannelB);
}

void MapleBusAnalyzer::LogError()
{
    // TODO
}

void MapleBusAnalyzer::AlignSerialMarkers()
{
    // Align the two sample numbers
    U64 aSample = mSerialA->GetSampleNumber();
    U64 bSample = mSerialB->GetSampleNumber();
    if (aSample > bSample)
    {
        mSerialB->AdvanceToAbsPosition(aSample);
    }
    else if (bSample > aSample)
    {
        mSerialA->AdvanceToAbsPosition(bSample);
    }
}

void MapleBusAnalyzer::AdvanceToNeutral()
{
    AlignSerialMarkers();

    // Wait until both serial lines are high
    while (mSerialA->GetBitState() == BIT_LOW || mSerialB->GetBitState() == BIT_LOW)
    {
        if (mSerialA->GetBitState() == BIT_LOW)
        {
            mSerialA->AdvanceToNextEdge();
            mSerialB->AdvanceToAbsPosition(mSerialA->GetSampleNumber());
        }

        if (mSerialB->GetBitState() == BIT_LOW)
        {
            mSerialB->AdvanceToNextEdge();
            mSerialA->AdvanceToAbsPosition(mSerialB->GetSampleNumber());
        }
    }
}

void MapleBusAnalyzer::ResetPacketData()
{
    mTotalBytesExpected = -1;
    mTotalWordsExpected = -1;
    mNumBytesLeftExpected = -1;
    mNumWordsLeftExpected = -1;
    mByteCount = 0;
    mCurrentWord = 0;
    mWordStartingSample = 0;
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
            // End sequence is not complete until B transitions back low
            mSerialA->AdvanceToAbsPosition(endSample);
            endSample = mSerialB->GetSampleOfNextEdge();
            if (!mSerialA->WouldAdvancingToAbsPositionCauseTransition(endSample))
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
                LogError();
            }
        }
        else
        {
            LogError();
        }
    }
}

S32 MapleBusAnalyzer::CheckForEnd(AnalyzerChannelData* clock, AnalyzerChannelData* data, U32 numDataEdges)
{
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
            clock->AdvanceToAbsPosition(markerEnd);
            data->AdvanceToAbsPosition(markerEnd);
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

void MapleBusAnalyzer::SaveByte(U64 startingSample, U8 theByte)
{
    ++mByteCount;

    // Build word (little endian)
    mCurrentWord = mCurrentWord >> 8;
    mCurrentWord |= (static_cast<U32>(theByte) << 24);

    if (mByteCount == 1)
    {
        // This is the first byte which tells us how many extra 32-bit words to expect
        mTotalBytesExpected = theByte * 4;
        // Add 4 bytes for the first frame
        mTotalBytesExpected += 4;
        // Add 1 byte for the CRC value
        mTotalBytesExpected += 1;

        mNumBytesLeftExpected = mTotalBytesExpected - 1;
    }
    else if (mNumBytesLeftExpected > 0)
    {
        --mNumBytesLeftExpected;
    }

    if (mResults->mDataFormat == MapleBusAnalyzerResults::DataFormat::BYTE || mNumBytesLeftExpected == 0)
    {
        Frame frame;
        frame.mData1 = theByte;
        MapleBusAnalyzerResults::FrameDataType wordType = MapleBusAnalyzerResults::FRAME_DATA_TYPE_PAYLOAD;
        if (mByteCount < 5)
        {
            wordType = MapleBusAnalyzerResults::FRAME_DATA_TYPE_FRAME;
        }
        else if (mNumBytesLeftExpected == 0)
        {
            wordType = MapleBusAnalyzerResults::FRAME_DATA_TYPE_CRC;
        }
        frame.mData2 = mNumBytesLeftExpected;
        frame.mType = wordType;
        frame.mFlags = 0;
        frame.mStartingSampleInclusive = startingSample;
        frame.mEndingSampleInclusive = mSerialB->GetSampleNumber();

        mResults->AddFrame(frame);
        mResults->CommitResults();
        ReportProgress(frame.mEndingSampleInclusive);
    }

    if (mByteCount == 1)
    {
        mTotalWordsExpected = theByte + 1;
        mNumWordsLeftExpected = mTotalWordsExpected;
    }
    else if (mByteCount % 4 == 0)
    {
        // We have a word to save
        if (mNumWordsLeftExpected > 0)
        {
            --mNumWordsLeftExpected;
        }

        if (mResults->mDataFormat == MapleBusAnalyzerResults::DataFormat::WORD ||
            mResults->mDataFormat == MapleBusAnalyzerResults::DataFormat::WORD_BYTES ||
            mResults->mDataFormat == MapleBusAnalyzerResults::DataFormat::WORD_BYTES_LE)
        {
            Frame frame;
            frame.mData1 = mCurrentWord;
            MapleBusAnalyzerResults::FrameDataType wordType = MapleBusAnalyzerResults::FRAME_DATA_TYPE_PAYLOAD;
            if (mByteCount == 4)
            {
                wordType = MapleBusAnalyzerResults::FRAME_DATA_TYPE_FRAME;
            }
            frame.mData2 = mNumWordsLeftExpected;
            frame.mType = wordType;
            frame.mStartingSampleInclusive = mWordStartingSample;
            frame.mEndingSampleInclusive = mSerialB->GetSampleNumber();

            mResults->AddFrame(frame);
            mResults->CommitResults();
        }
        mCurrentWord = 0;
    }

    if (mByteCount % 4 == 1)
    {
        mWordStartingSample = startingSample;
    }
}

void MapleBusAnalyzer::WorkerThread()
{
    mSerialA = GetAnalyzerChannelData(mSettings->mInputChannelA);
    mSerialB = GetAnalyzerChannelData(mSettings->mInputChannelB);

    bool errorDetected = false;
    while (true)
    {
        if (errorDetected)
        {
            LogError();
            errorDetected = false;
        }

        // Wait for the start of the next packet
        AdvanceToNextStart();
        ResetPacketData();

        bool endDetected = false;
        while (!endDetected && !errorDetected)
        {
            // Get the next byte

            U64 startingSample = mSerialA->GetSampleNumber();
            bool aIsClock = true;
            U8 currentByte = 0;
            U8 mask = 1 << 7;
            for (U32 i = 0; i < 8 && !endDetected && !errorDetected; i++, aIsClock = !aIsClock, mask = mask >> 1)
            {
                // Data and clock flip flop on each bit
                AnalyzerChannelData* clock = aIsClock ? mSerialA : mSerialB;
                Channel clockChannel = aIsClock ? mSettings->mInputChannelA : mSettings->mInputChannelB;
                AnalyzerChannelData* data = aIsClock ? mSerialB : mSerialA;
                Channel dataChannel = aIsClock ? mSettings->mInputChannelB : mSettings->mInputChannelA;

                if (clock->GetBitState() == BIT_LOW)
                {
                    // Need to wait for clock to transition to high first
                    clock->AdvanceToNextEdge();
                }

                // Go to clock falling edge
                clock->AdvanceToNextEdge();
                U64 clockEdgeSample = clock->GetSampleNumber();
                U32 numDataEdges = data->AdvanceToAbsPosition(clockEdgeSample);

                if (i == 0)
                {
                    U32 checkStatus = CheckForEnd(clock, data, numDataEdges);
                    if (checkStatus < 0)
                    {
                        errorDetected = true;
                    }
                    else if (checkStatus > 0)
                    {
                        endDetected = true;
                    }
                }
                else if (numDataEdges > 1)
                {
                    // More than 1 data edge before clock is not expected
                    errorDetected = true;
                }

                if (!endDetected && !errorDetected)
                {
                    // Valid bit detected
                    if (data->GetBitState() == BIT_HIGH)
                    {
                        currentByte |= mask;
                    }
                    // let's put markers exactly where we sample this bit:
                    // mResults->AddMarker( clockEdgeSample, AnalyzerResults::Dot, dataChannel );
                    mResults->AddMarker(clockEdgeSample, AnalyzerResults::DownArrow, clockChannel);
                }
            }

            if (!endDetected && !errorDetected)
            {
                // we have a byte to save!
                SaveByte(startingSample, currentByte);
            }
        }
    }
}

bool MapleBusAnalyzer::NeedsRerun()
{
    return false;
}

U32 MapleBusAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate,
                                             SimulationChannelDescriptor** simulation_channels)
{
    if (mSimulationInitilized == false)
    {
        mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
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

void DestroyAnalyzer(Analyzer* analyzer)
{
    delete analyzer;
}