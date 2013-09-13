#ifndef EVENT_H
#define EVENT_H

#include <cv.h>
#include <highgui.h>
#include <QTimer>
#include <common.h>

/*!
Event classes are used to process an acquired frame i.e. add an event to the video.
Event is an abstract base class that should be inherited by all subclasses. Event has
a pure virtual function apply(&frame) which takes a reference to an OpenCV matrice as
a parameter and which should be implemented by subclasses.
  */


enum EventType
{
    EVENT_NULL,
    EVENT_FLIP,
    EVENT_FADEIN,
    EVENT_FADEOUT,
    EVENT_IMAGE,
    EVENT_TEXT,
    EVENT_REMOVE,
    EVENT_ROTATE,
    EVENT_FREEZE
};

class Event
{
public:
                Event(EventType type, float start, float delay=0, float duration=0, int id = -1, int trigCode = 0);
    virtual     ~Event();

    virtual void apply(cv::Mat &frame) = 0;

    float       getStart() const;
    float       getDelay() const;
    float       getDuration() const;
    int         getId() const;
    EventType   getType() const;
    int         getTrigCode() const;

    void        appendLog(const QString &str);
    QString     getLog() const;

protected:
    EventType   type;

    float       start, delay, duration;
    int         id;
    int         trigCode;

    QString     log;
};


class RemoveEvent : public Event
{
public:

    RemoveEvent(float start, float delay, int removeId, int trigCode = 0);
    RemoveEvent(float start, float delay, EventType removeType, int trigCode = 0);

    void apply(cv::Mat &frame);

    EventType getRemoveType() const;
    int getRemoveId() const;

private:
    int         removeId;
    EventType   removeType;

};


class FlipEvent : public Event
{
public:

    FlipEvent(float start, float delay, int id = -1, int trigCode = 0);

    void apply(cv::Mat &frame);
};


class FadeInEvent : public QObject, public Event
{
    Q_OBJECT
public:
    FadeInEvent(float start, float duration = 5, float delay=0, int id = -1, int trigCode = 0);

    void apply(cv::Mat &frame);

private slots:
    void increaseAmount();

private:
    QTimer* timer;
    int     amount;
    bool    stopped;
};

class FadeOutEvent: public QObject, public Event
{
    Q_OBJECT
public:
    FadeOutEvent(float start, float duration = 5, float delay=0, int id = -1, int trigCode = 0);

    void apply(cv::Mat &frame);

private slots:
    void decreaseAmount();

private:
    QTimer* timer;
    int     amount;
    bool    stopped;
};

class ImageEvent : public Event
{
public:
    ImageEvent(float start, cv::Point2i pos,
               const cv::Mat &image, float delay, int id = -1, int trigCode = 0);

    void apply(cv::Mat &frame);
private:

    void overlayImage(const cv::Mat &background, const cv::Mat &foreground,
                                cv::Mat &output, cv::Point2i location);

    cv::Mat     image;
    cv::Point2i pos;
};


class TextEvent : public Event
{
public:
        TextEvent(float start, QString str, cv::Scalar color,
                  cv::Point2i pos, float delay, int id = -1, int trigCode = 0);

    void apply(cv::Mat &frame);

private:
    cv::Scalar  color;
    cv::Point2i pos;
    QString     str;
};


class RotateEvent : public Event
{
public:
    RotateEvent(float start, int angle, float delay, int id = -1, int trigCode = 0);

    void apply(cv::Mat &frame);

private:
    int angle;
};


class FreezeEvent: public Event
{
public:
    FreezeEvent(float start, float delay, int id = -1, int trigCode = 0);

    void apply(cv::Mat &frame);

private:
    bool    started;

    cv::Mat freezedFrame;
};

#endif // EVENT_H
