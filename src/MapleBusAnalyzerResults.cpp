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

void MapleBusAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	switch( mType )
    {
		default:
		case Type::BYTE:
		{
			char number_str[ 16 ];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, sizeof( number_str ) );
			char output_str[ 128 ];
			snprintf( output_str, sizeof( output_str ), "%s (%u)", number_str, static_cast<U32>( frame.mData2 ) );
			AddResultString( output_str );
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
            char number_str[ 32 ];
            AnalyzerHelpers::GetNumberString( frame.mData1, display_base, numDataBits, number_str, sizeof( number_str ) );

			char type_str[ 8 ] = {};
            switch( dataType )
            {
				case 1:
					type_str[ 0 ] = 'F';
                    break;

                case 2:
                    type_str[ 0 ] = 'C';
                    break;

				default:
					snprintf( type_str, sizeof( type_str ), "%u", static_cast<U32>( frame.mData2 & 0xFFFFFFFF ) );
                    break;
            }
            
            char output_str[ 128 ];
            snprintf( output_str, sizeof( output_str ), "%s (%s)", number_str, type_str );
            AddResultString( output_str );
        }
        break;
    }
}

void MapleBusAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

        char number_str[ 128 ];
		switch( mType )
        {
			default:
			case Type::BYTE:
			{
                AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
			}
            break;

            case Type::WORD:
            {
                U32 numDataBits = 8 * 4;
                U32 dataType = frame.mData2 >> 32;
                if( dataType == 2 )
                {
                    // CRC byte
                    numDataBits = 8;
                }
                char number_str[ 32 ];
                AnalyzerHelpers::GetNumberString( frame.mData1, display_base, numDataBits, number_str, sizeof( number_str ) );
            }
            break;
        }
		

		file_stream << time_str << "," << number_str << std::endl;

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