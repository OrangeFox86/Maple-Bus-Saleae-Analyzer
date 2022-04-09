#include "MapleBusAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "MapleBusAnalyzer.h"
#include "MapleBusAnalyzerSettings.h"
#include <iostream>
#include <fstream>

MapleBusAnalyzerResults::MapleBusAnalyzerResults( MapleBusAnalyzer* analyzer, MapleBusAnalyzerSettings* settings, Type type )
    :	AnalyzerResults(), 
	mType( type ),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

MapleBusAnalyzerResults::~MapleBusAnalyzerResults()
{
}

void MapleBusAnalyzerResults::GenerateNumberStr( char* str, U32 len, const Frame& frame, DisplayBase display_base, bool forExport ) const
{
    switch( mType )
    {
        default:
        case Type::BYTE:
        {
            AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, str, len );
        }
        break;

        case Type::WORD:
        {
            // mData: Word or byte value
            // Upper 32 bits of mData2:
            // 0: Data word
            // 1: Frame word
            // 2: CRC byte
            // Lower 32 bits of mData2: Number of words left

            U32 numDataBits = 8 * 4;
            U32 dataType = frame.mData2 >> 32;
            if( dataType == 2 )
            {
                // CRC byte
                numDataBits = 8;
            }
            AnalyzerHelpers::GetNumberString( frame.mData1, display_base, numDataBits, str, len );
        }
        break;

        case Type::WORD_BYTES:
        {
            // mData: Word or byte value
            // Upper 32 bits of mData2:
            // 0: Data word
            // 1: Frame word
            // 2: CRC byte
            // Lower 32 bits of mData2: Number of words left

            U32 dataType = frame.mData2 >> 32;
            if( dataType == 2 )
            {
                // CRC byte
                if( forExport )
                {
                    char number_str[ 32 ];
                    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, sizeof( number_str ) );
                    snprintf( str, len, "%s,,,", number_str );
                }
                else
                {
                    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, str, len );
                }
            }
            else
            {
                char number_strs[ 4 ][ 32 ];
                U32 data = static_cast<U32>( frame.mData1 );
                for( U32 i = 0; i < 4; ++i, data = data >> 8 )
                {
                    AnalyzerHelpers::GetNumberString( data & 0xFF, display_base, 8, number_strs[ i ], sizeof( number_strs[ i ] ) );
                }
                const char* format = NULL;
                if( forExport )
                {
                    format = "%s,%s,%s,%s";
                }
                else
                {
                    format = "%s %s %s %s";
                }
                snprintf( str, len, format, number_strs[ 0 ], number_strs[ 1 ], number_strs[ 2 ],
                          number_strs[ 3 ] );
            }
        }
        break;
    }
}

void MapleBusAnalyzerResults::GenerateExtraInfoStr( char* str, U32 len, const Frame& frame, bool forExport ) const
{
    switch( mType )
    {
        default:
        case Type::BYTE:
        {
            snprintf( str, len, "%u", static_cast<U32>( frame.mData2 ) );
        }
        break;

        case Type::WORD:
        case Type::WORD_BYTES:
        {
            // mData: Word or byte value
            // Upper 32 bits of mData2:
            // 0: Data word
            // 1: Frame word
            // 2: CRC byte
            // Lower 32 bits of mData2: Number of words left

            U32 dataType = frame.mData2 >> 32;
            char type_str[ 8 ] = {};
            switch( dataType )
            {
                case 1:
                {
                    if( forExport )
                    {
                        strncpy( type_str, "Frame", sizeof( type_str ) );
                    }
                    else
                    {
                        type_str[ 0 ] = 'F';
                    }
                    break;
                }

                case 2:
                {
                    if( forExport )
                    {
                        strncpy( type_str, "CRC", sizeof( type_str ) );
                    }
                    else
                    {
                        type_str[ 0 ] = 'C';
                    }
                    break;
                }

                default:
                {
                    const char* format = NULL;
                    if( forExport )
                    {
                        format = "Data %u";
                    }
                    else
                    {
                        format = "%u";
                    }
                    snprintf( type_str, sizeof( type_str ), format, static_cast<U32>( frame.mData2 & 0xFFFFFFFF ) );
                }
                break;

            }

            snprintf( str, len, "%s", type_str );
        }
        break;
    }
}

void MapleBusAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

    char number_str[ 64 ];
    GenerateNumberStr( number_str, sizeof( number_str ), frame, display_base, false );
    char extra_info_str[ 32 ];
    GenerateExtraInfoStr( extra_info_str, sizeof( extra_info_str ), frame, false );
    char output_str[ 128 ];
    snprintf( output_str, sizeof( output_str ), "%s (%s)", number_str, extra_info_str );
    AddResultString( output_str );
}

void MapleBusAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],";

    switch(mType)
    {
        default:
        case Type::BYTE:
            file_stream << "Byte Value,Bytes Remaining in Packet";
            break;

        case Type::WORD:
            file_stream << "Word Value,Word Type";
            break;

        case Type::WORD_BYTES:
            file_stream << "Byte1,Byte2,Byte3,Byte4,Word Type";
            break;
    }

    file_stream << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

        char number_str[ 64 ];
        GenerateNumberStr( number_str, sizeof( number_str ), frame, display_base, true );
        char extra_info_str[ 32 ];
        GenerateExtraInfoStr( extra_info_str, sizeof( extra_info_str ), frame, true );		

		file_stream << time_str << "," << number_str << "," << extra_info_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void MapleBusAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	Frame frame = GetFrame( frame_index );
	ClearTabularText();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	AddTabularText( number_str );
#endif
}

void MapleBusAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not supported

}

void MapleBusAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}