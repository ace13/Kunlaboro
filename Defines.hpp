/*  Kunlaboro - Defines

    The MIT License (MIT)
    Copyright (c) 2012 Alexander Olofsson (ace@haxalot.com)
 
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _KUNLABORO_DEFINES_HPP
#define _KUNLABORO_DEFINES_HPP

#include <boost/any.hpp>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Kunlaboro
{
    class Component;
    struct Message;

    /// A Globally Unique IDentifier.
    typedef unsigned int GUID;

    /// A ComponentId.
    typedef GUID ComponentId;
    /// A EntityId.
    typedef GUID EntityId;
    /// A RequestId.
    typedef GUID RequestId;

    /// The Payload type to use.
    typedef boost::any Payload;

    /// The MessageFunction.
    typedef std::function<void(const Message&)> MessageFunction;
    /// The FactoryFunction for a Component.
    typedef std::function<Component*()> 	    FactoryFunction;

    /// A ComponentMap to store and access Component objects against their names.
    typedef std::unordered_map<std::string, std::vector<Component*> > ComponentMap;
    
    /// A NameToIdMap to store GUID values of strings.
    typedef std::unordered_map<std::string, GUID> NameToIdMap;
    /// A IdToNameMap to store the string value that a GUID points to.
    typedef std::unordered_map<GUID, std::string> IdToNameMap;

    /// The type of a message.
    enum MessageType
    {
        Type_Create,  ///< A Component was created/added.
        Type_Destroy, ///< A Component was destroyed/removed.
        Type_Message  ///< A Message was sent.
    };

    /// The reason a message exists.
    enum MessageReason
    {
        Reason_Message,      ///< A Message was sent.
        Reason_Component,    ///< A Component did something.
        Reason_AllComponents ///< All Components were requested.
    };

    /// A Request of a Component.
    struct ComponentRequested
    {
        MessageReason reason; ///< The reason of the request.
        std::string name;     ///< The name of the Component owning request.
    };

    /// A Registered callback for a Component.
    struct ComponentRegistered
    {
        Component* component;     ///< The Component the registered a request.
        MessageFunction callback; ///< The callback in question.
        bool required;            ///< Is this a requirement and not a request.
    };

    /// A message that is sent through the EntitySystem.
    struct Message {
        MessageType type;  ///< The type of the message.
        Component* sender; ///< The sender of the message, can be NULL.
        Payload payload;   ///< The payload of the message, can be empty.
        Message(MessageType t) : type(t) {} ///< Create an empty message of the specified type.
        Message(MessageType t, Component *c) : type(t), sender(c) {} ///< Create an empty message with a specified sender.
        Message(MessageType t, Component *c, Payload p) : type(t), sender(c), payload(p) {} ///< Create a message with the specified sender and Payload.
    };
}

#endif