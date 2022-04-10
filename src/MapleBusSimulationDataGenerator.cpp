#include "MapleBusSimulationDataGenerator.h"
#include "MapleBusAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

MapleBusSimulationDataGenerator::MapleBusSimulationDataGenerator()
{
}

MapleBusSimulationDataGenerator::~MapleBusSimulationDataGenerator()
{
}

void MapleBusSimulationDataGenerator::Initialize(U32 simulation_sample_rate, MapleBusAnalyzerSettings* settings)
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;

    mSerialASimulationData.SetChannel(mSettings->mInputChannelA);
    mSerialASimulationData.SetSampleRate(simulation_sample_rate);
    mSerialASimulationData.SetInitialBitState(BIT_HIGH);

    mSerialBSimulationData.SetChannel(mSettings->mInputChannelB);
    mSerialBSimulationData.SetSampleRate(simulation_sample_rate);
    mSerialBSimulationData.SetInitialBitState(BIT_HIGH);
}

U32 MapleBusSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate,
                                                            SimulationChannelDescriptor** simulation_channel)
{
    // Not supporting this at the moment
    return 0;
}
