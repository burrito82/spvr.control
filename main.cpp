/*
 * Copyright (c) 2016
 *  Somebody
 */

char const copyright[] =
"Copyright (c) 2016\n\tSomebody.  All rights reserved.\n\n";

#include "driver/ControlInterface.h"
#include "driver/glm/gtx/quaternion.hpp"
#include "json/json.h"

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

static spvr::ControlInterface *g_pControlInterface = nullptr;

struct SpvrPacket
{
    float f[4];
    std::int32_t m_iCounter;
};

union SpvrPacketByteHack
{
    char c[sizeof(SpvrPacket)];
    SpvrPacket data;
};

void NtoH(SpvrPacketByteHack &packet)
{
    // not the way to to... UB!
    for (int iByte = 0; iByte < sizeof(packet.c); iByte += 4)
    {
        std::swap(packet.c[iByte], packet.c[iByte + 3]);
        std::swap(packet.c[iByte + 1], packet.c[iByte + 2]);
    }
}

void ProcessPacket(SpvrPacket const &packet)
{
    std::cout << "received: {"
        << packet.f[0] << ", \t"
        << packet.f[1] << ", \t"
        << packet.f[2] << ", \t"
        << packet.f[3] << "}" << "                          \r";//<< std::endl;

    glm::quat qRotation{packet.f[0], packet.f[1], packet.f[2], packet.f[3]};
    static glm::quat qPreviousRotation = qRotation;
    float const fAlpha = 0.90f;
    qRotation.w = fAlpha * qRotation.w + (1.0f - fAlpha) * qPreviousRotation.w;
    qRotation.x = fAlpha * qRotation.x + (1.0f - fAlpha) * qPreviousRotation.x;
    qRotation.y = fAlpha * qRotation.y + (1.0f - fAlpha) * qPreviousRotation.y;
    qRotation.z = fAlpha * qRotation.z + (1.0f - fAlpha) * qPreviousRotation.z;
    qRotation = glm::normalize(qRotation);
    qPreviousRotation = qRotation;

    g_pControlInterface->SetRotation(qRotation);
}

void ReceiveTcp()
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

            SpvrPacketByteHack oPacket;
            std::memset(oPacket.c, 0, sizeof(oPacket.c));
            boost::system::error_code oError{};

            while (oError != boost::asio::error::eof)
            {
                auto uBytesRead = oSocket.read_some(boost::asio::buffer(oPacket.c), oError);
                NtoH(oPacket);
                ProcessPacket(oPacket.data);
            }
        }
        catch (...)
        {
            std::cout << "some error occurred..." << std::endl;
        }
    }
}

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

            SpvrPacketByteHack oPacket;
            std::memset(oPacket.c, 0, sizeof(oPacket.c));
            boost::system::error_code oError{};

            while (oError != boost::asio::error::eof)
            {
                //auto uBytesRead = oSocket.read_some(boost::asio::buffer(aBuffer.c), oError);
                //auto uBytesRead = oSocket.receive_from(boost::asio::buffer(aBuffer.c), oEndpoint, 0, oError);
                auto uBytesRead = oSocket.receive(boost::asio::buffer(oPacket.c));
                NtoH(oPacket);
                if (oPacket.data.m_iCounter > m_iLastMsg || oPacket.data.m_iCounter < 1000)
                {
                    m_iLastMsg = oPacket.data.m_iCounter;
                }
                else
                {
                    continue;
                }
                ProcessPacket(oPacket.data);
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
    std::string strJsonConfigFile = "config.json";
    if (vecArgs.size() > 1)
    {
        strJsonConfigFile = vecArgs[1];
    }

    std::cout << "=== SmartVR Control ===" << std::endl;
    std::cout << copyright << std::endl;

    spvr::ControlInterface oControlInterface{};
    // GLOBAL VARIABLE...
    g_pControlInterface = &oControlInterface;

    {
        std::ifstream oJsonConfigFile{strJsonConfigFile, std::ifstream::in};
        if (oJsonConfigFile)
        {
            std::cout << "[ ] reading configuration from " << strJsonConfigFile << std::endl;
            Json::Value jsonRoot{};
            oJsonConfigFile >> jsonRoot;
            auto const jsonDistortionK0 = jsonRoot["distortion-k0"];
            auto const jsonDistortionK1 = jsonRoot["distortion-k1"];
            if (!jsonDistortionK0.empty() && !jsonDistortionK1.empty())
            {
                auto const k0 = jsonDistortionK0.asFloat();
                auto const k1 = jsonDistortionK1.asFloat();
                oControlInterface.SetDistortionCoefficients(k0, k1);
            }

            if (!jsonRoot["distortion-scale"].empty())
            {
                oControlInterface.SetDistortionScale(jsonRoot["distortion-scale"].asFloat());
            }
        }
    }

    float k0, k1;
    oControlInterface.GetDistortionCoefficients(k0, k1);
    std::cout << "[.] Distortion coefficients: " << k0 << ", " << k1 << std::endl;
    std::cout << "[.] Distortion scale: " << oControlInterface.GetDistortionScale() << std::endl;

    //auto oThread = std::thread{ReceiveTcp};
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

    std::ofstream oLogFileHandle{"latest.log", std::ofstream::out};
    int iEmptyMsgCounter = 0;
    while (iEmptyMsgCounter < 10000)
    {
        if (strLogLine.empty())
        {
            ++iEmptyMsgCounter;
        }
        else
        {
            std::cout << strLogLine;
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

