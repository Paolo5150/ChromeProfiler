# ChromeProfiler
Simple (and extendable) C++ profiler for chrome::tracing

# What is this?

Did you know that Google chrome has a tool that allows you to visualize profiling data?

Check: chrome://tracing/

# So?

This simple profiler for C++ allows you to quickly have some basic profiling data. 
You'll be able to see your function calls in a timeline, their execution time, process and thread IDs and you can add any argument you may like to profiling calls.
It outputs a json and all you have to do is drop it in the tracing tool, using the link above.

It's very basic, but can be extended.

Documentation on the json format is here: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/edit

# Usage

All you have to use are on liners macro calls.

```
PROFILE_BEGIN(name) Starts a profile session. Each session generates a json output file. Only 1 session can run at any time
PROFILE_END() Ends the session, closes out stream
PROFILE_FUNC(...) Profile this function. Should be added as the very first call in a method. Arguments can be passed in as pairs key:string, value:primitive
PROFILE_SCOPE(name,...) Profile this scope, using given name. Should be added as the very first call in a scope section. Accepts key,value args.

PROFILE_CUSTOM_ASYNC_START(name, ...) Starts custom async profile with given name. Must be followed by PROFILE_CUSTOM_ASYNC_END(name), using the same name. Accepts key,value args.
PROFILE_CUSTOM_ASYNC_START(name, ...) Ends previously started custom async profile of given name. Accepts key,value args.

PROFILE_CUSTOM_START(name,...) Starts custom profile event. If nested with a PROFILE_FUNC or PROFILE_SCOPE, it must end before the parent profile event (nested correctly). Accepts key,value args.
PROFILE_CUSTOM_END(name,...) Ends a previously started custom profile event of same name. Accepts key,value args.

PROFILE_INSTANT(name,...) Creates an instant event of given name. Accepts key,value args.
```
The example in main.cpp generates the following
![image](https://user-images.githubusercontent.com/8026573/160274363-2e48c3be-a74e-48a3-a00f-e4f17e4fc49b.png)


