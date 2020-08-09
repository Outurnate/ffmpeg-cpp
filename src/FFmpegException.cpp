#include "FFmpegException.h"

using namespace std;

namespace ffmpegcpp
{
	FFmpegException::FFmpegException(const string & error) : runtime_error(error)
	{
	}

	FFmpegException::FFmpegException(const string & error, int returnValue)
		: runtime_error(
			(error + ": " + av_make_error_string(this->error, AV_ERROR_MAX_STRING_SIZE, returnValue))
		)
	{
	}
}
