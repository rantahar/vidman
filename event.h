#ifndef EVENT_H
#define EVENT_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <QTimer>
#include <common.h>
#include "timerwithpause.h"

/*!
Event objects are used to specify the starting time, duration, effects etc.
of events handled by the program.
  */

class Event
{
public:

    //All possible events
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
        EVENT_FREEZE,
        EVENT_DETECT_MOTION
    };

                explicit Event(Event::EventType type, int start, int delay=0, int duration=0, int id = -1, int trigCode = 0);
                virtual ~Event();

    int         getStart() const;
    int         getDelay() const;
    int         getDuration() const;
    int         getId() const;
    EventType   getType() const;
    int         getTrigCode() const;

    void        appendLog(const QString &str);
    QString     getLog() const;

    virtual void pause();
    virtual void unpause();

protected:
    EventType   type;

    int         start, delay, duration;
    int         id;
    int         trigCode;

    QString     log;

private:
    Event(const Event& other);
    Event& operator=(const Event& other);
};

//This class is used to specify what event should be removed
class RemoveEvent : public Event
{
public:

    //Remove event can be initialized to remove either an event with a specific id
    //or all the events of a given type
    explicit RemoveEvent(int start, int delay, int removeId, int trigCode = 0);
    explicit RemoveEvent(int start, int delay, EventType removeType, int trigCode = 0);

    EventType getRemoveType() const;
    int getRemoveId() const;

private:
    int         removeId;
    EventType   removeType;

};

#endif // EVENT_H
