#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Message
{
    size_t address_len;
    size_t data_len;
    char *address;
    char *data;
} Message;

Message *new_message(char *address, char *data)
{
    Message *msg = malloc(sizeof(Message));

    msg->address_len = strlen(address);
    msg->data_len = strlen(data);
    msg->address = address;
    msg->data = data;
    
    return msg;
}

void *serialize_message(Message *msg)
{
    void *data = malloc(msg->address_len + msg->data_len + 2 * sizeof(size_t));

    memcpy(data, &msg->address_len, sizeof(size_t));
    memcpy(data + sizeof(size_t), &msg->data_len, sizeof(size_t));
    memcpy(data + 2 * sizeof(size_t), msg->address, msg->address_len);
    memcpy(data + 2 * sizeof(size_t) + msg->address_len, msg->data, msg->data_len);

    return data;
}

Message *deserialize_message(void *serialized_msg)
{
    size_t *address_len = serialized_msg;
    size_t *data_len = serialized_msg + sizeof(size_t);

    char *address = malloc(*address_len);
    char *data = malloc(*data_len);

    memcpy(address, serialized_msg + sizeof(size_t) * 2, *address_len);
    memcpy(data, serialized_msg + sizeof(size_t) * 2 + *address_len, *data_len);

    Message *msg = malloc(sizeof(Message));
    msg->address_len = *address_len;
    msg->data_len = *data_len;
    msg->address = address;
    msg->data = data;

    return msg;
}