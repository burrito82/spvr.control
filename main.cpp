/*
 * Copyright (c) 2016
 *  Somebody
 */

char const copyright[] =
"Copyright (c) 2016\n\tSomebody.  All rights reserved.\n\n";

#include "driver/ControlInterface.h"
#include "driver/glm/gtx/quaternion.hpp"

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

static smartvr::ControlInterface *pControlInterface = nullptr;

/*void ReceiveTcp()
{
    while (true)
    {
        try
        {
            using boost::asio::ip::tcp;
            boost::asio::io_service oIoService{};
            tcp::acceptor oAcceptor{oIoService, tcp::endpoint{tcp::v4(), 4321}};
            tcp::socket oSocket{oIoService};
            oAcceptor.accept(oSocket);

            //boost::array<char, 0xff> aBuffer{};
            union
            {
                char c[0xff];
                float f[4];
            } aBuffer;
            std::memset(aBuffer.c, 0, sizeof(aBuffer.c));
            boost::system::error_code oError{};

            while (oError != boost::asio::error::eof)
            {
                auto uBytesRead = oSocket.read_some(boost::asio::buffer(aBuffer.c), oError);
                for (int iByte = 0; iByte < 0xff; iByte += 4)
                {
                    std::swap(aBuffer.c[iByte], aBuffer.c[iByte + 3]);
                    std::swap(aBuffer.c[iByte + 1], aBuffer.c[iByte + 2]);
                }
                std::cout << "received: (" << uBytesRead << ") {" << aBuffer.f[0] << ", "
                    << aBuffer.f[1] << ", "
                    << aBuffer.f[2] << ", "
                    << aBuffer.f[3] << "}" << std::endl;

                glm::vec3 const v3ViewDir{aBuffer.f[0], aBuffer.f[1], aBuffer.f[2]};
                glm::quat const qRotation = glm::rotation(glm::vec3{0.0f, 0.0f, -1.0f}, v3ViewDir);

                if (pControlInterface)
                {
                    pControlInterface->SetRotation(qRotation);
                }
            }
        }
        catch (...)
        {
            std::cout << "some error occurred..." << std::endl;
        }
    }
}*/

void ReceiveUdp()
{
    std::int32_t m_iLastMsg = -1;
    while (true)
    {
        try
        {
            using boost::asio::ip::udp;
            boost::asio::io_service oIoService{};
            udp::endpoint oEndpoint{udp::v4(), 4321};
            udp::socket oSocket{oIoService, oEndpoint};

            //boost::array<char, 0xff> aBuffer{};
            union
            {
                char c[0xff];
                struct
                {
                    float f[4];
                    std::int32_t m_iCounter;
                };
            } aBuffer;
            std::memset(aBuffer.c, 0, sizeof(aBuffer.c));
            boost::system::error_code oError{};

            while (oError != boost::asio::error::eof)
            {
                //auto uBytesRead = oSocket.read_some(boost::asio::buffer(aBuffer.c), oError);
                //auto uBytesRead = oSocket.receive_from(boost::asio::buffer(aBuffer.c), oEndpoint, 0, oError);
                auto uBytesRead = oSocket.receive(boost::asio::buffer(aBuffer.c));
                for (int iByte = 0; iByte < 0xff; iByte += 4)
                {
                    std::swap(aBuffer.c[iByte], aBuffer.c[iByte + 3]);
                    std::swap(aBuffer.c[iByte + 1], aBuffer.c[iByte + 2]);
                }
                if (aBuffer.m_iCounter > m_iLastMsg || aBuffer.m_iCounter < 1000)
                {
                    m_iLastMsg = aBuffer.m_iCounter;
                }
                else
                {
                    continue;
                }
                std::cout << "received: (" << uBytesRead << ") {"
                    << aBuffer.f[0] << ", \t"
                    << aBuffer.f[1] << ", \t"
                    << aBuffer.f[2] << ", \t"
                    << aBuffer.f[3] << "}" << "                          \r";//<< std::endl;

                //glm::vec3 const v3ViewDir{aBuffer.f[0], aBuffer.f[1], aBuffer.f[2]};
                //glm::quat qRotation = glm::rotation(glm::vec3{0.0f, 0.0f, -1.0f}, v3ViewDir);
                glm::quat qRotation{aBuffer.f[0], aBuffer.f[1], aBuffer.f[2], aBuffer.f[3]};
                static glm::quat qPreviousRotation = qRotation;
                float const fAlpha = 0.75f;
                qRotation.w = fAlpha * qRotation.w + (1.0f - fAlpha) * qPreviousRotation.w;
                qRotation.x = fAlpha * qRotation.x + (1.0f - fAlpha) * qPreviousRotation.x;
                qRotation.y = fAlpha * qRotation.y + (1.0f - fAlpha) * qPreviousRotation.y;
                qRotation.z = fAlpha * qRotation.z + (1.0f - fAlpha) * qPreviousRotation.z;
                qRotation = glm::normalize(qRotation);
                qPreviousRotation = qRotation;

                if (pControlInterface)
                {
                    pControlInterface->SetRotation(qRotation);
                }
            }
        }
        catch (...)
        {
            std::cout << "some error occurred..." << std::endl;
        }
    }
}

int main(int argc, char const *argv[])
{
	std::vector<std::string> vecArgs(argv, argv + argc);

    std::cout << copyright << std::endl;
	std::copy(vecArgs.begin(), vecArgs.end(), std::ostream_iterator<std::string>(std::cout, ", "));
    std::cout << "\n=== SmartVR Control ===" << std::endl;

    smartvr::ControlInterface oControlInterface{};
    pControlInterface = &oControlInterface;

    auto oThread = std::thread{ReceiveUdp};

    std::string strLogLine{};
    std::cout << "[ ] waiting for driver... ";
    std::cout.flush();
    do
    {
        std::this_thread::sleep_for(std::chrono::seconds{1});
        strLogLine = oControlInterface.PullLog();
    } while (strLogLine.empty());
    std::cout << "connected!" << std::endl;

    /*std::ofstream oLogFileHandle{"latest.log", std::ofstream::out};
    int iEmptyMsgCounter = 0;
    while (iEmptyMsgCounter < 10000)
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
    }*/

    std::cout << "[ ] No new message, stopped pulling! (driver might still be running!)" << std::endl;
    std::cin.ignore();

	return 0;
}

