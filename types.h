#pragma once

const int N_PROCESSES_DEFAULT = 5;
const int D_HOUSES_DEFAULT = 3;
const int P_PASERS_DEFAULT = 2;

enum class ProcessState
{
    IDLE,
    WANT_HOUSE,
    HAVE_HOUSE_WANT_PASER,
    HAVE_BOTH,
    RELEASING
};

enum class ResourceType
{
    HOUSE_RESOURCE,
    PASER_RESOURCE
};

enum class MessageType
{
    REQUEST_HOUSE,
    REPLY_HOUSE,
    REQUEST_PASER,
    REPLY_PASER,
    UPDATE_HOUSE_STATE
};

const int HOUSE_STATE_FREE = 0;

struct Message
{
    MessageType type;
    int sender_id;
    int timestamp;
    int house_id;
    int new_house_status;
    // TODO Może warto użyć unii do wiadomości, chyba, że są jakieś lepsze metody typu msg type.
};
