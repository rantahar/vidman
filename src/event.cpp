#include <QImage>
#include <QPixmap>
#include "common.h"
#include "event.h"
#include "eventcontainer.h"
#include "eventreader.h"
#include "settings.h"

void Event::apply(cv::Mat &)
{
    if(first_) {
        emit triggered(trigCode_, log_);
        first_ = false;
    }
}

void DelEvent::apply(EventContainer &events)
{
    emit triggered(trigCode_, log_);
    if(delType_)
        events.deleteType(delType_);
    else
        events.deleteId(delId_);

    ready_ = true;
}

FlipEvent::FlipEvent(int start, int delay, int id, int trigCode) :
    Event(EVENT_FLIP, start, delay, 0, id, trigCode)
{
}

void FlipEvent::apply(cv::Mat &frame)
{    
    Event::apply(frame);
    cv::flip(frame, frame, 1);
}

void FlipEvent::apply(EventContainer &events)
{
    events.deleteType(Event::EVENT_FLIP);
}

FadeInEvent::FadeInEvent(int start, int duration, int delay, int id, int trigCode) :
    Event(EVENT_FADEIN, start, delay, duration, id, trigCode), amount_(-255),
    stopped_(false)
{
    timerWithPause_.invalidate();
}

void FadeInEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    if(!stopped_) {
        if(!timerWithPause_.isValid()) {
            timerWithPause_.start();
            interval_ = duration_/255;
        }
        int msecsElapsed = timerWithPause_.msecsElapsed();
        amount_ = -255 + msecsElapsed/interval_;
        if(amount_ >= 0) {
            amount_ = 0;
            stopped_ = true;
        }
    }
    frame += cv::Scalar(amount_, amount_, amount_);
}

void FadeInEvent::apply(EventContainer &events)
{
    events.deleteType(Event::EVENT_FADEIN);
    events.deleteType(Event::EVENT_FADEOUT);
}

void FadeInEvent::pause()
{
    stopped_ = true;
    timerWithPause_.pause();
}

void FadeInEvent::unpause()
{
    stopped_ = false;
    timerWithPause_.resume();
}

FadeOutEvent::FadeOutEvent(int start, int duration, int delay, int id, int trigCode) :
    Event(EVENT_FADEOUT, start, delay, duration, id, trigCode), amount_(0),
    stopped_(false)
{
    timerWithPause_.invalidate();
}

void FadeOutEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    if(!stopped_) {
        if(!timerWithPause_.isValid()) {
            timerWithPause_.start();
            interval_ = duration_/255;
        }
        int msecsElapsed = timerWithPause_.msecsElapsed();
        amount_ = -msecsElapsed/interval_;
        if(amount_ <= -255) {
            amount_ = -255;
            stopped_ = true;
        }
    }
    frame += cv::Scalar(amount_, amount_, amount_);
}

void FadeOutEvent::apply(EventContainer &events)
{
    events.deleteType(Event::EVENT_FADEIN);
    events.deleteType(Event::EVENT_FADEOUT);
}

void FadeOutEvent::pause()
{
    stopped_ = true;
    timerWithPause_.pause();
}

void FadeOutEvent::unpause()
{
    stopped_ = false;
    timerWithPause_.resume();
}

ImageEvent::ImageEvent(int start, const cv::Point2i& pos, const cv::Mat& image,
                       int delay, int id, int trigCode) :
    Event(EVENT_IMAGE, start, delay, 0, id, trigCode), image_(image), pos_(pos)
{
}

void ImageEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    if(!frame.empty() && !image_.empty())
        overlayImage(frame, image_, frame, pos_);
}

//Code from Jepson's Blog http://jepsonsblog.blogspot.fi/2012/10/overlay-transparent-image-in-opencv.html
void ImageEvent::overlayImage(const cv::Mat &background, const cv::Mat &foreground,
  cv::Mat &output, const cv::Point2i& location)
{
  background.copyTo(output);

  // start at the row indicated by location, or at row 0 if location.y is negative.
  for(int y = std::max(location.y , 0); y < background.rows; ++y) {
    int fY = y - location.y; // because of the translation

    // we are done of we have processed all rows of the foreground image.
    if(fY >= foreground.rows)
      break;

    // start at the column indicated by location,

    // or at column 0 if location.x is negative.
    for(int x = std::max(location.x, 0); x < background.cols; ++x)
    {
      int fX = x - location.x; // because of the translation.

      // we are done with this row if the column is outside of the foreground image.
      if(fX >= foreground.cols)
        break;

      // determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
      double opacity =
        ((double)foreground.data[fY * foreground.step + fX * foreground.channels() + 3])/ 255.;


      // and now combine the background and foreground pixel, using the opacity,

      // but only if opacity > 0.
      for(int c = 0; opacity > 0 && c < output.channels(); ++c)
      {
        unsigned char foregroundPx =
          foreground.data[fY * foreground.step + fX * foreground.channels() + c];
        unsigned char backgroundPx =
          background.data[y * background.step + x * background.channels() + c];
        output.data[y*output.step + output.channels()*x + c] =
          backgroundPx * (1.-opacity) + foregroundPx * opacity;
      }
    }
  }
}

TextEvent::TextEvent(int start, const QString& str, cv::Scalar color,
                     const cv::Point2i& pos, int delay, int id, int trigCode) :
    Event(EVENT_TEXT, start, delay, 0, id, trigCode), color_(color),
    pos_(pos), str_(str)
{
}

void TextEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    cv::putText(frame, str_.toStdString(), pos_, cv::FONT_HERSHEY_DUPLEX, 1, color_, 2);
}

RotateEvent::RotateEvent(int start, int angle, int delay, int id, int trigCode)
    : Event(EVENT_ROTATE, start, delay, 0, id, trigCode), angle_(angle)
{
}

void RotateEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    cv::Point2f center(frame.cols/2., frame.rows/2.);
    cv::Mat rotMat = getRotationMatrix2D(center, angle_, 1.0);
    if(!frame.empty())
        cv::warpAffine(frame, frame, rotMat, cv::Size(frame.cols, frame.rows+1));
}

void RotateEvent::apply(EventContainer &events)
{
    events.deleteType(Event::EVENT_ROTATE);
}

FreezeEvent::FreezeEvent(int start, int delay, int id, int trigCode)
        : Event(EVENT_FREEZE, start, delay, 0, id, trigCode, FREEZE_PRIORITY),
          started_(false)
{
}

void FreezeEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    if(!started_) {
        frame.copyTo(frame_);
        started_ = true;
    }
    frame_.copyTo(frame);
}

void FreezeEvent::apply(EventContainer &events)
{
    events.deleteType(Event::EVENT_FREEZE);
}

ZoomEvent::ZoomEvent(int start, double scale, int duration, int delay, int id,
                     int trigCode) :
        Event(EVENT_ZOOM, start, delay, duration, id, trigCode),
        scale_(scale), stopped_(false)
{
    timer_.invalidate();
}

void ZoomEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    if(!stopped_) {
        if(!timer_.isValid()) {
            interval_ = (scale_ - 1) / duration_;
            timer_.start();
        }
        int msecsElapsed = timer_.msecsElapsed();
        coef_ = 1 + interval_*msecsElapsed;
        if(coef_ >= scale_) {
            coef_ = scale_;
            stopped_ = true;
        }
    }

    cv::Mat tmp;
    //cv::Size newSize(frame.cols * coef_, frame.rows * coef_);
    resize(frame, tmp, cv::Size(), coef_, coef_, cv::INTER_LINEAR);

    //Take only the center 640x480 of the resized image
    cv::Point p((tmp.cols - frame.cols) / 2, (tmp.rows - frame.rows) / 2);
    cv::Rect roi(p, frame.size());
    frame = tmp(roi).clone();
}

void ZoomEvent::pause()
{
    stopped_ = true;
    timer_.pause();
}

void ZoomEvent::unpause()
{
    stopped_ = false;
    timer_.resume();
}

RecordEvent::RecordEvent(int start, VideoPtr video, int delay, int duration,
                         int id, int trigCode) :
    Event(EVENT_RECORD, start, delay, duration, id, trigCode, RECORD_PRIORITY),
    video_(video), finished_(false), paused_(false)
{
    timer_.invalidate();
}

void RecordEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    if(!finished_ && !paused_) {
        if(!timer_.isValid())
            timer_.start();

        if(timer_.msecsElapsed() < duration_)
            video_->frames_.append(frame.clone());
        else
            finished_ = true;
    }

}

void RecordEvent::pause()
{
    paused_ = true;
    timer_.pause();
}

void RecordEvent::unpause()
{
    paused_ = false;
    timer_.resume();
}

PlaybackEvent::PlaybackEvent(int start, VideoPtr video, int delay, int duration,
                             int id, int trigCode) :
    Event(EVENT_PLAYBACK, start, delay, duration, id, trigCode, PLAYBACK_PRIORITY),
    video_(video), finished_(false), paused_(false)
{
    iter_ = video_->frames_.begin();
}

void PlaybackEvent::apply(cv::Mat &frame)
{
    Event::apply(frame);
    if(video_->frames_.empty())
        finished_ = true;
    if(!finished_ && !paused_) {
        iter_->copyTo(frame);
        if(++iter_ == video_->frames_.end())
            finished_ = true;
    }
}

void PlaybackEvent::pause()
{
    paused_ = true;
}

void PlaybackEvent::unpause()
{
    paused_ = false;
}

MotionDetectorEvent::MotionDetectorEvent(int start, int delay, int id, int trigCode) :
    Event(EVENT_DETECT_MOTION, start, delay, id, trigCode, MOTION_DETECTOR_PRIORITY),
    color_(true), isTracking_(true)
{
    Settings settings;
    sensitivity_ = settings.movementSensitivity;
}

void MotionDetectorEvent::apply(cv::Mat &frame)
{
    if(next_.empty()) {
        next_ = frame.clone();
        current_ = next_;
        prev_ = next_;
    }
    else {
        prev_ = current_;
        current_ = next_;
        next_ = frame.clone();
    }

    movement_ = next_.clone();
    if(nChanges() > sensitivity_) {
        if(min_x-10 > 0) min_x -= 10;
        if(min_y-10 > 0) min_y -= 10;
        if(max_x+10 < result_.cols-1) max_x += 10;
        if(max_y+10 < result_.rows-1) max_y += 10;

        // draw rectangle round the changed pixel
        cv::Point x(min_x,min_y);
        cv::Point y(max_x,max_y);
        cv::Rect rect(x,y);
        cv::rectangle(movement_,rect,cv::Scalar(0, 0, 255),2);
        if(isTracking_) {
            emit triggered(trigCode_, log_);
            isTracking_ = false;
        }
    }

    createMotionPixmap();
}

int MotionDetectorEvent::nChanges()
{
    cv::Mat d1, d2;
    cv::absdiff(prev_, next_, d1);
    cv::absdiff(current_, next_, d2);
    cv::bitwise_and(d1, d2, result_);
    cv::cvtColor(result_, result_, CV_BGR2GRAY);
    cv::threshold(result_, result_, 5, 255, CV_THRESH_BINARY);
    cv::erode(result_,result_,cv::Mat());
    cv::dilate(result_,result_,cv::Mat());

    int changes = 0;
    min_x = result_.cols; min_y = result_.rows;
    max_x = 0;
    max_y = 0;

    for(int i = 0; i < VIDEO_WIDTH; i += 2) {
        for(int j = 0; j < VIDEO_HEIGHT; j += 2) {
            if(static_cast<int>(result_.at<uchar>(j, i)) == 255) {
                changes++;
                if(min_x>i) min_x = i;
                if(max_x<i) max_x = i;
                if(min_y>j) min_y = j;
                if(max_y<j) max_y = j;
            }
        }
    }

    return changes;
}

void MotionDetectorEvent::apply(EventContainer &events)
{
    events.deleteType(Event::EVENT_DETECT_MOTION);
}

void MotionDetectorEvent::createMotionPixmap()
{
    QImage img;
    if(color_)
        img = QImage(movement_.data, movement_.cols, movement_.rows, movement_.step, QImage::Format_RGB888);
    else
        img = QImage(result_.data, result_.cols, result_.rows, result_.step, QImage::Format_Indexed8);
    QPixmap pixmap;
    pixmap.convertFromImage(img.rgbSwapped());
    emit pixmapReady(pixmap);
}


void MotionDetectorEvent::changeMovementFrameColor(bool color)
{
    color_ = color;
}