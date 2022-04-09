#ifndef MAPLEBUS_ANALYZER_RESULTS
#define MAPLEBUS_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class MapleBusAnalyzer;
class MapleBusAnalyzerSettings;

class MapleBusAnalyzerResults : public AnalyzerResults
{
public:
    enum class Type
    {
		BYTE, WORD, WORD_BYTES
    };

	MapleBusAnalyzerResults( MapleBusAnalyzer* analyzer, MapleBusAnalyzerSettings* settings, Type type );
	virtual ~MapleBusAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

	const Type mType;

  protected: //functions
    void GenerateNumberStr( char* str, U32 len, const Frame& frame, DisplayBase display_base, bool forExport ) const;
    void GenerateExtraInfoStr( char* str, U32 len, const Frame& frame, bool forExport ) const;

protected:  //vars
	MapleBusAnalyzerSettings* mSettings;
	MapleBusAnalyzer* mAnalyzer;
    
};

#endif //MAPLEBUS_ANALYZER_RESULTS
