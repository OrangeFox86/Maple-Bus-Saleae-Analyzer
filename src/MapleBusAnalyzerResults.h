#ifndef MAPLEBUS_ANALYZER_RESULTS
#define MAPLEBUS_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class MapleBusAnalyzer;
class MapleBusAnalyzerSettings;

class MapleBusAnalyzerResults : public AnalyzerResults
{
  public:
    //! Determines how to handle Frame::mData1
    enum class DataFormat
    {
        //! Expect each frame is a byte of data.
        //! Data is processed verbatim in the order they are received.
        BYTE,
        //! Expect each frame is a 32-bit word in little endian order except 8-bit CRC codes.
        //! Data is processed verbatim in the order they are received.
        WORD,
        //! Expect each frame is a 32-bit word in little endian order except 8-bit CRC codes.
        //! Each word is processed in order and such that the LSB is printed first.
        WORD_BYTES,
        //! Expect each frame is a 32-bit word in little endian order except 8-bit CRC codes.
        //! Each word is processed in order and such that the MSB is printed first.
        WORD_BYTES_LE
    };

    //! Data type for the data in a result frame (Frame::mType values)
    enum FrameDataType
    {
        //! No data type (default type)
        FRAME_DATA_TYPE_NONE = 0,
        //! Data within the frame is payload data
        FRAME_DATA_TYPE_PAYLOAD = 0,
        //! Data within the frame is Maple Bus frame data
        FRAME_DATA_TYPE_FRAME,
        //! Data within the frame is a CRC byte
        FRAME_DATA_TYPE_CRC
    };

    //! Constructor
    MapleBusAnalyzerResults(MapleBusAnalyzer* analyzer, MapleBusAnalyzerSettings* settings, DataFormat type);
    //! Destructor
    virtual ~MapleBusAnalyzerResults();

    //! API: Generate the text seen above samples for a given frame and channel
    virtual void GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base);
    //! API: Export all frame data to the given file path
    virtual void GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id);

    virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
    virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
    virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

    //! Determines how this object will handle Frame::mData1
    const DataFormat mDataFormat;

  protected: // functions
    //! Generate bubble text into given string
    void GenerateBubbleText(char* str, U32 len, U64 frame_index, DisplayBase display_base);
    //! Generates the number string for bubble text and export file
    //! @param[out] str  output string buffer
    //! @param[in] len  byte length of str
    //! @param[in] frame  frame from which contains the data to generate data
    //! @param[in] display_base  contains string formatting information
    //! @param[in] forExport  true iff the string is being generated for export;
    //!                       this will add comma separation to byte values when true
    void GenerateNumberStr(char* str, U32 len, const Frame& frame, DisplayBase display_base, bool forExport) const;
    //! Generate extra information about a frame for bubble text
    //! @param[out] str  output string buffer
    //! @param[in] len  byte length of str
    //! @param[in] frame  frame from which contains the data to generate data
    void GenerateExtraInfoStr(char* str, U32 len, const Frame& frame) const;

  protected: // vars
    //! Pointer to my input settings
    MapleBusAnalyzerSettings* mSettings;
    //! Pointer back to the analyzer that made me
    MapleBusAnalyzer* mAnalyzer;
};

#endif // MAPLEBUS_ANALYZER_RESULTS
