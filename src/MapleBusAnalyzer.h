#ifndef MAPLEBUS_ANALYZER_H
#define MAPLEBUS_ANALYZER_H

#include <Analyzer.h>
#include "MapleBusAnalyzerResults.h"
#include "MapleBusSimulationDataGenerator.h"

class MapleBusAnalyzerSettings;
class ANALYZER_EXPORT MapleBusAnalyzer : public Analyzer2
{
  public:
    //! Constructor
    MapleBusAnalyzer();
    //! Deconstructor
    virtual ~MapleBusAnalyzer();

    //! API: Initialize results object
    virtual void SetupResults();
    //! API: Processes samples and saves frame data
    //! @returns never
    virtual void WorkerThread();

    //! API: Simulation data generator
    //! This feature is currently not implemented
    //! @returns 0
    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
    //! API: returns the minumum sample rate needed for this analyzer to Saleae's SDK
    virtual U32 GetMinimumSampleRateHz();

    //! API: returns this analyzer's name to Saleae's SDK
    virtual const char* GetAnalyzerName() const;
    //! API: returns true iff the analyzer failed and needs to be restarted
    //! @returns false always
    virtual bool NeedsRerun();

  private:
    //! Logs information about the current markers to debug log file 
    void LogError();
    //! Aligns the two serial positions to the same sample number
    void AlignSerialMarkers();
    //! Advances the two serial positions to the point where both signals are BIT_HIGH
    void AdvanceToNeutral();
    //! Resets all packet state data
    void ResetPacketData();
    //! Advances the two serial markers just past the next starting sequence
    void AdvanceToNextStart();
    //! Checks if the end sequence is detected just past the current positions.
    //! @param[in,out] clock  the analyzer channel of the current clock positioned at a falling edge; 
    //!                       this is advanced iff error or end detected
    //! @param[in,out] data  the analyzer channel of the current data; 
    //!                      this is advanced iff error or end detected
    //! @param[in] numDataEdges  number of data edges detected between last sample and current clock edge
    //! @returns -1 if error was detected
    //! @returns 0 if end was not detected
    //! @returns 1 if end was detected
    S32 CheckForEnd(AnalyzerChannelData* clock, AnalyzerChannelData* data, U32 numDataEdges);
    //! Saves the next byte
    //! @param[in] startingSample  the starting sample number where the first bit of this byte was read
    //! @param[in] theByte  value of the byte to save
    void SaveByte(U64 startingSample, U8 theByte);

  protected: // vars
    //! Pointer to my input settings
    std::auto_ptr<MapleBusAnalyzerSettings> mSettings;
    //! Pointer to my output results
    std::auto_ptr<MapleBusAnalyzerResults> mResults;
    //! Pointer to the selected serial A channel
    AnalyzerChannelData* mSerialA;
    //! Pointer to the selected serial B channel
    AnalyzerChannelData* mSerialB;

    //! Simulation data generated called by GenerateSimulationData()
    MapleBusSimulationDataGenerator mSimulationDataGenerator;
    //! false until first call to GenerateSimulationData()
    bool mSimulationInitilized;

    // Packet state variables
    //
    //! Total number of bytes expected in the current packet
    S32 mTotalBytesExpected;
    //! Total number of 32-bit words expected in the current packet
    S32 mTotalWordsExpected;
    //! Expected number of bytes left to sample in the current packet
    S32 mNumBytesLeftExpected;
    //! Expected number of 32-bit words left to sample in the current packet
    S32 mNumWordsLeftExpected;
    //! Number of bytes sampled in the current packet
    U32 mByteCount;
    //! The current 32-bit word state of this packet
    U32 mCurrentWord;
    //! The sample number of the start of the current word
    U64 mWordStartingSample;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);

#endif // MAPLEBUS_ANALYZER_H
