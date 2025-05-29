#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

void Notepad_plus_Window::loadLastSession()
{
    if (sessionLoadingTime.count() > 0)
    {
        std::wstringstream wss;
        auto hms = std::chrono::hh_mm_ss{ std::chrono::duration_cast<std::chrono::milliseconds>(sessionLoadingTime) };
        std::wstringstream timeStr;
        timeStr << std::setfill(L'0') << std::setw(2) << hms.hours().count() << L":"
               << std::setw(2) << hms.minutes().count() << L":"
               << std::setw(2) << hms.seconds().count() << L"."
               << std::setw(3) << hms.subseconds().count();
        wss << L"Last session loading: " << timeStr.str() << std::endl;
    }
} 