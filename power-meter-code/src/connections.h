/**
 * @file connections.h
 * @brief Classes to handle sending data to the outside world.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-06-27
 */
#pragma once

class Connection
{
public:
    virtual void enable() {}
    virtual void disable() {}
    virtual bool isConnected() {}
    virtual void sendData() {} // TODO: Data format to encode with.
};

class BLEConnection : Connection
{
    // TODO
};

class MQTTConnection : Connection
{
    // TODO
};