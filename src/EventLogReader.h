#ifndef EVENTLOGREADER_H
#define EVENTLOGREADER_H

class EventLogReader {
public:
    static int countFailedLogins(int withinLastMinutes = 5);
};

#endif // EVENTLOGREADER_H
