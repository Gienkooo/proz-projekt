#include "MessageHandler.h"
#include "ProcessLogic.h"

MessageHandler::MessageHandler(int process_id, int mpi_rank, int total_processes, ClockManager &clock_mgr)
    : my_id(process_id), my_rank(mpi_rank), N_PROCESSES_CONST(total_processes), clock_manager(clock_mgr), terminate_listening_flag(false) {}

void MessageHandler::log(const std::string &message_content)
{
    std::cout << "[MsgHandler P" << my_id << " C" << clock_manager.getTime() << "] " << message_content << std::endl;
}

void MessageHandler::sendMessage(int target_mpi_rank, MessageType type, int custom_ts, int h_id, int h_status)
{
    clock_manager.increment();
    int send_timestamp = (custom_ts != -1) ? custom_ts : clock_manager.getTime();

    int buf[5];
    buf[0] = static_cast<int>(type);
    buf[1] = my_id;
    buf[2] = send_timestamp;
    buf[3] = h_id;
    buf[4] = h_status;
    MPI_Send(buf, 5, MPI_INT, target_mpi_rank, 0, MPI_COMM_WORLD);
}

void MessageHandler::broadcastMessage(MessageType type, int custom_ts, int h_id, int h_status)
{
    clock_manager.increment();
    int broadcast_timestamp = (custom_ts != -1) ? custom_ts : clock_manager.getTime();

    for (int i = 0; i < N_PROCESSES_CONST; ++i)
    {
        if (i != my_rank)
        {
            int buf[5];
            buf[0] = static_cast<int>(type);
            buf[1] = my_id;
            buf[2] = broadcast_timestamp;
            buf[3] = h_id;
            buf[4] = h_status;
            MPI_Send(buf, 5, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }
}

void MessageHandler::listenForMessages(ProcessLogic *logic_ptr)
{
    MPI_Status status;
    int buf[5];

    while (!terminate_listening_flag.load())
    {
        int flag = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

        if (flag)
        {
            MPI_Recv(buf, 5, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            Message msg;
            msg.type = static_cast<MessageType>(buf[0]);
            msg.sender_id = buf[1];
            msg.timestamp = buf[2];
            msg.house_id = buf[3];
            msg.new_house_status = buf[4];

            clock_manager.updateOnReceive(msg.timestamp);

            if (logic_ptr)
            {
                logic_ptr->processIncomingMessage(msg);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void MessageHandler::stopListening()
{
    terminate_listening_flag = true;
}
