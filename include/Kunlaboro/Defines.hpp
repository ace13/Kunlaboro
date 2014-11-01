#pragma once

#include <cstring>
#include <cstdint>
#include <string>
#include <deque>
#include <vector>
#include <functional>
#include <typeindex>
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
    class Payload
    {
    public:
        Payload(const Payload& rhs) : mData(nullptr), mSize(rhs.mSize), mType(rhs.mType)
        {
            if (rhs.mData != nullptr)
            {
                mData = new char[mSize];
                std::memcpy(mData, rhs.mData, mSize);
            }
        }
        Payload(Payload&& rhs) : mData(std::move(rhs.mData)), mSize(std::move(rhs.mSize)), mType(std::move(rhs.mType)) { }

        template<typename T>
        Payload(const T& data) : mData(new T(data)), mSize(sizeof(T)), mType(typeid(T)) { }

        template<typename T>
        Payload(const T* data) : mData(new T(data)), mSize(sizeof(T)), mType(typeid(T)) { }

        Payload(std::nullptr_t) : mData(nullptr), mSize(0), mType(typeid(nullptr)) { }
        Payload() : mData(nullptr), mSize(0), mType(typeid(nullptr)) { }
        ~Payload() { if (mData) delete mData; }

        Payload& operator=(Payload p)
        {
            std::swap(mData, p.mData);
            std::swap(mSize, p.mSize);
            std::swap(mType, p.mType);

            return *this;
        }

        template<typename T>
        T get() const { return *static_cast<const T*>(mData); }
        template<typename T>
        bool is() const { return mType == typeid(T); }

    private:
        void* mData;
        size_t mSize;
        std::type_index mType;
    };

    /// The MessageFunction.
    typedef std::function<void(Message&)> MessageFunction;
    /// The factory function for a Component.
    typedef std::function<Component*()> ComponentFactory;

    /// A ComponentMap to store and access Component objects against their names.
    typedef std::unordered_map<std::string, std::deque<Component*> > ComponentMap;

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
        Reason_Component    ///< A Component did something.
    };

    /// A Request of a Component.
    struct ComponentRequested
    {
        MessageReason reason; ///< The reason of the request.
        std::string name;     ///< The name of the Component owning request.
        RequestId hash;       ///< The hashed name of the request.
    };

    /// A Registered callback for a Component.
    struct ComponentRegistered
    {
        Component* component;     ///< The Component the registered a request.
        MessageFunction callback; ///< The callback in question.
        bool required;            ///< Is this a requirement and not a request.
        int priority;             ///< The priority of this request.
    };

    /// A map of requests
    typedef std::unordered_map<GUID, std::deque<ComponentRegistered> > RequestMap;

    /// A message that is sent through the EntitySystem.
    struct Message {
        MessageType type;  ///< The type of the message.
        Component* sender; ///< The sender of the message, can be NULL.
        Payload payload;   ///< The payload of the message, can be empty.
        bool handled;      ///< Has the message been handled?
        Message(MessageType t = Type_Message, Component *c = nullptr, const Payload& p = nullptr) : type(t), sender(c), payload(p), handled(false) {} ///< Create an empty message of the specified type.

        /// Helper function to handle a message.
        template<typename T>
        void handle(const T& ret);
    };

    // Only supported in the November 2013 CTP at the moment
#if !_MSC_VER || _MSC_PLATFORM_TOOLSET_CTP_Nov2013
#define CONSTEXPR constexpr
#else
#define CONSTEXPR const
#endif

    /** Fowler-Null-Vo string hash functions
     *
     * Contains implementations both for 32 and 64 bit hashing.
     * And supports compile-time hashing with constexpr.
     */
    namespace hash
    {
        template<typename S>
        struct hash_internal;
        struct hash_func1;
        struct hash_func2;

        template<>
        struct hash_internal<uint64_t>
        {
            CONSTEXPR static uint64_t default_offset = 14695981039346656037ULL;
            CONSTEXPR static uint64_t prime = 1099511628211ULL;
        };
        template<>
        struct hash_internal<uint32_t>
        {
            CONSTEXPR static uint32_t default_offset = 0x811C9DC5;
            CONSTEXPR static uint32_t prime = 0x01000193;
        };

        struct hash_func1 : public hash_internal<GUID>
        {
            CONSTEXPR static inline GUID hash(const char* const string, const GUID val = default_offset)
            {
                return (string[0] == 0) ? val : hash(string + 1, (val * prime) ^ GUID(string[0]));
            }
            CONSTEXPR static inline GUID hashF(const char* const string, const size_t strlen, const GUID val = default_offset)
            {
                return (strlen == 0) ? val : hashF(string + 1, strlen - 1, (val * prime) ^ GUID(string[0]));
            }
        };

        struct hash_func2 : public hash_internal<GUID>
        {
            static inline GUID hash(const char* const string, const GUID val = default_offset)
            {
                return (string[0] == 0) ? val : hash(string + 1, (val * prime) ^ GUID(string[0]));
            }
            static inline GUID hashF(const char* const string, const size_t strlen, const GUID val = default_offset)
            {
                return (strlen == 0) ? val : hashF(string + 1, strlen - 1, (val * prime) ^ GUID(string[0]));
            }
        };

        inline GUID hashString(const std::string& str) { return hash_func2::hashF(str.c_str(), str.size()); }
    }

#if !defined(_MSC_VER)
    inline CONSTEXPR uint32_t operator "" _rid(const char* str, const size_t strLen)
    {
        return hash_func1::hashF(str, strLen);
    }
#endif

#undef CONSTEXPR
}

/// Only here for backwards compatibility
namespace boost
{
    template<typename T>
    inline T any_cast(const Kunlaboro::Payload& p) { return p.get<T>(); }
}

template<typename T>
void Kunlaboro::Message::handle(const T& ret)
{
    payload = ret;
    handled = true;
}
