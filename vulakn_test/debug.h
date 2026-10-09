#pragma once

class debug
{
public:
    
#ifdef Ndebug
    static constexpr bool EnableDebug = false;
#else
    static constexpr bool EnableDebug = true;
#endif
};
