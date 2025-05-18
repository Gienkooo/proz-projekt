#pragma once

#include <set>
#include <queue>
#include <string>
#include <mutex>
#include <map>
#include <iostream>
#include <algorithm>

#include "types.h"
#include "ClockManager.h"
#include "MessageHandler.h"

class ResourceManager
{
public:
    ResourceManager(int process_id, int n_procs, int d_houses, int p_pasers, ClockManager &clock_mgr, MessageHandler &msg_handler);

    // House Management
    void requestHouse();
    void releaseHouse(int house_id_to_release);
    void handleHouseRequest(const Message &msg);
    void handleHouseReply(const Message &msg);
    void updateLocalHouseState(int house_id, int status);
    bool isHouseHeld() const;
    int getHeldHouseId() const;
    bool allHouseRepliesReceived() const;
    void recordHouseAcquired(int house_id);
    void recordHouseReleased();
    bool isRequestingHouse() const { return requesting_house; }
    std::map<int, int> getHouseStates() const;

    // Paser Management
    void requestPaser();
    void releasePaser();
    void handlePaserRequest(const Message &msg);
    void handlePaserReply(const Message &msg);
    bool isPaserHeld() const;
    bool sufficientPaserRepliesReceived() const;
    void recordPaserAcquired();
    void recordPaserReleased();
    bool isRequestingPaser() const { return requesting_paser; }

    std::pair<int, int> getMyPriority(ResourceType type);
    void processDeferredQueues(ResourceType type);

    std::mutex &getMutex() { return resource_mutex; }

private:
    int my_id;
    const int N_PROCESSES_CONST;
    const int D_HOUSES_CONST;
    const int P_PASERS_CONST;

    ClockManager &clock_manager;
    MessageHandler &message_handler;

    // House State
    std::map<int, int> local_house_state;
    int held_house_id_val;
    bool requesting_house;
    int house_request_timestamp;
    std::set<int> house_replies_needed;
    std::queue<int> house_deferred_queue;

    // Paser State
    bool holding_paser_flag;
    bool requesting_paser;
    int paser_request_timestamp;
    std::set<int> paser_replies_needed;
    std::queue<int> paser_deferred_queue;

    std::mutex resource_mutex;

    void log(const std::string &message_content) const;
    void sendReply(int target_id, ResourceType resource_type);
    void addToDeferredQueue(int sender_id, ResourceType resource_type);
    void removeFromRepliesNeeded(int sender_id, ResourceType resource_type);
};
