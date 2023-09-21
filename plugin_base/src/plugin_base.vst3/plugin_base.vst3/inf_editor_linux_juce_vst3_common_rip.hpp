#if JUCE_LINUX || JUCE_BSD

namespace juce {

#define JUCE_DECLARE_VST3_COM_REF_METHODS \
    Steinberg::uint32 PLUGIN_API addRef() override   { return (Steinberg::uint32) ++refCount; } \
    Steinberg::uint32 PLUGIN_API release() override  { const int r = --refCount; if (r == 0) delete this; return (Steinberg::uint32) r; }

#define JUCE_DECLARE_VST3_COM_QUERY_METHODS \
    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID, void** obj) override \
    { \
        jassertfalse; \
        *obj = nullptr; \
        return Steinberg::kNotImplemented; \
    }

inline bool doUIDsMatch (const Steinberg::TUID a, const Steinberg::TUID b) noexcept
{
    return std::memcmp (a, b, sizeof (Steinberg::TUID)) == 0;
}

/*  Holds a tresult and a pointer to an object.

    Useful for holding intermediate results of calls to queryInterface.
*/
class QueryInterfaceResult
{
public:
    QueryInterfaceResult() = default;

    QueryInterfaceResult (Steinberg::tresult resultIn, void* ptrIn)
        : result (resultIn), ptr (ptrIn) {}

    bool isOk() const noexcept   { return result == Steinberg::kResultOk; }

    Steinberg::tresult extract (void** obj) const
    {
        *obj = result == Steinberg::kResultOk ? ptr : nullptr;
        return result;
    }

private:
    Steinberg::tresult result = Steinberg::kResultFalse;
    void* ptr = nullptr;
};

/*  Holds a tresult and a pointer to an object.

    Calling InterfaceResultWithDeferredAddRef::extract() will also call addRef
    on the pointed-to object. It is expected that users will use
    InterfaceResultWithDeferredAddRef to hold intermediate results of a queryInterface
    call. When a suitable interface is found, the function can be exited with
    `return suitableInterface.extract (obj)`, which will set the obj pointer,
    add a reference to the interface, and return the appropriate result code.
*/
class InterfaceResultWithDeferredAddRef
{
public:
    InterfaceResultWithDeferredAddRef() = default;

    template <typename Ptr>
    InterfaceResultWithDeferredAddRef (Steinberg::tresult resultIn, Ptr* ptrIn)
        : result (resultIn, ptrIn),
          addRefFn (doAddRef<Ptr>) {}

    bool isOk() const noexcept   { return result.isOk(); }

    Steinberg::tresult extract (void** obj) const
    {
        const auto toReturn = result.extract (obj);

        if (result.isOk() && addRefFn != nullptr && *obj != nullptr)
            addRefFn (*obj);

        return toReturn;
    }

private:
    template <typename Ptr>
    static void doAddRef (void* obj)   { static_cast<Ptr*> (obj)->addRef(); }

    QueryInterfaceResult result;
    void (*addRefFn) (void*) = nullptr;
};

template <typename ClassType>                                   struct UniqueBase {};
template <typename CommonClassType, typename SourceClassType>   struct SharedBase {};

template <typename ToTest, typename CommonClassType, typename SourceClassType>
InterfaceResultWithDeferredAddRef testFor (ToTest& toTest,
                                           const Steinberg::TUID targetIID,
                                           SharedBase<CommonClassType, SourceClassType>)
{
    if (! doUIDsMatch (targetIID, CommonClassType::iid))
        return {};

    return { Steinberg::kResultOk, static_cast<CommonClassType*> (static_cast<SourceClassType*> (std::addressof (toTest))) };
}

template <typename ToTest, typename ClassType>
InterfaceResultWithDeferredAddRef testFor (ToTest& toTest,
                                           const Steinberg::TUID targetIID,
                                           UniqueBase<ClassType>)
{
    return testFor (toTest, targetIID, SharedBase<ClassType, ClassType>{});
}

template <typename ToTest>
InterfaceResultWithDeferredAddRef testForMultiple (ToTest&, const Steinberg::TUID) { return {}; }

template <typename ToTest, typename Head, typename... Tail>
InterfaceResultWithDeferredAddRef testForMultiple (ToTest& toTest, const Steinberg::TUID targetIID, Head head, Tail... tail)
{
    const auto result = testFor (toTest, targetIID, head);

    if (result.isOk())
        return result;

    return testForMultiple (toTest, targetIID, tail...);
}

}
#endif