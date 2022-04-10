#include "MapleBusAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "MapleBusAnalyzer.h"
#include "MapleBusAnalyzerSettings.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

MapleBusAnalyzerResults::MapleBusAnalyzerResults(MapleBusAnalyzer* analyzer, MapleBusAnalyzerSettings* settings, DataFormat type)
    : AnalyzerResults(), mDataFormat(type), mSettings(settings), mAnalyzer(analyzer)
{
}

MapleBusAnalyzerResults::~MapleBusAnalyzerResults()
{
}

void MapleBusAnalyzerResults::GenerateNumberStr(char* str, U32 len, const Frame& frame, DisplayBase display_base, bool forExport) const
{
    switch (mDataFormat)
    {
    default:
    case DataFormat::BYTE:
    {
        AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, str, len);
    }
    break;

    case DataFormat::WORD:
    {
        U32 numDataBits = 8 * 4;
        if (frame.mType == FRAME_DATA_TYPE_CRC)
        {
            // CRC byte
            numDataBits = 8;
        }
        AnalyzerHelpers::GetNumberString(frame.mData1, display_base, numDataBits, str, len);
    }
    break;

    case DataFormat::WORD_BYTES:
    case DataFormat::WORD_BYTES_LE:
    {
        if (frame.mType == FRAME_DATA_TYPE_CRC)
        {
            // CRC byte
            if (forExport)
            {
                char number_str[32];
                AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, sizeof(number_str));
                snprintf(str, len, "%s,,,", number_str);
            }
            else
            {
                AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, str, len);
            }
        }
        else
        {
            char number_strs[4][32];
            U32 data = static_cast<U32>(frame.mData1);
            for (U32 i = 0; i < 4; ++i, data = data >> 8)
            {
                AnalyzerHelpers::GetNumberString(data & 0xFF, display_base, 8, number_strs[i], sizeof(number_strs[i]));
            }
            const char* format = NULL;
            if (forExport)
            {
                format = "%s,%s,%s,%s";
            }
            else
            {
                format = "%s %s %s %s";
            }
            if (mDataFormat == DataFormat::WORD_BYTES)
            {
                snprintf(str, len, format, number_strs[0], number_strs[1], number_strs[2], number_strs[3]);
            }
            else
            {
                snprintf(str, len, format, number_strs[3], number_strs[2], number_strs[1], number_strs[0]);
            }
        }
    }
    break;
    }
}

void MapleBusAnalyzerResults::GenerateExtraInfoStr(char* str, U32 len, const Frame& frame) const
{
    U32 numItemsLeft = static_cast<U32>(frame.mData2);

    switch (mDataFormat)
    {
    default:
    case DataFormat::BYTE:
    {
        snprintf(str, len, "%u", numItemsLeft);
    }
    break;

    case DataFormat::WORD:
    case DataFormat::WORD_BYTES:
    case DataFormat::WORD_BYTES_LE:
    {
        char type_str[8] = {};
        switch (frame.mType)
        {
        case FRAME_DATA_TYPE_FRAME:
        {
            type_str[0] = 'F';
            break;
        }

        case FRAME_DATA_TYPE_CRC:
        {
            type_str[0] = 'C';
            break;
        }

        default:
        {
            snprintf(type_str, sizeof(type_str), "%u", numItemsLeft);
        }
        break;
        }

        snprintf(str, len, "%s", type_str);
    }
    break;
    }
}

void MapleBusAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
    ClearResultStrings();
    Frame frame = GetFrame(frame_index);

    char number_str[64];
    GenerateNumberStr(number_str, sizeof(number_str), frame, display_base, false);
    char extra_info_str[32];
    GenerateExtraInfoStr(extra_info_str, sizeof(extra_info_str), frame);
    char output_str[128];
    snprintf(output_str, sizeof(output_str), "%s (%s)", number_str, extra_info_str);
    AddResultString(output_str);
}

void MapleBusAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
    std::ofstream file_stream(file, std::ios::out);

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    file_stream << "Time [s],";

    switch (mDataFormat)
    {
    default:
    case DataFormat::BYTE:
        file_stream << "Num Words, Sender Addr, Recipient Addr, Command, Data & CRC ->";
        break;

    case DataFormat::WORD:
        file_stream << "Frame Word (little endian), Data & CRC ->";
        break;

    case DataFormat::WORD_BYTES:
        file_stream << "Num Words, Sender Addr, Recipient Addr, Command, Data & CRC ->";
        break;

    case DataFormat::WORD_BYTES_LE:
        file_stream << "Command, Recipient Addr, Sender Addr, Num Words, Data (little endian) & CRC ->";
        break;
    }

    U64 num_frames = GetNumFrames();
    U32 previousNumItemsLeft = 0;
    U8 previousWordType = FRAME_DATA_TYPE_NONE;
    for (U32 i = 0; i < num_frames; i++)
    {
        Frame frame = GetFrame(i);

        char time_str[128];
        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

        char number_str[64];
        GenerateNumberStr(number_str, sizeof(number_str), frame, display_base, true);
        U32 numItemsLeft = static_cast<U32>(frame.mData2);

        if (i == 0 || (numItemsLeft > 0 && previousNumItemsLeft == 0) ||
            (previousNumItemsLeft > 0 && previousNumItemsLeft - 1 != numItemsLeft) ||
            (previousWordType == FRAME_DATA_TYPE_CRC && frame.mType != FRAME_DATA_TYPE_CRC))
        {
            file_stream << std::endl << time_str << ",";
        }
        previousNumItemsLeft = numItemsLeft;
        previousWordType = frame.mType;

        file_stream << number_str << ",";

        if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true)
        {
            file_stream.close();
            return;
        }
    }

    file_stream.close();
}

void MapleBusAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
    Frame frame = GetFrame(frame_index);
    ClearTabularText();

    char number_str[128];
    AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
    AddTabularText(number_str);
#endif
}

void MapleBusAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
    // not supported
}

void MapleBusAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
    // not supported
}