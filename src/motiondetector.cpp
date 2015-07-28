#include <iostream>
#include "common.h"
#include "motiondetector.h"

MotionDetector::MotionDetector() : isTracking_(false), event_(nullptr)
{
}

bool MotionDetector::movementDetected(const cv::Mat &frame)
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

    cv::Mat d1, d2;
    cv::absdiff(prev_, next_, d1);
    cv::absdiff(current_, next_, d2);
    cv::bitwise_and(d1, d2, result_);
    cv::cvtColor(result_, result_, CV_BGR2GRAY);
    cv::threshold(result_, result_, 5, 255, CV_THRESH_BINARY);
    cv::erode(result_,result_,cv::Mat());
    cv::dilate(result_,result_,cv::Mat());

    int nChanges = 0;
    int min_x, max_x, min_y, max_y;
    min_x = result_.cols; min_y = result_.rows;
    max_x = 0;
    max_y = 0;

    for(int i = 0; i < VIDEO_WIDTH; i++) {
        for(int j = 0; j < VIDEO_HEIGHT; j++) {
            if(static_cast<int>(result_.at<uchar>(j, i)) == 255) {
                nChanges++;
                if(min_x>i) min_x = i;
                if(max_x<i) max_x = i;
                if(min_y>j) min_y = j;
                if(max_y<j) max_y = j;
            }
        }
    }

    cv::cvtColor(result_, result_, CV_GRAY2BGR);
    if(nChanges) {

        if(min_x-10 > 0) min_x -= 10;
        if(min_y-10 > 0) min_y -= 10;
        if(max_x+10 < result_.cols-1) max_x += 10;
        if(max_y+10 < result_.rows-1) max_y += 10;
        // draw rectangle round the changed pixel
        cv::Point x(min_x,min_y);
        cv::Point y(max_x,max_y);
        cv::Rect rect(x,y);
        cv::rectangle(result_,rect,cv::Scalar(0, 0, 255),1);
        if(isTracking_) {
            isTracking_ = false;
            return true;
        }
    }

    return false;
}

QPixmap MotionDetector::foregroundPixmap() const
{
    QImage img = QImage(result_.data, result_.cols, result_.rows, result_.step, QImage::Format_RGB888);
    QPixmap pixmap;
    pixmap.convertFromImage(img.rgbSwapped());
    return pixmap;
}

void MotionDetector::startTracking(Event* ev)
{
    event_.reset(ev);
    isTracking_ = true;
}

int MotionDetector::getEventTrigCode() const
{
    if(event_)
        return event_->getTrigCode();

    return 0;
}

QString MotionDetector::getEventLog() const
{
    if(event_)
        return event_->getLog();

    return QString();
}
