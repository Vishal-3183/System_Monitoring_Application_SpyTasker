#include "EventLogReader.h"
#include <windows.h>
#include <winevt.h>
#pragma comment(lib, "wevtapi.lib")

int EventLogReader::countFailedLogins(int withinLastMinutes) {
    int count = 0;

    long long timeWindow = withinLastMinutes * 60 * 1000;

    const wchar_t* query =
        L"<QueryList>"
        L"  <Query Id=\"0\" Path=\"Security\">"
        L"    <Select Path=\"Security\">"
        L"      *[System[(EventID=4625) and TimeCreated[timediff(@SystemTime) <= 300000]]]"
        L"    </Select>"
        L"  </Query>"
        L"</QueryList>";

    EVT_HANDLE hResults = EvtQuery(nullptr, L"Security", query, EvtQueryForwardDirection);
    if (!hResults) return -1;

    DWORD returned = 0;
    EVT_HANDLE events[10];

    while (EvtNext(hResults, 10, events, INFINITE, 0, &returned)) {
        count += returned;
        for (DWORD i = 0; i < returned; i++) {
            EvtClose(events[i]);
        }
    }

    EvtClose(hResults);
    return count;
}
