#ifndef __AUDIO_TRANSCODE_H_INCLUDED__
#define __AUDIO_TRANSCODE_H_INCLUDED__
#ifdef _WIN32
//Windows
extern "C"
{
#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavcodec/avcodec.h"

#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"
#include <libavdevice/avdevice.h>
};
#endif

#include <thread>
#include <chrono>
#include "MyMTModules.h"
using namespace DDRMTLib;
using namespace std::chrono;

struct AudioStreamInfo
{
	AVFormatContext *inputFormatContext;
	AVFormatContext *outputFormatContext;
	AVCodecContext *inputCodecContext;
	AVCodecContext *outputCodecContext;
	SwrContext *resampleContext;
	AVAudioFifo *fifo;
};

class AudioTranscode : public MainSubsModel
{
public:
	AudioTranscode();
	~AudioTranscode();
	bool Init(const char *inFilename, const char *outFilename);
	void Start();
	bool Respond();
	void QuitSubThread();
	//void SetInput(char* input);
	//void SetOutput(char* output);

protected:
	bool _onRunOnce_Sub(int) override;
	void Quit();

private:
	AudioStreamInfo mStreamInfo;
	char mInFilename[256], mOutFilename[256], mLocalNodeName[256];

	bool bQuitSubThread;

	time_point<system_clock, system_clock::duration> mTimeBeforeBlocking;

	/* Global timestamp for the audio frames. */
	int64_t mPts;

	std::thread mLocalAudioThread;

	/**
	* Open an input file and the required decoder.
	* @param[out] inputFormatContext Format context of opened file
	* @param[out] inputCodecContext  Codec context of opened file
	* @return Error code (0 if successful)
	*/
	int OpenInputFile(AVFormatContext **inputFormatContext,
		              AVCodecContext **inputCodecContext);
	/**
	* Open an output file and the required encoder.
	* Also set some basic encoder parameters.
	* Some of these parameters are based on the input file's parameters.
	* @param      inputCodecContext   Codec context of input file
	* @param[out] outputFormatContext Format context of output file
	* @param[out] outputCodecContext  Codec context of output file
	* @return Error code (0 if successful)
	*/
	int OpenOutputFile(AVCodecContext *inputCodecContext,
		               AVFormatContext **outputFormatContext,
		               AVCodecContext **outputCodecContext);

	/**
	* Initialize one data packet for reading or writing.
	* @param packet Packet to be initialized
	*/
	void InitPacket(AVPacket *packet);

	/**
	* Initialize one audio frame for reading from the input file.
	* @param[out] frame Frame to be initialized
	* @return Error code (0 if successful)
	*/
	int InitInputFrame(AVFrame **frame);

	/**
	* Initialize the audio resampler based on the input and output codec settings.
	* If the input and output sample formats differ, a conversion is required
	* libswresample takes care of this, but requires initialization.
	* @param      input_codec_context  Codec context of the input file
	* @param      output_codec_context Codec context of the output file
	* @param[out] resample_context     Resample context for the required conversion
	* @return Error code (0 if successful)
	*/
	int InitResampler(AVCodecContext *inputCodecContext,
		              AVCodecContext *outputCodecContext,
		              SwrContext **resampleContext);

	/**
	* Initialize a FIFO buffer for the audio samples to be encoded.
	* @param[out] fifo                 Sample buffer
	* @param      output_codec_context Codec context of the output file
	* @return Error code (0 if successful)
	*/
	int InitFIFO(AVAudioFifo **fifo, AVCodecContext *outputCodecContext);

	/**
	* Write the header of the output file container.
	* @param output_format_context Format context of the output file
	* @return Error code (0 if successful)
	*/
	int WriteOutputFileHeader(AVFormatContext *outputFormatContext);

	/**
	* Decode one audio frame from the input file.
	* @param      frame                Audio frame to be decoded
	* @param      input_format_context Format context of the input file
	* @param      input_codec_context  Codec context of the input file
	* @param[out] data_present         Indicates whether data has been decoded
	* @param[out] finished             Indicates whether the end of file has
	*                                  been reached and all data has been
	*                                  decoded. If this flag is false, there
	*                                  is more data to be decoded, i.e., this
	*                                  function has to be called again.
	* @return Error code (0 if successful)
	*/
	int DecodeAudioFrame(AVFrame *frame,
		                 AVFormatContext *inputFormatContext,
		                 AVCodecContext *inputCodecContext,
		                 int *dataPresent, int *finished);

	/**
	* Initialize a temporary storage for the specified number of audio samples.
	* The conversion requires temporary storage due to the different format.
	* The number of audio samples to be allocated is specified in frame_size.
	* @param[out] converted_input_samples Array of converted samples. The
	*                                     dimensions are reference, channel
	*                                     (for multi-channel audio), sample.
	* @param      output_codec_context    Codec context of the output file
	* @param      frame_size              Number of samples to be converted in
	*                                     each round
	* @return Error code (0 if successful)
	*/
	int InitConvertedSamples(uint8_t ***convertedInputSamples,
		                     AVCodecContext *outputCodecContext,
							 int frameSize);

	/**
	* Convert the input audio samples into the output sample format.
	* The conversion happens on a per-frame basis, the size of which is
	* specified by frame_size.
	* @param      input_data       Samples to be decoded. The dimensions are
	*                              channel (for multi-channel audio), sample.
	* @param[out] converted_data   Converted samples. The dimensions are channel
	*                              (for multi-channel audio), sample.
	* @param      frame_size       Number of samples to be converted
	* @param      resample_context Resample context for the conversion
	* @return Error code (0 if successful)
	*/
	int ConvertSamples(const uint8_t **inputData,
		               uint8_t **convertedData, 
					   const int frameSize,
		               SwrContext *resampleContext);

	/**
	* Add converted input audio samples to the FIFO buffer for later processing.
	* @param fifo                    Buffer to add the samples to
	* @param converted_input_samples Samples to be added. The dimensions are channel
	*                                (for multi-channel audio), sample.
	* @param frame_size              Number of samples to be converted
	* @return Error code (0 if successful)
	*/
	int AddSamplesToFIFO(AVAudioFifo *fifo,
		                 uint8_t **convertedInputSamples,
		                 const int frameSize);

	/**
	* Read one audio frame from the input file, decode, convert and store
	* it in the FIFO buffer.
	* @param      fifo                 Buffer used for temporary storage
	* @param      input_format_context Format context of the input file
	* @param      input_codec_context  Codec context of the input file
	* @param      output_codec_context Codec context of the output file
	* @param      resampler_context    Resample context for the conversion
	* @param[out] finished             Indicates whether the end of file has
	*                                  been reached and all data has been
	*                                  decoded. If this flag is false,
	*                                  there is more data to be decoded,
	*                                  i.e., this function has to be called
	*                                  again.
	* @return Error code (0 if successful)
	*/
	int ReadDecodeConvertAndStore(AVAudioFifo *fifo,
		                          AVFormatContext *inputFormatContext,
		                          AVCodecContext *inputCodecContext,
		                          AVCodecContext *outputCodecContext,
		                          SwrContext *resamplerContext,
								  int *finished);

	/**
	* Initialize one input frame for writing to the output file.
	* The frame will be exactly frame_size samples large.
	* @param[out] frame                Frame to be initialized
	* @param      output_codec_context Codec context of the output file
	* @param      frame_size           Size of the frame
	* @return Error code (0 if successful)
	*/
	int InitOutputFrame(AVFrame **frame,
		                AVCodecContext *outputCodecContext,
		                int frameSize);

	/**
	* Encode one frame worth of audio to the output file.
	* @param      frame                 Samples to be encoded
	* @param      output_format_context Format context of the output file
	* @param      output_codec_context  Codec context of the output file
	* @param[out] data_present          Indicates whether data has been
	*                                   encoded
	* @return Error code (0 if successful)
	*/
	int EncodeAudioFrame(AVFrame *frame,
		                 AVFormatContext *outputFormatContext,
		                 AVCodecContext *outputCodecContext,
		                 int *dataPresent);

	/**
	* Load one audio frame from the FIFO buffer, encode and write it to the
	* output file.
	* @param fifo                  Buffer used for temporary storage
	* @param output_format_context Format context of the output file
	* @param output_codec_context  Codec context of the output file
	* @return Error code (0 if successful)
	*/
	int LoadEncodeAndWrite(AVAudioFifo *fifo,
		                   AVFormatContext *output_format_context,
		                   AVCodecContext *output_codec_context);

	/**
	* Write the trailer of the output file container.
	* @param output_format_context Format context of the output file
	* @return Error code (0 if successful)
	*/
	int WriteOutputFileTrailer(AVFormatContext *outputFormatContext);
};
#endif //__AUDIO_TRANSCODE_H_INCLUDED__