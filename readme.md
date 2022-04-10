# Saleae Analyzer for Sega's Mable Bus

Sega's Maple Bus is used on Dreamcast's controller interface.
![sample](sample.jpg?raw=true)

## Build

**Prerequisites**

- For Windows: Visual Studio (Community Edition or better) https://visualstudio.microsoft.com/vs/
- For other OS: Any CMake Builder
- `git` command line executable 
  - For Windows, use: https://gitforwindows.org/

**Getting the Saleae Analizer SDK**

First, execute the following to pull down the AnalyzerSDK. This is the only dependency of this library.
```
git submodule update --init --recursive
```

**For Windows:**

- Open Visual Studio
- `File->Open->Folder...`
- Select this repo
  - Visual Studio will try to execute `git`, so it is necessary to have `git` on command line installed
- `Build->Build All`

**For any other OS:**

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

In order for the library to be loaded, you must then close and then reopen the Logic application.

### Configuring the Maple Bus Analyzer

Select the two data lines and what kind of output style to view/export.
![maple_bus_setting](maple_bus_setting.jpg?raw=true)

This is what the output will look like if SDCKA is channel 0, SDCKB is channel 1, and output style is `Each Byte`:
![sample](sample.jpg?raw=true)
Byte index is displayed within parentheses next to each byte value.

This is output style `Each Word (little endian)`:
![sample_each_word](sample_each_word.jpg?raw=true)
(F) stands for Frame Word, (C) stands for CRC byte, and (#) is the data word index where # is an integer value.

This is output style `Word Bytes`:
![sample_word_bytes](sample_word_bytes.jpg?raw=true)
(F) stands for Frame Word, (C) stands for CRC byte, and (#) is the data word index where # is an integer value.

This is output style `Word Bytes (little endian)`:
![sample_word_bytes_le](sample_word_bytes_le.jpg?raw=true)
(F) stands for Frame Word, (C) stands for CRC byte, and (#) is the data word index where # is an integer value.

### Running the Analyzer

I recommend enabling the glitch filter at 50 ns on the channels set for SDCKA and SDCKB when making measurements on the Dreamcast.
![glitch_filter_settings](glitch_filter_settings.jpg?raw=true)

### Data Generator

The data generator is not supported, and I don't have any current plans to.

## External Resources

**Saleae SDK**

https://github.com/saleae/SampleAnalyzer/blob/master/docs/Analyzer_API.md

https://support.saleae.com/saleae-api-and-sdk/protocol-analyzer-sdk

**Maple Bus Resources**

https://tech-en.netlify.app/articles/en540236/index.html

http://mc.pp.se/dc/maplebus.html#cmds and http://mc.pp.se/dc/controller.html

https://www.raphnet.net/programmation/dreamcast_usb/index_en.php
