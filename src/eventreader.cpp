#include <iostream>
#include <QMessageBox>
#include <sys/ioctl.h>
#include "eventreader.h"
#include "videoevent.h"

EventReader::EventReader()
{
}

bool EventReader::loadEvents(const QStringList &strList, EventContainer<Event*>& events)
{
    events.clear();

    //Start reading the events, line by line
    for(int i = 0; i < strList.size(); i++)
    {

        QStringList split = strList[i].split(':');
        QString str = split[0].toLower().replace(" ", "").simplified();

        if(split.size() > 1)
        {

            if(str == "event")
            {
                if(!readEvent(split[1], events, i+1))
                    return false;
            }
            else if(str == "imageobject")
            {
                if(!readImageObject(split[1], i+1))
                    return false;
            }
            else if(str == "videoobject")
            {
                if(!readVideoObject(split[1], i+1))
                    return false;
            }
            else if(str == "removeevent")
            {
                if(!readRemoveEvent(split[1], events, i+1))
                    return false;
            }
            //Ignore comments
            else if(str[0] == '#' || split[0].simplified() == "");
            else
            {
                emit error(QString("Couldn't understand '%1' in line %2.").arg(split[0]).arg(i+1));
                return false;
            }
        }
        else if(!str.isEmpty())
        {
            emit error(QString("Couldn't understand line %1.").arg(i+1));
            return false;
        }
    }

    return true;
}

bool EventReader::readEvent(const QString &str, EventContainer<Event*>& events, int lineNumber)
{
    QStringList strList = str.split(',');

    //Event parameters
    int start = 0, duration = 0, delay=0;
    int x = 0, y = 0, imageId = 0, eventId = -1, angle = 0, trigCode = 0, videoId = 0;
    float scale = 1;
    bool imageIdOk = false;
    bool videoIdOk = false;
    QString text;
    cv::Scalar color(0, 0, 0);
    Event::EventType type = Event::EVENT_NULL;

    //Fill all the parameters required to create an event
    for(int i = 0; i < strList.size(); i++)
    {
        if(strList[i].contains("="))
        {
            QStringList split = strList[i].split('=');
            QString param = split[0].toLower().replace(" ", "");
            QString value = split[1].toLower().replace(" ", "");;

            if(param == "type")
            {
                if(value == "flip") type = Event::EVENT_FLIP;
                else if(value == "fadein") type = Event::EVENT_FADEIN;
                else if(value == "fadeout") type = Event::EVENT_FADEOUT;
                else if(value == "image") type = Event::EVENT_IMAGE;
                else if(value == "text") type = Event::EVENT_TEXT;
                else if(value == "freeze") type = Event::EVENT_FREEZE;
                else if(value == "rotate") type = Event::EVENT_ROTATE;
                else if(value == "detectmotion") type = Event::EVENT_DETECT_MOTION;
                else if(value == "zoom") type = Event::EVENT_ZOOM;
                else if(value == "record") type = Event::EVENT_RECORD;
                else if(value == "playback") type = Event::EVENT_PLAYBACK;
                else
                {
                    emit error(QString("Couldn't understand type '%1' in line %2.").arg(split[1]).arg(lineNumber));
                    return false;
                }
            }
            else if(param == "start")
            {
                if((start = toInt(split[1], lineNumber, QString("start time"))) == -1)
                    return false;
            }
            else if(param == "duration")
            {
                if((duration = toInt(split[1], lineNumber, QString("duration"))) == -1)
                    return false;
            }
            else if(param == "x")
            {
                if((x = toInt(split[1], lineNumber, QString("x-coordinate"))) == -1)
                    return false;
            }
            else if(param == "y")
            {
                if((y = toInt(split[1], lineNumber, QString("y-coordinate"))) == -1)
                    return false;
            }
            else if(param == "imageid")
            {
                if((imageId = toInt(split[1], lineNumber, QString("image ID"))) == -1)
                    return false;

                if(!imageContainer_.contains(imageId))
                {
                    emit error(QString("Couldn't find image object with id %1 in line %2").arg(imageId).arg(lineNumber));
                    return false;
                }
                imageIdOk = true;
            }
            else if(param == "videoid")
            {
                if((videoId = toInt(split[1], lineNumber, QString("image ID"))) == -1)
                    return false;

                if(!videoObjects_.contains(videoId))
                {
                    emit error(QString("Couldn't find video object with id %1 in line %2").arg(videoId).arg(lineNumber));
                    return false;
                }
                videoIdOk = true;
            }
            else if(param == "text")
            {
                text=split[1];
            }
            else if(param == "angle")
            {
                if((angle = toInt(split[1], lineNumber, QString("angle"))) == -1)
                    return false;
            }
            else if(param == "id")
            {
                if((eventId = toInt(split[1], lineNumber, QString("id"))) == -1)
                    return false;
            }
            else if(param == "delay")
            {
                if((delay = toInt(split[1], lineNumber, QString("delay"))) == -1)
                    return false;
            }
            else if(param == "trigcode")
            {
                if(value == "dtr") trigCode = TIOCM_DTR;
                else if(value == "rts") trigCode = TIOCM_RTS;
                else if((trigCode = toInt(split[1], lineNumber, QString("trigcode"))) == -1)
                    return false;
            }
            else if(param == "scale")
            {
                if((scale = toFloat(split[1], lineNumber, QString("scale"))) == -1)
                    return false;
                else if(scale < 1)
                {
                    emit error(QString("Scale must be greater than 1 for zoom event in line %1").arg(lineNumber));
                    return false;
                }
            }
            else if(param == "color")
            {
                if(value=="black") color=cv::Scalar(0, 0, 0);
                else if(value=="white") color=cv::Scalar(255, 255, 255);
                else if(value=="red") color=cv::Scalar(0, 0, 255);
                else if(value=="blue") color=cv::Scalar(255, 0, 0);
                else
                {
                    emit error(QString("Couldn't understand color '%1' in line %2. Try black, white, red or blue.").arg(split[1]).arg(lineNumber));
                    return false;
                }
            }
            else if(param.replace(" ", "").isEmpty());
            else
            {
                emit error(QString("Couldn't understand '%1' in line %2.").arg(param).arg(lineNumber));
                return false;
            }
        }

    }

    //Create the new event with acquired parameters
    Event* ev;
    switch(type)
    {
        case Event::EVENT_FLIP:
            ev = new FlipEvent(start, delay, eventId, trigCode);
            ev->appendLog(QString("Flip event added."));
            break;

        case Event::EVENT_FADEIN:
            ev = new FadeInEvent(start, duration, delay, eventId, trigCode);
            ev->appendLog(QString("Fade in event added. "));
            break;

        case Event::EVENT_FADEOUT:
            ev = new FadeOutEvent(start, duration, delay, eventId, trigCode);
            ev->appendLog(QString("Fade out event added. "));
            break;

        case Event::EVENT_IMAGE:
            if(imageIdOk)
            {
                ev = new ImageEvent(start, cv::Point2i(x, y), imageContainer_[imageId], delay, eventId, trigCode);
                ev->appendLog(QString("Image event added. "));
            }
            else
            {
                emit error(QString("Image event declared without object id in line %1").arg(lineNumber));
                return false;
            }
            break;

        case Event::EVENT_TEXT:
            ev = new TextEvent(start, text, color, cv::Point2i(x, y), delay, eventId, trigCode);
            ev->appendLog(QString("Text event added. "));
            break;

        case Event::EVENT_FREEZE:
            ev = new FreezeEvent(start, delay, eventId, trigCode);
            ev->appendLog(QString("Freeze event added. "));
            break;

        case Event::EVENT_ROTATE:
            ev = new RotateEvent(start, angle, delay, eventId, trigCode);
            ev->appendLog(QString("Rotate event added. "));
            break;

        case Event::EVENT_ZOOM:
            ev = new ZoomEvent(start, scale, duration, delay, eventId, trigCode);
            ev->appendLog(QString("Zoom event added"));
            break;

        case Event::EVENT_DETECT_MOTION:
            ev = new Event(Event::EVENT_DETECT_MOTION, start, delay, duration, eventId, trigCode);
            break;

        case Event::EVENT_RECORD:
            if(videoIdOk)
            {
                if(duration > videoLengths_[videoId])
                {
                    emit error(QString("Record event duration too big for video object in line %1").arg(lineNumber));
                    return false;
                }
                ev = new RecordEvent(start, videoObjects_[videoId], delay, duration, eventId, trigCode);
                ev->appendLog(QString("Record event added"));
            }
            else
            {
                emit error(QString("Record event declared without object id in line %1").arg(lineNumber));
                return false;
            }
            break;

        case Event::EVENT_PLAYBACK:
            if(videoIdOk)
            {
                ev = new PlaybackEvent(start, videoObjects_[videoId], delay, duration, eventId, trigCode);
                ev->appendLog(QString("Playback event added."));
            }
            else
            {
                emit error(QString("Playback event declared without object id in line %1").arg(lineNumber));
                return false;
            }
            break;

        default:
            emit error(QString("Event declared without type in line %1").arg(lineNumber));
            return false;

    }
    events.append(ev);

    return true;
}

bool EventReader::readImageObject(const QString &str, int lineNumber)
{
    QStringList strList = str.split(',');

    QString filename("");
    int id = 0;

    for(int i = 0; i < strList.size(); i++)
    {
        if(strList[i].contains("="))
        {
            QStringList split = strList[i].split('=');
            QString param = split[0].toLower().replace(" ", "");
            QString value = split[1];

            if(param == "filename")
            {
                filename = value;
            }

            else if(param == "id")
            {
                if((id = toInt(value, lineNumber, "id")) == -1)
                    return false;
            }
            else
            {
                emit error(QString("Couldn't understand '%1' in line %2.").arg(split[0].simplified()).arg(lineNumber));
                return false;
            }
        }
    }

    if(!imageContainer_.addImage(id, filename))
    {
        emit error(QString("Couldn't load image file '%1'.").arg(filename));
        return false;
    }

    return true;
}

bool EventReader::readVideoObject(const QString &str, int lineNumber)
{
    QStringList strList = str.split(',');

    int id = 0, length = 0;
    for(int i = 0; i < strList.size(); i++)
    {
        if(strList[i].contains("="))
        {
            QStringList split = strList[i].split('=');
            QString param = split[0].toLower().replace(" ", "");
            QString value = split[1];

            if(param == "id")
            {
                if((id = toInt(value, lineNumber, "id")) == -1)
                    return false;
            }
            else if(param == "length")
            {
                if((length = toInt(value, lineNumber, "length")) == -1)
                    return false;
            }
            else
            {
                emit error(QString("Couldn't understand '%1' in line %2.").arg(split[0].simplified()).arg(lineNumber));
                return false;
            }
        }
    }

    std::shared_ptr<QList<cv::Mat>> frames(new QList<cv::Mat>);
    //Reserve enough memory to hold all the frames
    frames->reserve(length/1000*settings_.fps + 10);
    videoObjects_.insert(id, frames);
    videoLengths_.insert(id, length);
    return true;
}

bool EventReader::readRemoveEvent(const QString &str, EventContainer<Event*>& events, int lineNumber)
{
    QStringList strList = str.split(',');

    //Event parameters
    int id = -1,  trigCode = 0;
    Event::EventType type = Event::EVENT_NULL;
    int start = 0, delay = 0;

    for(int i = 0; i < strList.size(); i++)
    {
        if(strList[i].contains("="))
        {
            QStringList split = strList[i].split("=");
            QString param = split[0].toLower().replace(" ", "");
            QString value = split[1].toLower().replace(" ", "");

            if(param == "id")
            {
                if((id = toInt(value, lineNumber, "id")) == -1)
                    return false;
            }

            else if(param == "type")
            {
                if(value == "flip") type = Event::EVENT_FLIP;
                else if(value == "fadein") type = Event::EVENT_FADEIN;
                else if(value == "fadeout") type = Event::EVENT_FADEOUT;
                else if(value == "image") type = Event::EVENT_IMAGE;
                else if(value == "text") type = Event::EVENT_TEXT;
                else if(value == "freeze") type = Event::EVENT_FREEZE;
                else if(value == "rotate") type = Event::EVENT_ROTATE;
                else if(value == "zoom") type = Event::EVENT_ZOOM;
                else
                {
                    emit error(QString("Couldn't understand type '%1' in line %2.").arg(split[1]).arg(lineNumber));
                    return false;
                }
            }
            else if(param == "start")
            {
                if((start = toInt(value, lineNumber, "start time")) == -1)
                    return false;
            }
            else if(param == "delay")
            {
                if((delay = toInt(value, lineNumber, QString("delay"))) == -1)
                    return false;
            }
            else if(param == "trigcode")
            {

                if(value == "dtr") trigCode = TIOCM_DTR;
                else if(value == "rts") trigCode = TIOCM_RTS;
                else if((trigCode = toInt(split[1], lineNumber, QString("trigcode"))) == -1)
                {
                    emit error(QString("Couldn't understand trigcode '%1' in line %2.").arg(split[1]).arg(lineNumber));
                    return false;
                }
            }
            else
            {
                emit error(QString("Couldn't understand '%1' in line %2.").arg(split[0].simplified()).arg(lineNumber));
                return false;
            }
        }
    }

    if(id > -1 && type)
    {
        emit error(QString("Remove event declared with id and type in line %1.").arg(lineNumber));
        return false;
    }
    else if(id > -1)
    {
        Event* ev = new RemoveEvent(start, delay, id, trigCode);
        ev->appendLog(QString("Event ID %1 removed. ").arg(id));
        events.append(ev);
    }
    else
    {
        Event* ev = new RemoveEvent(start, delay, type, trigCode);

        switch(type)
        {
            case Event::EVENT_FLIP:
                ev->appendLog(QString("Flip event removed. "));
                break;
            case Event::EVENT_FADEIN:
                ev->appendLog(QString("Fade in event removed. "));
                break;
            case Event::EVENT_FADEOUT:
                ev->appendLog(QString("Fade out event removed. "));
                break;
            case Event::EVENT_IMAGE:
                ev->appendLog(QString("Image event removed. "));
                break;
            case Event::EVENT_TEXT:
                ev->appendLog(QString("Text event removed. "));
                 break;
            case Event::EVENT_ROTATE:
                ev->appendLog(QString("Rotate event removed. "));
                break;
            case Event::EVENT_FREEZE:
                ev->appendLog(QString("Freeze event removed. "));
                break;

            case Event::EVENT_ZOOM:
                ev->appendLog(QString("Zoom event removed."));
                break;

            default:
                emit error(QString("Remove event declared without id or type in line %1").arg(lineNumber));
                delete ev;
                return false;
        }

        events.append(ev);
    }


    return true;

}

float EventReader::toFloat(const QString &str, int line, const QString &param) const
{
    bool ok;
    QString string = str.toLower().replace(" ", "");
    float num = string.toFloat(&ok);
    if(!ok)
    {
        emit error(QString("Couldn't convert %1 '%2' in line %3 to floating point").arg(param).arg(str).arg(line));
        return -1;
    }
    if(num < 0)
    {
        emit error(QString("%1 on line %2 must be a positive floating point").arg(param).arg(line));
        return -1;
    }
    return  num;
}

int EventReader::toInt(const QString &str, int line, const QString &param) const
{
    bool ok;
    QString string = str.toLower().replace(" ", "");
    int num = string.toInt(&ok);
    if(!ok)
    {
        emit error(QString("Couldn't convert %1 '%2' in line %3 to integer").arg(param).arg(str).arg(line));
        return -1;
    }
    if(num < 0)
    {
        emit error(QString("%1 on line %2 must be a positive integer").arg(param).arg(line));
        return -1;
    }
    return  num;
}