# Saleae Analyzer for Sega's Mable Bus

Sega's Maple Bus is used on Dreamcast's controller interface.
![sample](sample.jpg?raw=true)

## Build

*Prerequisites*
- For Windows: Visual Studio (Community Edition or better) https://visualstudio.microsoft.com/vs/
- For other OS: Any CMake Builder
- `git` command line executable (for Windows: https://gitforwindows.org/)

First, execute the following to pull down the AnalyzerSDK.
```
git submodule update --init --recursive
```

For Windows:
- Open Visual Studio
- `File->Open->Folder...`
  - Visual Studio will try to execute `git`, so it is necessary to have `git` on command line installed
- Select this repo
- `Build->Build All`

For any other OS:
```
cd Maple-Bus-Saleae-Analyzer
cmake .
make
```

## Using the Analyzer

### Adding the Analyzer

Execute the Saleae software and open up the Preferences dialog.
![settings_preferences](settings_preferences.jpg?raw=true)

Go to the `Custom Low Level Analyzers` setting and browse to the directory which contains the compiled `.dll` for Windows or `.so` for Linux.
![custom_analyzers_setting](custom_analyzers_setting.jpg?raw=true)

### Configuring the Maple Bus Analyzer

Select the two data lines and what kind of output style to view/export.
![maple_bus_setting](maple_bus_setting.jpg?raw=true)

This is what the output will look like if SDCKA is channel 0, SDCKB is channel 1, and output style is `Each Byte`:
![sample](sample.jpg?raw=true)

This is output style `Each Word (little endian)`:
![sample_each_word](sample_each_word.jpg?raw=true)

This is output style `Word Bytes`:
![sample_word_bytes](sample_word_bytes.jpg?raw=true)

### Data Generator

The data generator is not supported, and I don't have any current plans to.

## External Resources

https://github.com/saleae/SampleAnalyzer/blob/master/docs/Analyzer_API.md
https://support.saleae.com/saleae-api-and-sdk/protocol-analyzer-sdk
