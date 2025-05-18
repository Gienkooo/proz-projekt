#pragma once

#include <mpi.h>
#include <string>
#include <iostream> 

#include "types.h"
#include "ClockManager.h"

class ProcessLogic;

class MessageHandler
{
public:
    MessageHandler(int process_id, int mpi_rank, int total_processes, ClockManager &clock_mgr);

    void sendMessage(int target_mpi_rank, MessageType type, int custom_ts = -1, int h_id = 0, int h_status = 0);
    void broadcastMessage(MessageType type, int custom_ts = -1, int h_id = 0, int h_status = 0);
    void listenForMessages(ProcessLogic *logic_ptr);
    void stopListening();

private:
    int my_id;
    int my_rank;
    int N_PROCESSES_CONST;
    ClockManager &clock_manager;
    std::atomic<bool> terminate_listening_flag;

    void log(const std::string &message_content);
};
