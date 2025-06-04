#include "ResourceManager.h"

ResourceManager::ResourceManager(int process_id, int n_procs, int d_houses, int p_pasers,
                                 ClockManager &clock_mgr, MessageHandler &msg_handler)
    : my_id(process_id), N_PROCESSES_CONST(n_procs), D_HOUSES_CONST(d_houses), P_PASERS_CONST(p_pasers),
      clock_manager(clock_mgr), message_handler(msg_handler),
      held_house_id_val(0), requesting_house(false), house_request_timestamp(0),
      holding_paser_flag(false), requesting_paser(false), paser_request_timestamp(0)
{
    for (int i = 1; i <= D_HOUSES_CONST; ++i)
    {
        local_house_state[i] = HOUSE_STATE_FREE;
    }
    log("ResourceManager initialized.");
}

void ResourceManager::log(const std::string &message_content) const
{
    std::cout << "[ResMgr P" << my_id << " C" << clock_manager.getTime() << "] " << message_content << std::endl;
}

#pragma region house
void ResourceManager::requestHouse()
{
    log("Initiating RequestHouse.");
    requesting_house = true;
    house_request_timestamp = clock_manager.getTime();

    house_replies_needed.clear();
    for (int i = 1; i <= N_PROCESSES_CONST; ++i)
    {
        if (i != my_id)
        {
            house_replies_needed.insert(i);
        }
    }
    log("Broadcasting REQUEST_HOUSE with ts " + std::to_string(house_request_timestamp) + ". Expecting " + std::to_string(house_replies_needed.size()) + " replies.");
    message_handler.broadcastMessage(MessageType::REQUEST_HOUSE, house_request_timestamp);
}

void ResourceManager::handleHouseRequest(const Message &msg)
{
    log("Handling HOUSE request from " + std::to_string(msg.sender_id) + " (ts:" + std::to_string(msg.timestamp) + ")");
    std::pair<int, int> my_priority = getMyPriority(ResourceType::HOUSE_RESOURCE);
    std::pair<int, int> sender_priority = {msg.timestamp, msg.sender_id};

    bool am_i_requesting_or_holding = requesting_house || (held_house_id_val != 0);
    bool sender_has_higher_priority = (sender_priority.first < my_priority.first) ||
                                      (sender_priority.first == my_priority.first && sender_priority.second < my_priority.second);

    if (!am_i_requesting_or_holding || sender_has_higher_priority)
    {
        log("Replying immediately to " + std::to_string(msg.sender_id) + " for HOUSE");
        sendReply(msg.sender_id, ResourceType::HOUSE_RESOURCE);
    }
    else
    {
        log("Deferring reply to " + std::to_string(msg.sender_id) + " for HOUSE");
        addToDeferredQueue(msg.sender_id, ResourceType::HOUSE_RESOURCE);
    }
}

void ResourceManager::handleHouseReply(const Message &msg)
{
    log("Handling HOUSE reply from " + std::to_string(msg.sender_id) + " (ts:" + std::to_string(msg.timestamp) + ")");
    if (!requesting_house || msg.timestamp < house_request_timestamp)
    {
        log("Stale/unexpected HOUSE reply from " + std::to_string(msg.sender_id) + ". My req_ts: " + std::to_string(house_request_timestamp) + ", reply_ts: " + std::to_string(msg.timestamp));
        return;
    }
    removeFromRepliesNeeded(msg.sender_id, ResourceType::HOUSE_RESOURCE);
}

void ResourceManager::updateLocalHouseState(int house_id, int status)
{
    if (house_id > 0 && house_id <= D_HOUSES_CONST)
    {
        local_house_state[house_id] = status;
        log("Updated local_house_state[" + std::to_string(house_id) + "] to " + (status == HOUSE_STATE_FREE ? "FREE" : "TAKEN_BY_" + std::to_string(status)));
    }
}

bool ResourceManager::isHouseHeld() const
{
    return held_house_id_val != 0;
}

int ResourceManager::getHeldHouseId() const
{
    return held_house_id_val;
}

bool ResourceManager::allHouseRepliesReceived() const
{
    return house_replies_needed.empty();
}

void ResourceManager::recordHouseAcquired(int house_id)
{
    held_house_id_val = house_id;
    local_house_state[house_id] = my_id;
    requesting_house = false;
    log("Recorded acquisition of house " + std::to_string(house_id));
    message_handler.broadcastMessage(MessageType::UPDATE_HOUSE_STATE, -1, house_id, my_id);
}

void ResourceManager::recordHouseReleased()
{
    if (held_house_id_val != 0)
    {
        int released_hid = held_house_id_val;
        local_house_state[released_hid] = HOUSE_STATE_FREE;
        held_house_id_val = 0;
        requesting_house = false;
        log("Recorded release of house " + std::to_string(released_hid));
        message_handler.broadcastMessage(MessageType::UPDATE_HOUSE_STATE, -1, released_hid, HOUSE_STATE_FREE);
    }
}

std::map<int, int> ResourceManager::getHouseStates() const
{
    return local_house_state;
}
#pragma endregion house

#pragma region paser
void ResourceManager::requestPaser()
{
    log("Initiating RequestPaser.");
    requesting_paser = true;
    paser_request_timestamp = clock_manager.getTime();

    paser_replies_needed.clear();
    for (int i = 1; i <= N_PROCESSES_CONST; ++i)
    {
        if (i != my_id)
        {
            paser_replies_needed.insert(i);
        }
    }
    log("Broadcasting REQUEST_PASER with ts " + std::to_string(paser_request_timestamp) + ". Expecting " + std::to_string(paser_replies_needed.size()) + " replies.");
    message_handler.broadcastMessage(MessageType::REQUEST_PASER, paser_request_timestamp);
}

void ResourceManager::handlePaserRequest(const Message &msg)
{
    log("Handling PASER request from " + std::to_string(msg.sender_id) + " (ts:" + std::to_string(msg.timestamp) + ")");
    std::pair<int, int> my_priority = getMyPriority(ResourceType::PASER_RESOURCE);
    std::pair<int, int> sender_priority = {msg.timestamp, msg.sender_id};

    bool am_i_requesting_or_holding = requesting_paser || holding_paser_flag;
    bool sender_has_higher_priority = (sender_priority.first < my_priority.first) ||
                                      (sender_priority.first == my_priority.first && sender_priority.second < my_priority.second);

    if (!am_i_requesting_or_holding || sender_has_higher_priority)
    {
        log("Replying immediately to " + std::to_string(msg.sender_id) + " for PASER");
        sendReply(msg.sender_id, ResourceType::PASER_RESOURCE);
    }
    else
    {
        log("Deferring reply to " + std::to_string(msg.sender_id) + " for PASER");
        addToDeferredQueue(msg.sender_id, ResourceType::PASER_RESOURCE);
    }
}

void ResourceManager::handlePaserReply(const Message &msg)
{
    log("Handling PASER reply from " + std::to_string(msg.sender_id) + " (ts:" + std::to_string(msg.timestamp) + ")");
    if (!requesting_paser || msg.timestamp < paser_request_timestamp)
    {
        log("Stale/unexpected PASER reply from " + std::to_string(msg.sender_id) + ". My req_ts: " + std::to_string(paser_request_timestamp) + ", reply_ts: " + std::to_string(msg.timestamp));
        return;
    }
    removeFromRepliesNeeded(msg.sender_id, ResourceType::PASER_RESOURCE);
}

bool ResourceManager::isPaserHeld() const
{
    return holding_paser_flag;
}

bool ResourceManager::sufficientPaserRepliesReceived() const
{
    if (P_PASERS_CONST <= 0)
    {
        log("Error: P_PASERS_CONST is not positive, cannot acquire paser.");
        return false;
    }
    return paser_replies_needed.size() < P_PASERS_CONST;
}

void ResourceManager::recordPaserAcquired()
{
    holding_paser_flag = true;
    requesting_paser = false;
    log("Recorded paser acquisition.");
}

void ResourceManager::recordPaserReleased()
{
    holding_paser_flag = false;
    requesting_paser = false;
    log("Recorded paser release.");
}
#pragma endregion paser

std::pair<int, int> ResourceManager::getMyPriority(ResourceType type)
{
    if (type == ResourceType::HOUSE_RESOURCE && requesting_house)
    {
        return {house_request_timestamp, my_id};
    }
    else if (type == ResourceType::PASER_RESOURCE && requesting_paser)
    {
        return {paser_request_timestamp, my_id};
    }
    return {N_PROCESSES_CONST + D_HOUSES_CONST + P_PASERS_CONST + 10000, my_id};
}

void ResourceManager::processDeferredQueues(ResourceType type)
{
    if (type == ResourceType::HOUSE_RESOURCE)
    {
        while (!house_deferred_queue.empty())
        {
            int p_id = house_deferred_queue.front();
            house_deferred_queue.pop();
            log("Sending deferred REPLY_HOUSE to " + std::to_string(p_id));
            sendReply(p_id, ResourceType::HOUSE_RESOURCE);
        }
    }
    else
    {
        while (!paser_deferred_queue.empty())
        {
            int p_id = paser_deferred_queue.front();
            paser_deferred_queue.pop();
            log("Sending deferred REPLY_PASER to " + std::to_string(p_id));
            sendReply(p_id, ResourceType::PASER_RESOURCE);
        }
    }
}

void ResourceManager::sendReply(int target_id, ResourceType resource_type)
{
    MessageType reply_type = (resource_type == ResourceType::HOUSE_RESOURCE) ? MessageType::REPLY_HOUSE : MessageType::REPLY_PASER;
    message_handler.sendMessage(target_id - 1, reply_type, clock_manager.getTime());
    log("Sent " + std::string(resource_type == ResourceType::HOUSE_RESOURCE ? "REPLY_HOUSE" : "REPLY_PASER") + " to " + std::to_string(target_id));
}

void ResourceManager::addToDeferredQueue(int sender_id, ResourceType resource_type)
{
    if (resource_type == ResourceType::HOUSE_RESOURCE)
    {
        house_deferred_queue.push(sender_id);
    }
    else
    {
        paser_deferred_queue.push(sender_id);
    }
}

void ResourceManager::removeFromRepliesNeeded(int sender_id, ResourceType resource_type)
{
    if (resource_type == ResourceType::HOUSE_RESOURCE)
    {
        house_replies_needed.erase(sender_id);
        log("Removed " + std::to_string(sender_id) + " from house_replies_needed. Remaining: " + std::to_string(house_replies_needed.size()));
    }
    else
    {
        paser_replies_needed.erase(sender_id);
        log("Removed " + std::to_string(sender_id) + " from paser_replies_needed. Remaining: " + std::to_string(paser_replies_needed.size()));
    }
}
