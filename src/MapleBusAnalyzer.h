#ifndef MAPLEBUS_ANALYZER_H
#define MAPLEBUS_ANALYZER_H

#include <Analyzer.h>
#include "MapleBusAnalyzerResults.h"
#include "MapleBusSimulationDataGenerator.h"

class MapleBusAnalyzerSettings;
class ANALYZER_EXPORT MapleBusAnalyzer : public Analyzer2
{
public:
	MapleBusAnalyzer();
	virtual ~MapleBusAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

  private:
    void LogError();
    void AlignSerialMarkers();
    void AdvanceToNeutral();
    void ResetPacketData();
    void AdvanceToNextStart();
	//! @returns -1 if error was detected
	//! @returns 0 if end was not detected
	//! @returns 1 if end was detected
    S32 CheckForEnd(AnalyzerChannelData* clock, AnalyzerChannelData* data, U32 numDataEdges);
    void SaveByte(U64 startingSample, U8 theByte);

  protected: //vars
	std::auto_ptr< MapleBusAnalyzerSettings > mSettings;
    std::auto_ptr<MapleBusAnalyzerResults> mResults;
    AnalyzerChannelData* mSerialA;
    AnalyzerChannelData* mSerialB;

	MapleBusSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	// Packet state variables
	S32 mTotalBytesExpected;
    S32 mTotalWordsExpected;
    S32 mNumBytesLeftExpected;
    S32 mNumWordsLeftExpected;
    U32 mByteCount;
    U32 mCurrentWord;
    U64 mWordStartingSample;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //MAPLEBUS_ANALYZER_H
