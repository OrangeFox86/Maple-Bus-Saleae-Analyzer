#ifndef MAPLEBUS_SIMULATION_DATA_GENERATOR
#define MAPLEBUS_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class MapleBusAnalyzerSettings;

class MapleBusSimulationDataGenerator
{
public:
	MapleBusSimulationDataGenerator();
	~MapleBusSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, MapleBusAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	MapleBusAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	SimulationChannelDescriptor mSerialASimulationData;
    SimulationChannelDescriptor mSerialBSimulationData;

};
#endif //MAPLEBUS_SIMULATION_DATA_GENERATOR