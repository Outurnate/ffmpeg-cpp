#pragma once

#include "Codecs/Codec.h"

enum AVCodecID;
enum AVSampleFormat;

namespace ffmpegcpp
{
	class OpenCodec;

	class AudioCodec : public Codec
	{
	public:

		AudioCodec(const char* codecName);
		AudioCodec(AVCodecID codecId);
		virtual ~AudioCodec();

        std::unique_ptr<OpenCodec> Open(int bitRate, AVSampleFormat format, int sampleRate);

		bool IsFormatSupported(AVSampleFormat format);
		bool IsChannelsSupported(int channels);
		bool IsSampleRateSupported(int sampleRate);

		AVSampleFormat GetDefaultSampleFormat();
		int GetDefaultSampleRate();

	};


}