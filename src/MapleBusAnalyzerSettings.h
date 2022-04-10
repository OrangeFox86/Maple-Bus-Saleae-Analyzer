#ifndef MAPLEBUS_ANALYZER_SETTINGS
#define MAPLEBUS_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class MapleBusAnalyzerSettings : public AnalyzerSettings
{
  public:
    //! Output style menu options
    enum OutputStyleNumber
    {
        OUTPUT_STYLE_EACH_BYTE = 0,
        OUTPUT_STYLE_EACH_WORD,
        OUTPUT_STYLE_WORD_BYTES,
        OUTPUT_STYLE_WORD_BYTES_LE,

        //! Used for conversion only
        OUTPUT_STYLE_COUNT
    };

    //! Constructor
    MapleBusAnalyzerSettings();
    //! Destructor
    virtual ~MapleBusAnalyzerSettings();

    //! API: load settings from my interfaces
    //! @returns true always
    virtual bool SetSettingsFromInterfaces();
    //! API: set interface from my settings
    void UpdateInterfacesFromSettings();
    //! API: load settings from string
    //! @param[in] settings  input settings string
    virtual void LoadSettings(const char* settings);
    //! API: save settings to string
    //! @returns settings string
    virtual const char* SaveSettings();

    //! Converts a number to output style enum
    static OutputStyleNumber NumberToOutputStyle(double num);

    //! The selected input channel
    Channel mInputChannelA;
    //! The selected output channel
    Channel mInputChannelB;
    //! The selected data output style
    OutputStyleNumber mOutputStyle;

  protected:
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mInputChannelAInterface;
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mInputChannelBInterface;
    std::auto_ptr<AnalyzerSettingInterfaceNumberList> mOutputStyleInterface;
};

#endif // MAPLEBUS_ANALYZER_SETTINGS
