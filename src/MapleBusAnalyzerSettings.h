#ifndef MAPLEBUS_ANALYZER_SETTINGS
#define MAPLEBUS_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class MapleBusAnalyzerSettings : public AnalyzerSettings
{
public:
	enum OutputStyleNumber
	{
		OUTPUT_STYLE_EACH_BYTE = 0,
		OUTPUT_STYLE_EACH_WORD,
		OUTPUT_STYLE_WORD_BYTES
	};

	MapleBusAnalyzerSettings();
	virtual ~MapleBusAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	Channel mInputChannelA;
    Channel mInputChannelB;
    double mOutputStyle;

protected:
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mInputChannelAInterface;
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mInputChannelBInterface;
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mOutputStyleInterface;
};

#endif //MAPLEBUS_ANALYZER_SETTINGS
