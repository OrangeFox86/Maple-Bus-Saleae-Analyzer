#ifndef MAPLEBUS_ANALYZER_SETTINGS
#define MAPLEBUS_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class MapleBusAnalyzerSettings : public AnalyzerSettings
{
public:
	MapleBusAnalyzerSettings();
	virtual ~MapleBusAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannelA;
    Channel mInputChannelB;

protected:
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mInputChannelAInterface;
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mInputChannelBInterface;
};

#endif //MAPLEBUS_ANALYZER_SETTINGS
