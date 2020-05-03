#include "Demuxer.h"
#include "FFmpegException.h"
#include "CodecDeducer.h"
#include <string>
#include <iostream>

using namespace std;

namespace ffmpegcpp
{

    Demuxer::Demuxer(const char* fileName)
        : Demuxer(fileName, NULL, NULL)
    {
    }


    Demuxer::Demuxer(const char* p_fileName, AVInputFormat* p_inputFormat, AVDictionary *p_format_opts)
    {
        this->m_fileName    = p_fileName;

        int ret = 0;// open input file, and allocate format context

        if ((ret = avformat_open_input(&pAVFormatContextIn, m_fileName, p_inputFormat, &p_format_opts)) < 0)
        {
            CleanUp();
            throw FFmpegException(std::string("Failed to open input container " + string(m_fileName)).c_str(), ret);
        }

//        this->options       = p_format_opts;
//        this->m_inputFormat = p_inputFormat;

        // retrieve stream information
        if ( (ret = (avformat_find_stream_info(pAVFormatContextIn, NULL))) < 0)
        {
            CleanUp();
            throw FFmpegException(std::string("Failed to read streams from " + string(m_fileName)).c_str(), ret);
        }

        inputStreams = new InputStream*[pAVFormatContextIn->nb_streams];
        for (unsigned int i = 0; i < pAVFormatContextIn->nb_streams; ++i)
        {
/*
            AVCodecParameters* par = pAVFormatContextIn->streams[i]->codecpar;

            if  ((par->codec_type) == AVMEDIA_TYPE_VIDEO)
            {
                pAVCodec =  avcodec_find_encoder(pAVFormatContextIn->streams[i]->codecpar->codec_id);
                m_width  = pAVFormatContextIn->streams[i]->codecpar->width;
                m_height = pAVFormatContextIn->streams[i]->codecpar->height;
                options  = pAVFormatContextIn->metadata;
            }
*/
            inputStreams[i] = nullptr;
        }

        // initialize packet, set data to NULL, let the demuxer fill it
        pkt = av_packet_alloc();
        if (!pkt)
        {
            CleanUp();
            throw FFmpegException(std::string("Failed to create packet for input stream").c_str());
        }
        av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;
    }

    Demuxer::Demuxer(const char* s_fileName, int d_width, int d_height, int d_framerate)
    {
        m_fileName = s_fileName;
        m_width  = d_width;
        m_height = d_height;
        m_framerate = d_framerate;
        setVideoStreamDevice();
    }


    void Demuxer::setVideoStreamDevice ()
    {
        int ret = 0;
#ifdef _WIN32
        // Fixed by the operating system
        const char * input_device = "dshow"; // I'm using dshow when cross compiling :-)
#elif defined(__linux__)
        // libavutil, pixdesc.h
        const char * pix_fmt_name = av_get_pix_fmt_name(AV_PIX_FMT_YUVJ420P); // = "mjpeg"
        enum AVPixelFormat pix_name = av_get_pix_fmt("mjpeg");

        cout<<"AVPixelFormat name of AV_PIX_FMT_YUVJ420P : " << pix_fmt_name << "\n";
        cout<<"PixelFormat value for \"mjpeg\" : " << pix_name << "\n";

        // Fixed by the operating system
        const char * input_device = "v4l2";
#endif
        pAVFormatContextIn = avformat_alloc_context();
        pAVFormatContextIn->video_codec_id = AV_CODEC_ID_MJPEG;

        m_inputFormat = av_find_input_format(input_device);

        // WORKS OK TOO
        char videoSize[32];
        sprintf(videoSize, "%dx%d", this->m_width, this->m_height);
        av_dict_set(&options, "video_size", videoSize, 0);
        //av_dict_set(&options, "video_size", "1920x1080", 0); // Other way. Other usual values 1280x720@30fps

        const char * framerate_option_name = "framerate";
        char frameRateValue[10];
        sprintf(frameRateValue, "%d", this->m_framerate);

        av_dict_set(&options, framerate_option_name, frameRateValue, 0);
        std::cerr << "framerate_option_name :  " << framerate_option_name  << "\n";
        std::cerr << "frameRateValue        :  " << frameRateValue  << "\n";
//        av_dict_set(&options, "framerate", "30", 0);

        av_dict_set(&options, "pixel_format", pix_fmt_name, 0);  //  "mjpeg" "yuvj420p"
        av_dict_set(&options, "use_wallclock_as_timestamps", "1", 0);

        if ((ret = avformat_open_input(&pAVFormatContextIn, m_fileName, m_inputFormat, &options)) < 0)
        {
            std::cerr << "Failed to open input container "  <<  "\n";
            CleanUp();
            throw FFmpegException(std::string("Failed to open input container " + string(m_fileName)).c_str(), ret);
        }
#ifdef DEBUG
        else
            std::cerr << "open input container() DONE"  <<  "\n";
#endif

        // retrieve stream information
        if ( (ret = (avformat_find_stream_info(pAVFormatContextIn, NULL))) < 0)
        {
            CleanUp();
            throw FFmpegException(std::string("Failed to read streams from " + string(m_fileName)).c_str(), ret);
        }
#ifdef DEBUG
        else
            std::cerr << "avformat_find_stream_info() DONE"  <<  "\n";
#endif
        av_dump_format(pAVFormatContextIn , 0 , m_fileName , 0 );
#ifdef DEBUG
        std::cerr << "av_dump_format() DONE"  <<  "\n";
#endif
        int VideoStreamIndx = -1;

        for(unsigned int i=0; i<pAVFormatContextIn->nb_streams ;i++ )
        {
            if( pAVFormatContextIn->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) // if video stream found then get the index.
            {
                cout <<  "pAVFormatContextIn->streams["  << i <<  "] is an AVMEDIA_TYPE_VIDEO stream " << "\n";
                cout << "pAVFormatContextIn->->streams[" << i << "]->codec->codec_type = start + " 
                     << pAVFormatContextIn->streams[i]->codecpar->codec_type << "\n";
                cout << "AVMEDIA_TYPE_UNKNOWN = -1, AVMEDIA_TYPE_VIDEO == 0, AVMEDIA_TYPE_AUDIO == 1,AVMEDIA_TYPE_DATA == 2, AVMEDIA_TYPE_SUBTITLE = 3 " <<  "\n";
                cout << "pAVFormatContextIn->->streams[" << i << "]->codec->codec_id = " 
                     << pAVFormatContextIn->streams[i]->codecpar->codec_id << "\n";
                cout << " (NONE = start+0, MJPEG == start+7, MPEG4 == start+12, RAWVIDEO == start+13, H264 == start+27 )" << "\n";

                VideoStreamIndx = i;
                break;
            }
        }


        if((VideoStreamIndx) == -1)
        {
            cout<<"Error : video streams not found in demuxer ctor";
        }

        // inspired from https://code.mythtv.org/trac/ticket/13186?cversion=0&cnum_hist=2
        AVCodecContext *pAVCodecContext = NULL;
        pAVCodec = NULL;
        pAVCodec = avcodec_find_decoder(pAVFormatContextIn->streams[VideoStreamIndx]->codecpar->codec_id);
        pAVCodecContext = avcodec_alloc_context3(pAVCodec);
        avcodec_parameters_to_context(pAVCodecContext, pAVFormatContextIn->streams[VideoStreamIndx]->codecpar);

        inputStreams = new InputStream*[pAVFormatContextIn->nb_streams];

// TODO : understand why doing that
        for (unsigned int i = 0; i < pAVFormatContextIn->nb_streams; ++i)
        {
            inputStreams[i] = nullptr;
        }

        // initialize packet, set data to NULL, let the demuxer fill it
        pkt = av_packet_alloc();

        if (!pkt)
        {
            CleanUp();

            throw FFmpegException(std::string("Failed to create packet for input stream").c_str());
        }
        av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;

        avcodec_close(pAVCodecContext);
    }


    Demuxer::~Demuxer()
    {
        CleanUp();
    }

    void Demuxer::CleanUp()
    {
        if (inputStreams != nullptr)
        {
            for (unsigned int i = 0; i < pAVFormatContextIn->nb_streams; ++i)
            {
                inputStreams[i] = nullptr;
            }
            delete inputStreams;
            inputStreams = nullptr;
        }

        if (pAVFormatContextIn != nullptr)
        {
            avformat_close_input(&pAVFormatContextIn);
            pAVFormatContextIn = nullptr;
        }

        if (pkt != nullptr)
        {
            av_packet_free(&pkt);
            pkt = nullptr;
        }
    }

    void Demuxer::DecodeBestAudioStream(FrameSink* frameSink)
    {
        int ret = av_find_best_stream(pAVFormatContextIn, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

        if (ret < 0)
        {
            throw FFmpegException(std::string("Could not find " + string(av_get_media_type_string(AVMEDIA_TYPE_AUDIO)) + " stream in input file " + m_fileName).c_str(), ret);
        }

        int streamIndex = ret;
        return DecodeAudioStream(streamIndex, frameSink);
    }

    void Demuxer::DecodeBestVideoStream(FrameSink* frameSink)
    {
        int ret = av_find_best_stream(pAVFormatContextIn, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

        if (ret < 0)
        {
            throw FFmpegException(std::string("Could not find " + string(av_get_media_type_string(AVMEDIA_TYPE_VIDEO)) + " stream in input file " + m_fileName).c_str(), ret);
        }

        int streamIndex = ret;
        return DecodeVideoStream(streamIndex, frameSink);
    }

    void Demuxer::DecodeAudioStream(int streamIndex, FrameSink* frameSink)
    {
        // each input stream can only be used once
        if (inputStreams[streamIndex] != nullptr)
        {
            throw FFmpegException(std::string("That stream is already tied to a frame sink, you cannot process the same stream multiple times").c_str());
        }

        // create the stream
        InputStream* inputStream = GetInputStream(streamIndex);
        inputStream->Open(frameSink);

        // remember and return
        inputStreams[streamIndex] = inputStream;
    }

    void Demuxer::DecodeVideoStream(int streamIndex, FrameSink* frameSink)
    {
        // each input stream can only be used once
        if (inputStreams[streamIndex] != nullptr)
        {
            throw FFmpegException(std::string("That stream is already tied to a frame sink, you cannot process the same stream multiple times").c_str());
        }

        // create the stream
        InputStream* inputStream = GetInputStream(streamIndex);
        inputStream->Open(frameSink);

        // remember and return
        inputStreams[streamIndex] = inputStream;
    }

    InputStream* Demuxer::GetInputStream(int streamIndex)
    {
        // already exists
        if (inputStreams[streamIndex] != nullptr) return inputStreams[streamIndex];

        // The stream doesn't exist but we already processed all our frames, so it makes no sense
        // to add it anymore.
        if (IsDone())
            return nullptr;

        AVStream* stream = pAVFormatContextIn->streams[streamIndex];
        pAVCodec = CodecDeducer::DeduceDecoder(stream->codecpar->codec_id);

        if (pAVCodec == nullptr)
            return nullptr; // no codec found - we can't really do anything with this stream!

        switch (pAVCodec->type)
        {
            case AVMEDIA_TYPE_VIDEO:
                inputStreams[streamIndex] = new VideoInputStream(pAVFormatContextIn, stream);
            break;

            case AVMEDIA_TYPE_AUDIO:
                inputStreams[streamIndex] = new AudioInputStream(pAVFormatContextIn, stream);
            break;

            default:
            break;
        }

        // return the created stream
        return inputStreams[streamIndex];
    }

    InputStream* Demuxer::GetInputStreamById(int streamId)
    {
        // map the stream id to an index by going over all the streams and comparing the id
        for (unsigned int i = 0; i < pAVFormatContextIn->nb_streams; ++i)
        {
            AVStream* stream = pAVFormatContextIn->streams[i];

            if (stream->id == streamId)
                return GetInputStream(i);
        }

        // no match found
        return nullptr;
    }

    void Demuxer::PreparePipeline()
    {
        bool allPrimed = false;

        do
        {
            Step();

            // see if all input streams are primed
            allPrimed = true;

            for (unsigned int i = 0; i < pAVFormatContextIn->nb_streams; ++i)
            {
                InputStream* stream = inputStreams[i];

                if (stream != nullptr)
                {
                    if (!stream->IsPrimed())
                        allPrimed = false;
                }
            }

        } while (!allPrimed && !IsDone());

    }

    bool Demuxer::IsDone()
    {
        return done;
    }

    void Demuxer::Step()
    {
        // read frames from the file
        int ret = av_read_frame(pAVFormatContextIn, pkt);

        // EOF
        if (ret == AVERROR_EOF)
        {
            pkt->data = NULL;
            pkt->size = 0;

            for (unsigned int i = 0; i < pAVFormatContextIn->nb_streams; ++i)
            {
                InputStream* stream = inputStreams[i];

                if (stream != nullptr)
                {
                    pkt->stream_index = i;
                    DecodePacket();
                    stream->Close();
                }
            }

            done = true;
            return;
        }

        // not ready yet
        if (ret == AVERROR(EAGAIN)) return;

        // error
        if (ret < 0)
        {
            throw FFmpegException(std::string("Error during demuxing").c_str(), ret);
        }

        // decode the finished packet
        DecodePacket();
    }

    void Demuxer::DecodePacket()
    {
        int streamIndex = pkt->stream_index;
        InputStream* inputStream = inputStreams[streamIndex];

        if (inputStream != nullptr)
        {
            inputStream->DecodePacket(pkt);
        }

        // We need to unref the packet here because packets might pass by here
        // that don't have a stream attached to them. We want to dismiss them!
        av_packet_unref(pkt);
    }

    ContainerInfo Demuxer::GetInfo()
    {
        ContainerInfo info;

        // general data
        // the duration is calculated like this... why?
        int64_t duration = pAVFormatContextIn->duration + (pAVFormatContextIn->duration <= INT64_MAX - 5000 ? 5000 : 0);
        info.durationInMicroSeconds = duration;
        info.durationInSeconds = (float)info.durationInMicroSeconds / AV_TIME_BASE;
        info.start = (float)pAVFormatContextIn->start_time / AV_TIME_BASE;
        info.bitRate = pAVFormatContextIn->bit_rate;
        info.format = pAVFormatContextIn->iformat;

        // go over all streams and get their info
        for (unsigned int i = 0; i < pAVFormatContextIn->nb_streams; ++i)
        {
            InputStream* stream = GetInputStream(i);

            if (stream == nullptr) continue; // no valid stream
                stream->AddStreamInfo(&info);
        }

        return info;
    }

    int Demuxer::GetFrameCount(int streamId)
    {
        // Make sure all streams exist, so we can query them later.
        for (unsigned int i = 0; i < pAVFormatContextIn->nb_streams; ++i)
        {
            GetInputStream(i);
        }

        // Process the entire container so we can know how many frames are in each 
        while (!IsDone())
        {
            Step();
        }

        // Return the right stream's frame count.
        return GetInputStreamById(streamId)->GetFramesProcessed();
    }

    const char* Demuxer::GetFileName()
    {
        return m_fileName;
    }


    bool Demuxer::convertToRGB(int VideoStreamIndx, AVFormatContext *bAVFormatContext, AVCodecContext *bAVCodecContext, AVCodec * /*bAVCodec*/ )
    {
        AVPacket bAVPacket;
        AVFrame *bAVFrame = NULL;
        bAVFrame = av_frame_alloc();
        AVFrame *bAVFrameRGB = NULL;
        bAVFrameRGB = av_frame_alloc();

        if(bAVFrame == NULL)
            cout<<"\n\nframe alloc failed";

        if(bAVFrameRGB == NULL)
            cout<<"\n\nframe alloc RGB failed";

        int numBytes;
        uint8_t *buffer = NULL;
        numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, bAVCodecContext->width, bAVCodecContext->height, 1);

        buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

        av_image_fill_arrays (bAVFrameRGB->data,
                              bAVFrameRGB->linesize,
                              buffer,
                              AV_PIX_FMT_RGB24,
                              bAVCodecContext->width,
                              bAVCodecContext->height, 1);


        // TODO : FIXME once ffmpeg will have solved the problem
        // https://news.ycombinator.com/item?id=20036710

        struct SwsContext *sws_ctx = NULL;
        sws_ctx = sws_getContext( bAVCodecContext->width,
                                  bAVCodecContext->height,
                                  bAVCodecContext->pix_fmt,
                                  bAVCodecContext->width,
                                  bAVCodecContext->height,
                                  AV_PIX_FMT_RGB24,
                                  SWS_BILINEAR,
                                  NULL,NULL,NULL);
        int framefinish = 0;

        while(av_read_frame(bAVFormatContext,&bAVPacket) >=0)
        {
            if(bAVPacket.stream_index == VideoStreamIndx)
            {
                int ret = avcodec_receive_frame(bAVCodecContext, bAVFrame);

                if (ret == 0)
                    framefinish = 1;

                if (ret == AVERROR(EAGAIN))
                    ret = 0;

                if (ret == 0)
                    ret = avcodec_send_packet(bAVCodecContext, &bAVPacket);

                if(framefinish)
                {
                  // convert image from native format to RGB
                  sws_scale(sws_ctx , (uint8_t const* const *)bAVFrame->data ,
                  bAVFrame->linesize , 0, bAVCodecContext->height,
                  bAVFrameRGB->data , bAVFrameRGB->linesize);

                    // save frame to disk

                    //if(++i <= 100)
                    //SaveMyFrame(bAVFrameRGB , bAVCodecContext->width , bAVCodecContext->height , i );
                }
            }
        }
        av_free(bAVFrame);
        av_free(bAVFrameRGB);
        return true;
    }
}

