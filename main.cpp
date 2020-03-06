//#define USE_READLINE
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
using namespace std;

#define WIDTH "640"
#define HEIGHT "480"
#define FRATE "30/1"

#define FRATE_F "60/1"
#define OVERLAY_IMG_F "/home/ilya/Pictures/test.png"
#define FILE_PATH_F "/home/ilya/Desktop/gtest1.mp4"
#define H264_ENC_F "omxh264enc"
#define H264_ENC_PARAM_F ""
#define JPEG_DEC_F "nvjpegdec"
#define TARGET_URL_F "rtmp://a.rtmp.youtube.com/live2/4759-h2dr-qg53-ad4w live=2"

#define OVERLAY_IMG "/home/yosimura/Pictures/test.png"
#define FILE_PATH "/home/yosimura/Desktop/gtest1.mp4"
#define TARGET_URL "rtmp://127.0.0.1/live/test live=1"

#define H264_ENC "x264enc"
#define H264_ENC_PARAM " tune=zerolatency"
#define JPEG_DEC "jpegdec"
#define VIDEO_DEVICE "/dev/video0"
#define AUDIO_DEVICE "alsa_input.usb-046d_HD_Pro_Webcam_C920_35B755BF-02.analog-stereo" //mine
#define AUDIO_DEVICE_F "alsa_input.platform-sound.analog-stereo" //


#define GST_PREVIEW_CMD "gst-launch-1.0 -v -e \
videomixer name=mix sink_1::xpos=0 sink_1::ypos=0 ! videoconvert ! xvimagesink sync=false \
v4l2src device=$V_DEVICE \
! image/jpeg,width=$WIDTH,height=$HEIGHT,framerate=$FRATE \
! queue ! $JPEG_DEC ! videoconvert ! mix. \
multifilesrc location=\"$OVERLAY_PATH\" index=0 caps=\"image/png,framerate=\(fraction\)1/1\" \
! queue ! pngdec ! videoconvert ! mix."

#define GST_RECORD_STREAM_CMD "gst-launch-1.0 -v -e \
mp4mux name=fmux ! filesink location=$FILE_PATH sync=false \
flvmux streamable=true name=smux ! rtmpsink sync=false location='$URL' \
v4l2src device=$V_DEVICE \
! image/jpeg,width=$WIDTH,height=$HEIGHT,framerate=$FRATE \
! $JPEG_DEC ! queue max-size-buffers=0 max-size-bytes=0 max-size-time=0 ! $H264_ENC$H264_ENC_PARAM \
! tee name=tsplit \
! queue ! h264parse ! fmux. tsplit. \
! queue max-size-buffers=0 max-size-bytes=0 max-size-time=0 ! smux. \
pulsesrc device=$A_DEVICE do-timestamp=true \
! audio/x-raw,rate=32000,format=S16LE,channels=2 ! queue ! audioresample ! audio/x-raw,rate=44100 ! queue ! audioconvert ! voaacenc bitrate=96000 \
! tee name=asplit \
! queue max-size-buffers=0 max-size-bytes=0 max-size-time=0 ! fmux. asplit. \
! queue max-size-buffers=0 max-size-bytes=0 max-size-time=0 ! aacparse ! smux."

#define GST_ALL_CMD "gst-launch-1.0 -v -e \
videomixer name=mix sink_1::xpos=0 sink_1::ypos=0 ! videoconvert ! xvimagesink sync=false \
mp4mux name=fmux ! filesink location=$FILE_PATH sync=false \
flvmux streamable=true name=smux ! rtmpsink sync=false location='$URL' \
v4l2src device=$V_DEVICE \
! image/jpeg,width=$WIDTH,height=$HEIGHT,framerate=$FRATE \
! $JPEG_DEC \
! tee name=t1 \
t1. ! queue ! mix. \
t1. ! queue max-size-buffers=0 max-size-bytes=0 max-size-time=0 ! $H264_ENC$H264_ENC_PARAM ! tee name=t2 \
t2. ! queue ! h264parse ! fmux. \
t2. ! queue ! smux. \
pulsesrc device=$A_DEVICE do-timestamp=true \
! audio/x-raw,rate=32000,format=S16LE,channels=2 \
! queue ! audioresample ! audio/x-raw,rate=44100 ! queue ! audioconvert ! voaacenc bitrate=96000 \
! tee name=t3 \
t3. ! queue max-size-buffers=0 max-size-bytes=0 max-size-time=0 ! fmux. \
t3. ! queue max-size-buffers=0 max-size-bytes=0 max-size-time=0 ! aacparse ! smux. \
multifilesrc location=\"$OVERLAY_PATH\" index=0 caps=\"image/png,framerate=\(fraction\)1/1\" \
! queue ! pngdec ! videoconvert ! mix."

#define GST_VIDEO_STREAM_CMD "gst-launch-1.0 -v -e \
flvmux streamable=true name=smux ! rtmpsink sync=false location='$URL' \
v4l2src device=$V_DEVICE \
! image/jpeg,width=$WIDTH,height=$HEIGHT,framerate=$FRATE \
! $JPEG_DEC ! queue ! $H264_ENC$H264_ENC_PARAM ! smux."

char **get_input(char *input) {
    char **command = (char**)malloc(8 * sizeof(char *));
    char *separator = " ";
    char *parsed;
    int index = 0;

    parsed = strtok(input, separator);
    while (parsed != NULL) {
        command[index] = parsed;
        index++;

        parsed = strtok(NULL, separator);
    }

    command[index] = NULL;
    return command;
}

void get_args2(std::string cmd, std::vector<string>& result)
{
    // I have no idea what "paramids" etc. are for, so I've cut out all of that
    std::string          item;
    std::stringstream    ss(cmd);
    bool first = true;

    while(ss >> item){
        if(first)
        {
            first  = false;
            continue;
        }
        int quoteIndex = item.find("'");
        if (quoteIndex >= 0) {
          if (item[item.length() - 1] == '\'') {
            // special case: single word was needlessly quoted
            result.push_back(item);
          } else {
            // use the free-standing std::getline to read a "line" into another string
            // starting after the current word, delimited by double-quote
            std::string restOfItem;
            getline(ss, restOfItem, '\'');
            // That trailing quote is removed from the stream and not copied to restOfItem.
            item = item.replace(quoteIndex, 1, "");
            result.push_back(item + restOfItem);
          }
        } else {
          result.push_back(item);
        }
    }
}

void get_args(std::string cmd, std::vector<char *>& args)
{
    std::istringstream iss(cmd);

    std::string token;
    bool first = true;
    while(iss >> token) {
      char *arg = new char[token.size() + 1];
      copy(token.begin(), token.end(), arg);
      arg[token.size()] = '\0';
      if(first)
      {
          first = false;
          delete[] arg;
          continue;
      }
      args.push_back(arg);
    }
    args.push_back(0);
}

pid_t pid_previewer = 0;
pid_t pid_streamer = 0;

void stopPreviewer()
{
    if(pid_previewer > 0)
    {
        kill(pid_previewer, SIGTERM);
        pid_previewer = 0;
        cout << "Previewer has been stoped...\n";
    }
}

void stopStreamer()
{
    if(pid_streamer > 0)
    {
        kill(pid_streamer, SIGTERM);
        pid_streamer = 0;
        cout << "Streamer, Recorder has been stoped...\n";
    }
}

void stopAll()
{
    stopPreviewer();
    stopStreamer();
}

void print_usage()
{
    cout << "###Usage:\n";
    cout << "#Preview: preview start/stop\n";
    cout << "#Recoring&Streaming: stream start/stop\n";
    cout << "#Stop: stopall / exit\n";
}


int main(int argc, const char * argv[])
{
    char *h264enc = H264_ENC_F;
    char *h264enc_param = H264_ENC_PARAM_F;
    char *jpgdec = JPEG_DEC_F;
    char *frate = FRATE_F;
    char *overlay = OVERLAY_IMG_F;
    char *video_path = FILE_PATH_F;
    char *target_url = TARGET_URL_F;
    if(argc == 2 && strcmp(argv[1], "dev") == 0)
    {
        h264enc = H264_ENC;
        h264enc_param = H264_ENC_PARAM;
        jpgdec = JPEG_DEC;
        frate = FRATE;
        overlay = OVERLAY_IMG;
        video_path = FILE_PATH;
        target_url = TARGET_URL;
    }

    std::string preview_cmd = GST_PREVIEW_CMD;
    preview_cmd.replace(preview_cmd.find("$WIDTH"), 6, WIDTH);
    preview_cmd.replace(preview_cmd.find("$HEIGHT"), 7, HEIGHT);
    preview_cmd.replace(preview_cmd.find("$FRATE"), 6, frate);
    preview_cmd.replace(preview_cmd.find("$OVERLAY_PATH"), 13, overlay);
    preview_cmd.replace(preview_cmd.find("$JPEG_DEC"), 9, jpgdec);
    preview_cmd.replace(preview_cmd.find("$V_DEVICE"), 9, VIDEO_DEVICE);

    std::string record_stream_cmd = GST_RECORD_STREAM_CMD;
    record_stream_cmd.replace(record_stream_cmd.find("$WIDTH"), 6, WIDTH);
    record_stream_cmd.replace(record_stream_cmd.find("$HEIGHT"), 7, HEIGHT);
    record_stream_cmd.replace(record_stream_cmd.find("$FRATE"), 6, frate);
    record_stream_cmd.replace(record_stream_cmd.find("$URL"), 4, target_url);
    record_stream_cmd.replace(record_stream_cmd.find("$FILE_PATH"), 10, video_path);
    record_stream_cmd.replace(record_stream_cmd.find("$V_DEVICE"), 9, VIDEO_DEVICE);
    record_stream_cmd.replace(record_stream_cmd.find("$A_DEVICE"), 9, AUDIO_DEVICE);
    record_stream_cmd.replace(record_stream_cmd.find("$H264_ENC"), 9, h264enc);
    record_stream_cmd.replace(record_stream_cmd.find("$H264_ENC_PARAM"), 15, h264enc_param);
    record_stream_cmd.replace(record_stream_cmd.find("$JPEG_DEC"), 9, jpgdec);

    std::string video_stream_cmd = GST_VIDEO_STREAM_CMD;
    video_stream_cmd.replace(video_stream_cmd.find("$WIDTH"), 6, WIDTH);
    video_stream_cmd.replace(video_stream_cmd.find("$HEIGHT"), 7, HEIGHT);
    video_stream_cmd.replace(video_stream_cmd.find("$FRATE"), 6, frate);
    video_stream_cmd.replace(video_stream_cmd.find("$URL"), 4, target_url);
    video_stream_cmd.replace(video_stream_cmd.find("$V_DEVICE"), 9, VIDEO_DEVICE);
    video_stream_cmd.replace(video_stream_cmd.find("$H264_ENC"), 9, h264enc);
    video_stream_cmd.replace(video_stream_cmd.find("$H264_ENC_PARAM"), 15, h264enc_param);
    video_stream_cmd.replace(video_stream_cmd.find("$JPEG_DEC"), 9, jpgdec);

    std::string all_cmd = GST_ALL_CMD;
    all_cmd.replace(all_cmd.find("$WIDTH"), 6, WIDTH);
    all_cmd.replace(all_cmd.find("$HEIGHT"), 7, HEIGHT);
    all_cmd.replace(all_cmd.find("$FRATE"), 6, frate);
    all_cmd.replace(all_cmd.find("$URL"), 4, target_url);
    all_cmd.replace(all_cmd.find("$FILE_PATH"), 10, video_path);
    all_cmd.replace(all_cmd.find("$V_DEVICE"), 9, VIDEO_DEVICE);
    all_cmd.replace(all_cmd.find("$A_DEVICE"), 9, AUDIO_DEVICE);
    all_cmd.replace(all_cmd.find("$H264_ENC"), 9, h264enc);
    all_cmd.replace(all_cmd.find("$H264_ENC_PARAM"), 15, h264enc_param);
    all_cmd.replace(all_cmd.find("$JPEG_DEC"), 9, jpgdec);
    all_cmd.replace(all_cmd.find("$OVERLAY_PATH"), 13, overlay);

    std::vector<string> preview_args, stream_args, all_args;

    get_args2(preview_cmd, preview_args);
    //get_args2(record_stream_cmd, stream_args);
    get_args2(all_cmd, stream_args);
    //get_args2(video_stream_cmd, stream_args);

    char **command;
    char *input;
#ifndef USE_READLINE
    input = (char*)malloc(256);
    memset(input, 256, 0);
#endif
    //printf(record_stream_cmd.c_str());
    //printf("\n#########\n");

    while (1) {
#ifdef USE_READLINE
        input = readline("mystreamer> ");
#else
        std::string temp;
        cout << "mystreamer> ";
        std::getline (std::cin, temp);
        if(temp.length()==0)
            continue;
        memset(input, 256, 0);
        strcpy(input, temp.c_str());
#endif

        command = get_input(input);

        /* Never returns if the call is successful */
        if(0 == strcmp(command[0], "preview"))
        {
            if(command[1] != 0)
            {
                if(0 == strcmp(command[1], "start"))
                {
                    if(!pid_previewer)
                    {
                        pid_previewer = fork();
                        if(pid_previewer < 0)
                            cout << "Failed to start previewer!" << endl;
                        else if(pid_previewer == 0)
                        {
                            const char **_argv = new const char* [preview_args.size() + 1];
                            for (int j = 0;  j < preview_args.size();  ++j)
                                _argv [j] = preview_args[j].c_str();
                            _argv [preview_args.size()] = NULL;

                            int fd = open("/dev/null", O_WRONLY);
                            dup2(fd, 1);    /* make stdout a copy of fd (> /dev/null) */
                            dup2(fd, 2);    /* ...and same with stderr */
                            close(fd);      /* close fd */
                            execvp("gst-launch-1.0", (char**)_argv);
                        }
                        else
                            cout << "Previewer has been started.\n";
                        //pid_previewer = system2(preview_cmd.c_str(), 0, 0);
                    }
                    else
                        cout << "Previewer is running!\n";

                }
                else if(0 == strcmp(command[1], "stop"))
                {
                    stopPreviewer();
                }
                else
                {
                    cout << "Commant Format Error. previewer start / previewer stop\n";
                }

            }
        }
        else if(0 == strcmp(command[0], "stream"))
        {
            if(command[1] != 0)
            {
                if(0 == strcmp(command[1], "start"))
                {
                    if(!pid_streamer)
                    {
                        pid_streamer = fork();
                        if(pid_streamer < 0)
                            cout << "Failed to start streamer!" << endl;
                        else if(pid_streamer == 0)
                        {
                            const char **_argv = new const char* [stream_args.size() + 1];
                            for (int j = 0;  j < stream_args.size();  ++j)
                            {
                                _argv [j] = stream_args[j].c_str();
//                                cout << _argv [j] << endl;
                            }
                            _argv [stream_args.size()] = NULL;

                            int fd = open("/dev/null", O_WRONLY);
                            dup2(fd, 1);
                            dup2(fd, 2);
                            close(fd);
                            execvp("gst-launch-1.0", (char**)_argv);
                        }
                        else
                            cout << "Streamer has been started.\n";
                    }
                    else
                        cout << "Streamer is running.\n";
                }
                else if(0 == strcmp(command[1], "stop"))
                {
                    stopStreamer();
                }
                else
                {
                    print_usage();
                }
            }
        }
        else if(0 == strcmp(command[0], "stopall"))
        {
            stopAll();
        }
        else if(0 == strcmp(command[0], "exit"))
        {
            stopAll();
            break;
        }
        else
        {
            print_usage();
        }

        free(input);
        free(command);
    }
    // now exec with &args[0], and then:

    /*for(size_t i = 0; i < preview_args.size(); i++)
      delete[] preview_args[i];

    for(size_t i = 0; i < stream_args.size(); i++)
      delete[] stream_args[i];
*/
    return 0;
}
