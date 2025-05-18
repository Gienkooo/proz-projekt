#pragma once

#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>

#include "types.h"
#include "ClockManager.h"
#include "MessageHandler.h"
#include "ResourceManager.h"

class ProcessLogic
{
public:
    ProcessLogic(int id, int rank, int n_procs, int d_houses, int p_pasers);
    void run();
    void stop();
    void processIncomingMessage(const Message &msg);

private:
    int my_id;
    int my_rank;
    const int N_PROCESSES_CONST;
    const int D_HOUSES_CONST;
    const int P_PASERS_CONST;

    ClockManager clock_manager;
    MessageHandler message_handler;
    ResourceManager resource_manager;

    ProcessState current_state;
    std::atomic<bool> terminate_flag;
    std::mt19937 rng;
    std::thread listener_thread_obj;

    void log(const std::string &message_content);
    bool shouldStartCycle();
    void simulateWork();

    void tryAcquireHouse();
    void tryAcquirePaser();
    void initiateReleaseSequence();
    void enterHouseCriticalSection();
    void enterPaserCriticalSection();
    void releaseAcquiredHouse();
    void releaseAcquiredPaser();
};