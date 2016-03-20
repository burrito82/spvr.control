/*
 * Copyright (c) 2016
 *  Somebody
 */

char const copyright[] =
"Copyright (c) 2016\n\tSomebody.  All rights reserved.\n\n";

#include "driver/ControlInterface.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char const *argv[])
{
	std::vector<std::string> vecArgs(argv, argv + argc);

    std::cout << copyright << std::endl;
	std::copy(vecArgs.begin(), vecArgs.end(), std::ostream_iterator<std::string>(std::cout, ", "));
    std::cout << "\n=== SmartVR Control ===" << std::endl;

    smartvr::ControlInterface oControlInterface{};

    std::string strLogLine{};
    std::cout << "[ ] waiting for driver... ";
    std::cout.flush();
    do
    {
        std::this_thread::sleep_for(std::chrono::seconds{1});
        strLogLine = oControlInterface.PullLog();
    } while (strLogLine.empty());
    std::cout << "connected!" << std::endl;

    std::ofstream oLogFileHandle{"latest.log", std::ofstream::out};
    int iEmptyMsgCounter = 0;
    while (iEmptyMsgCounter < 1000)
    {
        if (strLogLine.empty())
        {
            ++iEmptyMsgCounter;
        }
        else
        {
            std::cout << "LOG: " << strLogLine;
            oLogFileHandle << strLogLine;
            std::cout.flush();
            iEmptyMsgCounter = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
        strLogLine = oControlInterface.PullLog();
    }

    std::cout << "[ ] No new message, stopped pulling! (driver might still be running!)" << std::endl;
    std::cin.ignore();

	return 0;
}

