#include "FrameContainer.h"
#include "FFmpegException.h"

using namespace std;

namespace ffmpegcpp
{
	FrameContainer::FrameContainer(AVFrame* p_frame, AVRational* p_timeBase)
	{
		AVFrame *tmp = av_frame_clone(p_frame);

		if (!tmp) throw new FFmpegException("Failed to clone frame");
		    av_frame_unref(frame);

		this->frame = tmp;
		this->timeBase = p_timeBase;
	}

	FrameContainer::~FrameContainer()
	{
		av_frame_free(&frame);
	}

	AVFrame* FrameContainer::GetFrame()
	{
		return frame;
	}

	AVRational* FrameContainer::GetTimeBase()
	{
		return timeBase;
	}
}
