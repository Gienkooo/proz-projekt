#include "ProcessLogic.h"

ProcessLogic::ProcessLogic(int id, int rank, int n_procs, int d_houses, int p_pasers)
    : my_id(id), my_rank(rank),
      N_PROCESSES_CONST(n_procs), D_HOUSES_CONST(d_houses), P_PASERS_CONST(p_pasers),
      clock_manager(),
      message_handler(id, rank, n_procs, clock_manager),
      resource_manager(id, n_procs, d_houses, p_pasers, clock_manager, message_handler),
      current_state(ProcessState::IDLE), terminate_flag(false)
{
    rng.seed(my_id + std::chrono::system_clock::now().time_since_epoch().count());
    log("ProcessLogic initialized.");
}

void ProcessLogic::log(const std::string &message_content)
{
    std::cout << "[Logic P" << my_id << " C" << clock_manager.getTime() << " S:" << static_cast<int>(current_state) << "] " << message_content << std::endl;
}

void ProcessLogic::run()
{
    //log("Process " + std::to_string(my_id) + " starting run loop.");
    auto start_time = std::chrono::steady_clock::now();

    listener_thread_obj = std::thread([this]()
                                      { this->message_handler.listenForMessages(this); });

    while (!terminate_flag.load())
    {
        { // Scope for lock_guard
            std::lock_guard<std::mutex> lock(resource_manager.getMutex());

            switch (current_state)
            {
            case ProcessState::IDLE:
                if (shouldStartCycle())
                {
                    log("IDLE: ShouldStartCycle is true. Transitioning to WANT_HOUSE.");
                    current_state = ProcessState::WANT_HOUSE;
                    resource_manager.requestHouse();
                }
                break;

            case ProcessState::WANT_HOUSE:
                if (resource_manager.isRequestingHouse() && resource_manager.allHouseRepliesReceived())
                {
                    log("WANT_HOUSE: All replies received. Entering CS for House.");
                    enterHouseCriticalSection();
                }
                break;

            case ProcessState::HAVE_HOUSE_WANT_PASER:
                if (!resource_manager.isRequestingPaser())
                {
                    log("HAVE_HOUSE_WANT_PASER: Requesting paser.");
                    resource_manager.requestPaser();
                }
                else if (resource_manager.sufficientPaserRepliesReceived())
                {
                    log("HAVE_HOUSE_WANT_PASER: All replies received. Entering CS for Paser.");
                    enterPaserCriticalSection();
                }
                break;

            case ProcessState::HAVE_BOTH:
                log("HAVE_BOTH: (This state is usually brief, work simulation leads to RELEASING)");
                break;

            case ProcessState::RELEASING:
                if (resource_manager.isHouseHeld())
                {
                    log("RELEASING: House is held. Proceeding to release it.");
                    releaseAcquiredHouse();
                }
                else if (resource_manager.isPaserHeld())
                {
                    log("RELEASING: Paser is held. Proceeding to release it.");
                    releaseAcquiredPaser();
                }
                else
                {
                    log("RELEASING: No resources held. Transitioning to IDLE.");
                    current_state = ProcessState::IDLE;
                }
                break;
            }

            if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(600))
            {
                log("Run loop timeout. Signaling termination.");
                stop();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    //log("Main run loop in ProcessLogic finished for process " + std::to_string(my_id) + ". Waiting for listener thread...");

    message_handler.stopListening();
    if (listener_thread_obj.joinable())
    {
        listener_thread_obj.join();
    }
    log("Listener thread joined.");
}

void ProcessLogic::stop()
{
    log("Stop called. Setting terminate_flag.");
    terminate_flag = true;
}

void ProcessLogic::processIncomingMessage(const Message &msg)
{
    std::lock_guard<std::mutex> lock(resource_manager.getMutex());
    log("Processing incoming msg type " + std::to_string(static_cast<int>(msg.type)) + " from " + std::to_string(msg.sender_id));

    switch (msg.type)
    {
    case MessageType::REQUEST_HOUSE:
        resource_manager.handleHouseRequest(msg);
        break;
    case MessageType::REPLY_HOUSE:
        resource_manager.handleHouseReply(msg);
        break;
    case MessageType::REQUEST_PASER:
        resource_manager.handlePaserRequest(msg);
        break;
    case MessageType::REPLY_PASER:
        resource_manager.handlePaserReply(msg);
        break;
    case MessageType::UPDATE_HOUSE_STATE:
        resource_manager.updateLocalHouseState(msg.house_id, msg.new_house_status);
        break;
    }
    log("Finished processing incoming msg type " + std::to_string(static_cast<int>(msg.type)));
}

bool ProcessLogic::shouldStartCycle()
{
    if (current_state == ProcessState::IDLE)
    {
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng) <= 25;
    }
    return false;
}

void ProcessLogic::simulateWork()
{
    log("Simulating work with house and paser...");

    resource_manager.getMutex().unlock();
    std::uniform_int_distribution<int> dist(4000, 5000); // milliseconds
    std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
    resource_manager.getMutex().lock(); // TODO rozważyć zmianę na unique_lock, bo to może nie być do końca poprawne

    log("Work simulation complete. Transitioning to RELEASING.");
    current_state = ProcessState::RELEASING;
}

void ProcessLogic::enterHouseCriticalSection()
{
    log("Attempting to enter House CS.");
    bool acquired_house = false;
    int chosen_house_id = 0;

    if (D_HOUSES_CONST > 0)
    {
        auto house_states = resource_manager.getHouseStates();
        for (int k = 1; k <= D_HOUSES_CONST; ++k)
        {
            if (house_states.count(k) == 0 || house_states.at(k) == HOUSE_STATE_FREE)
            {
                chosen_house_id = k;
                acquired_house = true;
                break;
            }
        }
    }

    if (acquired_house)
    {
        resource_manager.recordHouseAcquired(chosen_house_id);
        log("Acquired house " + std::to_string(chosen_house_id) + ". Transitioning to HAVE_HOUSE_WANT_PASER.");
        current_state = ProcessState::HAVE_HOUSE_WANT_PASER;
    }
    else
    {
        log("No free house found. Returning to IDLE.");
        current_state = ProcessState::IDLE;
        resource_manager.processDeferredQueues(ResourceType::HOUSE_RESOURCE);
    }
}

void ProcessLogic::enterPaserCriticalSection()
{
    log("Attempting to enter Paser CS.");

    bool acquired_paser = false;
    if (P_PASERS_CONST > 0)
    {
        acquired_paser = true;
    }

    if (acquired_paser)
    {
        resource_manager.recordPaserAcquired();
        log("Acquired a paser. Transitioning to HAVE_BOTH.");
        current_state = ProcessState::HAVE_BOTH;
        simulateWork();
    }
    else
    {
        log("Could not acquire a paser (P=" + std::to_string(P_PASERS_CONST) + "). Releasing house and returning to IDLE.");
        current_state = ProcessState::RELEASING;
        resource_manager.processDeferredQueues(ResourceType::PASER_RESOURCE);
    }
}

void ProcessLogic::releaseAcquiredHouse()
{
    log("Releasing acquired house.");
    resource_manager.recordHouseReleased();
    resource_manager.processDeferredQueues(ResourceType::HOUSE_RESOURCE);

    if (!resource_manager.isPaserHeld())
    {
        log("House released, no paser held. Transitioning to IDLE.");
        current_state = ProcessState::IDLE;
    }
    else
    {
        log("House released. Paser still held. State remains RELEASING (for paser).");
    }
}

void ProcessLogic::releaseAcquiredPaser()
{
    log("Releasing acquired paser.");
    resource_manager.recordPaserReleased();
    resource_manager.processDeferredQueues(ResourceType::PASER_RESOURCE);
    log("Paser released. Transitioning to IDLE.");
    current_state = ProcessState::IDLE;
}