![License MIT](https://img.shields.io/github/license/BehaviorTree/BehaviorTree.CPP?color=blue)
[![conan Ubuntu](https://github.com/endurodave/StateMachine/actions/workflows/cmake_ubuntu.yml/badge.svg)](https://github.com/endurodave/StateMachine/actions/workflows/cmake_ubuntu.yml)
[![conan Ubuntu](https://github.com/endurodave/StateMachine/actions/workflows/cmake_clang.yml/badge.svg)](https://github.com/endurodave/StateMachine/actions/workflows/cmake_clang.yml)
[![conan Windows](https://github.com/endurodave/StateMachine/actions/workflows/cmake_windows.yml/badge.svg)](https://github.com/endurodave/StateMachine/actions/workflows/cmake_windows.yml)

# State Machine Design in C++

A compact, delegate-based C++ finite state machine (FSM) with built-in asynchronous active-object support. Runs on embedded and PC targets. State functions are plain member functions registered at construction — no macros, no template metaprogramming. Guards, entry/exit actions, state machine inheritance, and pub/sub signals (`OnTransition`, `OnEntry`, `OnExit`) are all supported. Promote any SM to a thread-safe active object with a single `SetThread()` call; `shared_ptr` event data keeps derived types intact across the thread boundary without slicing.

# Table of Contents

- [State Machine Design in C++](#state-machine-design-in-c)
- [Table of Contents](#table-of-contents)
- [Preface](#preface)
  - [Related repositories](#related-repositories)
- [Getting Started](#getting-started)
- [Introduction](#introduction)
  - [Background](#background)
  - [Why use a state machine?](#why-use-a-state-machine)
- [State machine design](#state-machine-design)
  - [Internal and external events](#internal-and-external-events)
  - [Event data](#event-data)
  - [State transitions](#state-transitions)
- [StateMachine class](#statemachine-class)
- [Motor example](#motor-example)
  - [State functions](#state-functions)
  - [State map](#state-map)
  - [Transition map](#transition-map)
- [State engine](#state-engine)
- [Generating events](#generating-events)
- [Signals](#signals)
- [Asynchronous active-object mode](#asynchronous-active-object-mode)
- [Validate](#validate)
- [State machine inheritance](#state-machine-inheritance)
  - [Base class external event functions](#base-class-external-event-functions)
- [State function inheritance](#state-function-inheritance)
- [Multithread safety](#multithread-safety)
- [Fixed-block allocator](#fixed-block-allocator)
- [Comparison with other libraries](#comparison-with-other-libraries)
    - [Boost.MSM](#boostmsm)
    - [Boost.Statechart](#booststatechart)
    - [SML (kgrzybek)](#sml-kgrzybek)
    - [TinyFSM](#tinyfsm)
    - [QP/C++](#qpc)
    - [Summary](#summary)


# Preface

Originally published on CodeProject at: [**State Machine Design in C++**](https://www.codeproject.com/Articles/1087619/State-Machine-Design-in-Cplusplus)

Based on original design published in C\C++ Users Journal (Dr. Dobb's) at: [**State Machine Design in C++**](http://www.drdobbs.com/cpp/state-machine-design-in-c/184401236)

## Related repositories

The following repositories offer various implementations of finite state machines, ranging from compact single-threaded versions to advanced asynchronous frameworks utilizing the Signal-Slot pattern.

| Project | Description |
| :--- | :--- |
| [**State Machine Design in C**](https://github.com/endurodave/C_StateMachine) | A compact C language finite state machine (FSM) implementation. |
| [**DelegateMQ**](https://github.com/endurodave/DelegateMQ) | The asynchronous delegate library used by this project for active-object dispatch, type-safe multicast delegates, and pub/sub signals. |


# Getting Started

[CMake](https://cmake.org/) is used to create the project build files on any Windows or Linux machine. The state machine source code works on any C++ compiler on any platform.

```bash
# Build
cmake -B build .

# Run
./build/StateMachineApp          # Linux
build\Debug\StateMachineApp.exe  # Windows
```

`StateMachineApp` runs all example state machines and outputs results to stdout. There is no separate test runner.

# Introduction

In 2000, I wrote an article entitled "*State Machine Design in C++*" for C/C++ Users Journal (R.I.P.). Interestingly, that old article is still available and (at the time of writing this article) the #1 hit on Google when searching for C++ state machine. The article was written over 25 years ago, but I continue to use the basic idea on numerous projects. It's compact, easy to understand and, in most cases, has just enough features to accomplish what I need.

This implementation updates the design with modern C++ delegate-based state registration, an optional asynchronous active-object mode, and publisher/subscriber signals — while retaining the compact table-driven core that makes the original design so practical for embedded and PC targets alike.

This state machine has the following features:

1. **Transition tables** – transition tables precisely control state transition behavior.
2. **Events** – every event is a simple public instance member function with any argument types.
3. **State action** – every state action is a separate member function registered as a delegate, with a typed event data argument.
4. **Guards/entry/exit actions** – optionally a state machine can use guard conditions and separate entry/exit action functions for each state, all registered as delegates.
5. **Signals** – `OnTransition`, `OnEntry`, `OnExit`, and `OnCannotHappen` publisher/subscriber signals allow external observers to react to state changes without modifying the SM.
6. **Async active-object mode** – calling `SetThread()` enables active-object dispatch: `ExternalEvent()` marshals to the SM thread and returns immediately. All state logic executes on that thread with no mutex needed.
7. **State machine inheritance** – supports inheriting states from a base state machine class.
8. **State function inheritance** – supports overriding a state function within a derived class.
9. **Type safe** – compile-time checks via delegate adapters catch signature mismatches early. Runtime checks for the other cases.
10. **Thread-safe** – structural thread safety via active-object dispatch; no explicit locks needed inside the state machine.
11. **shared_ptr event data** – event data is passed as `std::shared_ptr<const EventData>`, making ownership unambiguous and keeping derived data intact across async thread boundaries.

## Background

A common design technique in the repertoire of most programmers is the venerable finite state machine (FSM). Designers use this programming construct to break complex problems into manageable states and state transitions. There are innumerable ways to implement a state machine.

A switch statement provides one of the easiest to implement and most common version of a state machine. Here, each case within the switch statement becomes a state, implemented something like:

```cpp
switch (currentState) {
   case ST_IDLE:
       // do something in the idle state
       break;
    case ST_STOP:
       // do something in the stop state
       break;
    // etc...
}
```

This method is certainly appropriate for solving many different design problems. When employed on an event driven, multithreaded project, however, state machines of this form can be quite limiting.

The first problem revolves around controlling what state transitions are valid and which ones are invalid. There is no way to enforce the state transition rules. Any transition is allowed at any time, which is not particularly desirable. For most designs, only a few transition patterns are valid. Ideally, the software design should enforce these predefined state sequences and prevent the unwanted transitions. Another problem arises when trying to send data to a specific state. Since the entire state machine is located within a single function, sending additional data to any given state proves difficult. And lastly these designs are rarely suitable for use in a multithreaded system. The designer must ensure the state machine is called from a single thread of control.

## Why use a state machine?

Implementing code using a state machine is an extremely handy design technique for solving complex engineering problems. State machines break down the design into a series of steps, or what are called states in state-machine lingo. Each state performs some narrowly defined task. Events, on the other hand, are the stimuli, which cause the state machine to move, or transition, between states.

To take a simple example, which I will use throughout this article, let's say we are designing motor-control software. We want to start and stop the motor, as well as change the motor's speed. Simple enough. The motor control events to be exposed to the client software will be as follows:

1. **Set Speed** – sets the motor going at a specific speed.
2. **Halt** – stops the motor.

These events provide the ability to start the motor at whatever speed desired, which also implies changing the speed of an already moving motor. Or we can stop the motor altogether. To the motor-control class, these two events, or functions, are considered external events. To a client using our code, however, these are just plain functions within a class.

These events are not state machine states. The steps required to handle these two events are different. In this case the states are:

1. **Idle** — the motor is not spinning but is at rest.

   - Do nothing.

2. **Start** — starts the motor from a dead stop.

   - Turn on motor power.
   - Set motor speed.

3. **Change Speed** — adjust the speed of an already moving motor.

   - Change motor speed.

4. **Stop** — stop a moving motor.

   - Turn off motor power.
   - Go to the Idle state.

As can be seen, breaking the motor control into discreet states, as opposed to having one monolithic function, we can more easily manage the rules of how to operate the motor.

Every state machine has the concept of a "current state." This is the state the state machine currently occupies. At any given moment in time, the state machine can be in only a single state. Every instance of a particular state machine class can set the initial state during construction. That initial state, however, does not execute during object creation. Only an event sent to the state machine causes a state function to execute.

To graphically illustrate the states and events, we use a state diagram. Figure 1 below shows the state transitions for the motor control class. A box denotes a state and a connecting arrow indicates the event transitions. Arrows with the event name listed are external events, whereas unadorned lines are considered internal events. (I cover the differences between internal and external events later in the article.)

To graphically illustrate the states and events, we use a state diagram. Figure 1 below shows the state transitions for the motor control class. A box denotes a state and a connecting arrow indicates the event transitions. Arrows with the event name listed are external events, whereas unadorned lines are considered internal events. (I cover the differences between internal and external events later in the article.)

![](Motor.png)

**Figure 1: Motor state diagram**

As you can see, when an event comes in the state transition that occurs depends on state machine's current state. When a `SetSpeed` event comes in, for instance, and the motor is in the Idle state, it transitions to the `Start` state. However, that same `SetSpeed` event generated while the current state is Start transitions the motor to the `ChangeSpeed` state. You can also see that not all state transitions are valid. For instance, the motor can't transition from `ChangeSpeed` to Idle without first going through the Stop state.

In short, using a state machine captures and enforces complex interactions, which might otherwise be difficult to convey and implement.

# State machine design

## Internal and external events

As I mentioned earlier, an event is the stimulus that causes a state machine to transition between states. For instance, a button press could be an event. Events can be broken out into two categories: external and internal. The external event, at its most basic level, is a function call into a state-machine object. These functions are public and are called from the outside or from code external to the state-machine object. Any thread or task within a system can generate an external event. In synchronous mode (the default), the external event function call causes state execution on the caller's thread of control. In asynchronous active-object mode (see below), `ExternalEvent()` marshals the call to the designated SM thread and returns immediately. An internal event, on the other hand, is self-generated by the state machine itself during state execution and always runs synchronously on the SM thread.

A typical scenario consists of an external event being generated, which boils down to a function call into the class's public interface. Based upon the event being generated and the state machine's current state, a lookup is performed to determine if a transition is required. If so, the state machine transitions to the new state and the code for that state executes. At the end of the state function, a check is performed to determine whether an internal event was generated. If so, another transition is performed and the new state gets a chance to execute. This process continues until the state machine is no longer generating internal events, at which time the original external event function call returns. The external event and all internal events, if any, execute within the same thread of control.

Once the external event starts the state machine executing, it cannot be interrupted by another external event until the external event and all internal events have completed execution. In active-object mode this is guaranteed structurally — all events are serialized through the SM thread's message queue with no mutex required.

## Event data

When an event is generated, it can optionally attach event data to be used by the state function during execution. Event data is passed as a `std::shared_ptr<const EventData>`. Using a shared pointer rather than a raw pointer provides two important guarantees:

1. **Ownership is unambiguous** — no heap-allocation contract to document or forget. The framework holds the last reference; the data is automatically released when the transition completes.
2. **Async safety** — in active-object mode, DelegateMQ copies the shared pointer by value across the thread boundary, keeping the derived type alive on the queue without object slicing. A raw pointer would be deep-copied as `EventData` by the async marshaling layer, silently losing derived-class fields such as `MotorData::speed`.

All event data structures must inherit from the `EventData` base class:

```cpp
class EventData {
public:
    virtual ~EventData() = default;
};
using NoEventData = EventData;
```

States that carry no data use `NoEventData`. States that carry typed data define a derived struct:

```cpp
class MotorData : public EventData {
public:
    int speed;
};
```

## State transitions

When an external event is generated, a lookup is performed to determine the state transition course of action. There are three possible outcomes to an event: new state, event ignored, or cannot happen. A new state causes a transition to a new state where it is allowed to execute. Transitions to the existing state are also possible, which means the current state is re-executed. For an ignored event, no state executes. The last possibility, cannot happen, is reserved for situations where the event is not valid given the current state of the state machine. If this occurs, the `OnCannotHappen` signal fires and the software faults.

Internal events are not required to perform a validating transition lookup. The state transition is assumed to be valid. You could check for both valid internal and external event transitions, but in practice, this just takes more storage space and generates busywork for very little benefit. The real need for validating transitions lies in the asynchronous, external events where a client can cause an event to occur at an inappropriate time.

# StateMachine class

Two base classes are necessary when creating your own state machine: `StateMachine` and `EventData`. A class inherits from `StateMachine` to obtain the necessary mechanisms to support state transitions and event handling. To send unique data to state functions, the structure must inherit from the `EventData` base class.

The key classes are declared as follows:

```cpp
/// @brief One row in the state map. All fields are optional delegates.
struct StateMapRow {
    // State action — fires on every visit, including self-transitions.
    dmq::MulticastDelegate<void(std::shared_ptr<const EventData>)> action;
    // Optional guard — when bound and returning false the transition is vetoed.
    dmq::UnicastDelegate<bool(std::shared_ptr<const EventData>)>   guard;
    // Optional entry action — called only when the state actually changes.
    dmq::MulticastDelegate<void(std::shared_ptr<const EventData>)> entry;
    // Optional exit action — called only when the state actually changes.
    dmq::MulticastDelegate<void()>                                  exit;
};

class StateMachine {
public:
    enum : uint8_t { EVENT_IGNORED = 0xFE, CANNOT_HAPPEN = 0xFF };

    StateMachine(uint8_t maxStates, uint8_t initialState = 0);
    virtual ~StateMachine() = default;

    uint8_t GetCurrentState() const;
    uint8_t GetMaxStates()    const;

    // Enable active-object async mode. Call before the first ExternalEvent.
    void SetThread(dmq::IThread& thread);

    // Validate that every state has at least one action registered.
    bool Validate(std::function<void(uint8_t)> onUnregistered = nullptr) const;

    // Signals — fire on the SM thread in async mode.
    dmq::Signal<void(uint8_t fromState, uint8_t toState)> OnTransition;
    dmq::Signal<void(uint8_t state)>                      OnEntry;
    dmq::Signal<void(uint8_t state)>                      OnExit;
    dmq::Signal<void(uint8_t currentState)>               OnCannotHappen;

protected:
    void ExternalEvent(uint8_t newState,
                       std::shared_ptr<const EventData> pData = nullptr);
    void InternalEvent(uint8_t newState,
                       std::shared_ptr<const EventData> pData = nullptr);

    // State map populated by the derived class constructor.
    std::vector<StateMapRow> m_stateMap;
};
```

The state map is an `std::vector<StateMapRow>` allocated with `maxStates` rows at construction time. The derived class constructor populates each row by binding member functions as delegates using three typed adapter helpers:

```cpp
// Bind a state action or entry action (receives typed event data).
MakeStateDelegate(this, &MyClass::ST_SomeState)

// Bind a guard condition (returns bool, receives typed event data).
MakeGuardDelegate(this, &MyClass::GD_SomeGuard)

// Bind an exit action (no event data argument).
MakeExitDelegate(this, &MyClass::EX_SomeExit)
```

These adapters enforce the correct member function signature at compile time and handle the `static_cast` from `shared_ptr<const EventData>` to the derived data type internally. No manual casting is needed in the state functions themselves.

# Motor example

`Motor` is an example of how to use `StateMachine`. It implements a hypothetical motor-control state machine where clients can start the motor at a specific speed and stop it. The `SetSpeed()` and `Halt()` public functions are the external events.

The `Motor` class declaration:

```cpp
class MotorData : public EventData {
public:
    int speed;
};

class Motor : public StateMachine {
public:
    Motor();

    void SetSpeed(std::shared_ptr<MotorData> data);
    void Halt();

private:
    int m_currentSpeed;

    enum States {
        ST_IDLE,
        ST_STOP,
        ST_START,
        ST_CHANGE_SPEED,
        ST_MAX_STATES
    };

    void ST_Idle(const NoEventData* data);
    void ST_Stop(const NoEventData* data);
    void ST_Start(const MotorData* data);
    void ST_ChangeSpeed(const MotorData* data);
};
```

When the `Motor` object is created, its initial state is Idle. The first call to `SetSpeed()` transitions the state machine to the Start state, where the motor is initially set into motion. Subsequent `SetSpeed()` events transition to the ChangeSpeed state, where the speed of an already moving motor is adjusted. The `Halt()` event transitions to Stop, where, during state execution, an internal event is generated to transition back to the Idle state.

Creating a new state machine requires a few basic high-level steps:

1. Inherit from the `StateMachine` base class.
2. Create a `States` enumeration with one entry per state.
3. Declare and define state functions (and optionally guard/entry/exit functions).
4. In the constructor, register each function into `m_stateMap` using the delegate adapters.
5. Create one transition map lookup table per external event using the `TRANSITION_MAP` macros.

## State functions

State functions implement each state — one state function per state-machine state. All state functions follow this signature:

```cpp
void <class>::ST_<Name>(const <DataType>* data)
```

The function returns void and takes a pointer to a typed event data argument. The convention is to prefix state functions with `ST_`, guard functions with `GD_`, entry functions with `EN_`, and exit functions with `EX_`. Unlike earlier versions of this design, no macros are needed to declare or define these functions — they are ordinary member functions:

```cpp
void Motor::ST_Start(const MotorData* data)
{
    cout << "Motor::ST_Start : Speed is " << data->speed << endl;
    m_currentSpeed = data->speed;
}

void Motor::ST_Stop(const NoEventData* data)
{
    cout << "Motor::ST_Stop" << endl;
    m_currentSpeed = 0;
    InternalEvent(ST_IDLE);  // chain to Idle without returning to caller
}
```

Note that any state function may call `InternalEvent()` to chain to another state. Guard, entry, and exit functions must not call `InternalEvent()`; a runtime assertion will fire if they do.

## State map

The state map is populated in the derived class constructor by registering delegates into `m_stateMap`. The `action` field is a `MulticastDelegate`, so multiple observers (e.g. loggers or test hooks) can attach without modifying the state machine logic.

```cpp
Motor::Motor() :
    StateMachine(ST_MAX_STATES),
    m_currentSpeed(0)
{
    m_stateMap[ST_IDLE].action         += MakeStateDelegate(this, &Motor::ST_Idle);
    m_stateMap[ST_STOP].action         += MakeStateDelegate(this, &Motor::ST_Stop);
    m_stateMap[ST_START].action        += MakeStateDelegate(this, &Motor::ST_Start);
    m_stateMap[ST_CHANGE_SPEED].action += MakeStateDelegate(this, &Motor::ST_ChangeSpeed);
}
```

The state enumeration order must match the index positions used when accessing `m_stateMap`. `ST_MAX_STATES` is passed to the base class constructor so it knows how large to size the map. The `static_assert` inside `END_TRANSITION_MAP` enforces that each transition map has exactly `ST_MAX_STATES` entries at compile time.

For states that require guard, entry, or exit behavior, register those delegates in addition to the action:

```cpp
// Guard — vetoes the transition if it returns false
m_stateMap[ST_START_TEST].guard  = MakeGuardDelegate(this, &CentrifugeTest::GD_GuardStartTest);

// Entry — called once on state entry, before the action
m_stateMap[ST_IDLE].entry       += MakeStateDelegate(this, &SelfTest::EN_EntryIdle);

// Exit — called once on state exit, before the new state's entry
m_stateMap[ST_WAIT_FOR_ACCELERATION].exit += MakeExitDelegate(this,
    &CentrifugeTest::EX_ExitWaitForAcceleration);
```

The `entry` and `exit` fields are `MulticastDelegate`s, so multiple hooks can be chained if needed. The `guard` field is a `UnicastDelegate` since only one guard function is meaningful per state.

## Transition map

The last detail to attend to is the state transition rules. A transition map is a lookup table that maps the current state to a destination state. Every external event function defines one transition map using three macros:

```cpp
BEGIN_TRANSITION_MAP
TRANSITION_MAP_ENTRY
END_TRANSITION_MAP
```

The `Halt()` event function in `Motor` defines its transition map as:

```cpp
void Motor::Halt()
{
    BEGIN_TRANSITION_MAP                            // - Current State -
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)         // ST_IDLE
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)         // ST_STOP
        TRANSITION_MAP_ENTRY(ST_STOP)               // ST_START
        TRANSITION_MAP_ENTRY(ST_STOP)               // ST_CHANGE_SPEED
    END_TRANSITION_MAP(nullptr)
}
```

The macro-expanded code for `Halt()` is:

```cpp
void Motor::Halt()
{
    static const uint8_t TRANSITIONS[] = {
        EVENT_IGNORED,      // ST_IDLE
        CANNOT_HAPPEN,      // ST_STOP
        ST_STOP,            // ST_START
        ST_STOP,            // ST_CHANGE_SPEED
    };
    static_assert((sizeof(TRANSITIONS) / sizeof(uint8_t)) == ST_MAX_STATES,
        "Transition map size does not match ST_MAX_STATES");
    ExternalEvent(TRANSITIONS[GetCurrentState()], nullptr);
}
```

`BEGIN_TRANSITION_MAP` starts the map. Each `TRANSITION_MAP_ENTRY` indicates what the state machine should do based upon the current state. The number of entries must match `ST_MAX_STATES` exactly — the `static_assert` in `END_TRANSITION_MAP` enforces this at compile time. `END_TRANSITION_MAP` takes the event data as its argument; `Halt()` has no data so it passes `nullptr`.

`EVENT_IGNORED` means "if this event fires in this state, silently do nothing." `CANNOT_HAPPEN` means "this event must never fire in this state; fire `OnCannotHappen` and fault."

# State engine

The state engine executes state functions based upon events generated. `StateEngine()` runs the following sequence in a loop until no more internal events are pending:

1. Look up the `StateMapRow` for the new state.
2. If a guard condition is bound, evaluate it. If it returns `false`, skip this iteration (transition vetoed).
3. If transitioning to a different state, call the current state's exit delegate, fire `OnExit`, update `m_currentState`, call the new state's entry delegate, and fire `OnEntry`. Entry and exit actions must not generate internal events.
4. Call the state action delegate.
5. Fire `OnTransition(fromState, toState)`. If `fromState == toState`, it was a self-transition.
6. Check whether the action generated an internal event. If so, repeat from step 1.

```cpp
void StateMachine::StateEngine()
{
    while (m_eventGenerated) {
        StateMapRow& row   = m_stateMap[m_newState];
        auto         pData = m_pEventData;
        m_pEventData       = nullptr;
        m_eventGenerated   = false;

        if (row.guard && !row.guard(pData))
            continue;

        const uint8_t fromState = m_currentState;
        const bool    changing  = (m_newState != m_currentState);

        if (changing) {
            m_stateMap[m_currentState].exit();
            OnExit(m_currentState);
            m_currentState = m_newState;
            row.entry(pData);
            OnEntry(m_currentState);
        } else {
            m_currentState = m_newState;
        }

        row.action(pData);
        OnTransition(fromState, m_currentState);
    }
}
```

# Generating events

External events are generated by creating event data on the heap via `std::make_shared`, populating its fields, and calling the external event function. The `shared_ptr` keeps the data alive until the SM is done with it — including across async thread boundaries.

```cpp
auto data = std::make_shared<MotorData>();
data->speed = 100;
motor.SetSpeed(data);
```

To generate an internal event from within a state function, call `InternalEvent()`. If the destination state accepts no data, only the state enum is needed:

```cpp
InternalEvent(ST_IDLE);
```

If event data must be passed to the destination state:

```cpp
auto data = std::make_shared<MotorData>();
data->speed = 100;
InternalEvent(ST_CHANGE_SPEED, data);
```

The class can also hide data creation from the caller entirely by allocating the `shared_ptr` inside the event method:

```cpp
void Motor::SetSpeed(int speed)
{
    auto data = std::make_shared<MotorData>();
    data->speed = speed;

    BEGIN_TRANSITION_MAP                            // - Current State -
        TRANSITION_MAP_ENTRY(ST_START)              // ST_IDLE
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)         // ST_STOP
        TRANSITION_MAP_ENTRY(ST_CHANGE_SPEED)       // ST_START
        TRANSITION_MAP_ENTRY(ST_CHANGE_SPEED)       // ST_CHANGE_SPEED
    END_TRANSITION_MAP(data)
}
```

# Signals

`StateMachine` exposes four publisher/subscriber signals that allow external code to observe state machine behavior without subclassing or modifying the SM:

- **OnTransition(fromState, toState)** — fires after every completed state action. If `fromState == toState` the transition was a self-transition.
- **OnEntry(state)** — fires when entering a new state (only on actual state changes, before the action).
- **OnExit(state)** — fires when exiting a state (only on actual state changes, before entry).
- **OnCannotHappen(state)** — fires before the fault handler when a `CANNOT_HAPPEN` transition is taken. Use this to flush logs or record diagnostics before the process aborts.

In async active-object mode, all signals fire on the SM thread — not the caller's thread. This means signal subscribers execute within the same thread context as the state logic and require no extra synchronization.

Connecting to a signal returns a connection token. Dropping the token disconnects the subscriber.

```cpp
Motor motor;

// Log every completed transition
auto transConn = motor.OnTransition.Connect(
    MakeDelegate(std::function<void(uint8_t, uint8_t)>(
        [](uint8_t from, uint8_t to) {
            cout << "  [transition " << (int)from << " -> " << (int)to << "]" << endl;
        })));

// Log actual state changes
auto entryConn = motor.OnEntry.Connect(
    MakeDelegate(std::function<void(uint8_t)>(
        [](uint8_t state) {
            cout << "  [entry " << (int)state << "]" << endl;
        })));

auto exitConn = motor.OnExit.Connect(
    MakeDelegate(std::function<void(uint8_t)>(
        [](uint8_t state) {
            cout << "  [exit " << (int)state << "]" << endl;
        })));

// Pre-fault hook for diagnostics
auto faultConn = motor.OnCannotHappen.Connect(
    MakeDelegate(std::function<void(uint8_t)>(
        [](uint8_t state) {
            cerr << "  [CANNOT_HAPPEN from state " << (int)state << "]" << endl;
        })));
```

Because `action` and `entry` in `StateMapRow` are `MulticastDelegate`s, additional hooks can also be attached directly to individual states at runtime without modifying the state machine logic:

```cpp
motor.m_stateMap[Motor::ST_START].action +=
    MakeDelegate(logger, &Logger::OnStateAction);
```

# Asynchronous active-object mode

By default, `ExternalEvent()` runs synchronously on the caller's thread — state logic, guards, entry/exit actions, and signals all complete before `ExternalEvent()` returns. This is suitable for single-threaded use or when the caller is already on the correct thread.

For multithreaded applications the state machine can be promoted to an **active object** by calling `SetThread()` with a dedicated `IThread` before the first event. After that, every `ExternalEvent()` call marshals the event (state enum + `shared_ptr` to event data) to the SM thread's message queue and returns immediately. All state logic, guard evaluation, entry/exit actions, and signal callbacks execute on the SM thread, not the caller's thread.

```cpp
// Create and start the SM thread
Thread smThread("MotorSMThread");
smThread.CreateThread();

Motor asyncMotor;
asyncMotor.SetThread(smThread);   // enable active-object mode

// Subscribers fire on smThread, not the main thread
auto conn = asyncMotor.OnTransition.Connect(
    MakeDelegate(std::function<void(uint8_t, uint8_t)>(
        [](uint8_t from, uint8_t to) {
            cout << "  [async transition " << (int)from
                 << " -> " << (int)to << "]" << endl;
        })));

auto d1 = std::make_shared<MotorData>(); d1->speed = 100;
asyncMotor.SetSpeed(d1);    // returns immediately

auto d2 = std::make_shared<MotorData>(); d2->speed = 200;
asyncMotor.SetSpeed(d2);    // returns immediately

asyncMotor.Halt();

// Drain the queue and stop the SM thread before asyncMotor goes out of scope.
// Without this, the SM thread may still be processing events while the Motor
// destructor runs, causing a use-after-free.
smThread.ExitThread();
```

The key design property of active-object mode is **structural thread safety**: because all state logic runs on a single thread and all external events are serialized through the message queue, no mutex is needed inside the state machine itself. The safety comes from the architecture, not from locks.

The `shared_ptr<const EventData>` signature of `ExternalEvent()` is what makes async dispatch safe. DelegateMQ copies the shared pointer by value when building the async message, incrementing the reference count. The derived event data (e.g. `MotorData::speed`) remains alive and intact on the queue until the SM thread processes the event, at which point the shared pointer is released and the data is destroyed. A raw pointer would be deep-copied as the base `EventData` type, losing all derived fields.

# Validate

`Validate()` checks that every state in the map has at least one action delegate registered. Call it once after construction, before the first event, to catch states that were added to the `States` enum but never wired up in the constructor.

```cpp
Motor motor;

ASSERT_TRUE(motor.Validate([](uint8_t state) {
    cerr << "  [validate] state " << (int)state
         << " has no action registered" << endl;
}));
```

`Validate()` returns `true` if all states are wired. The optional callback receives the index of each offending state so you can log exactly which one is missing. `ASSERT_TRUE` hard-aborts if any state is unregistered. This catches configuration errors at startup rather than silently executing an empty delegate at runtime.

# State machine inheritance

Inheriting states allows common states to reside in a base class for sharing with derived classes. Some systems have a built-in self-test mode where the software executes a series of test steps to determine if the system is operational. In this example, each self-test has common states to handle Idle, Completed, and Failed states. Using inheritance, a base class contains the shared states. The classes `SelfTest` and `CentrifugeTest` demonstrate the concept.

![](CentrifugeTest.png)

**Figure 2: CentrifugeTest state diagram**

`SelfTest` defines the following states:

0. Idle
1. Completed
2. Failed

`CentrifugeTest` inherits those states and creates new ones:

3. Start Test
4. Acceleration
5. Wait For Acceleration
6. Deceleration
7. Wait For Deceleration

`SelfTest` registers its states in its own constructor. Since only the most-derived class knows the total state count, each level in the hierarchy passes its own `ST_MAX_STATES` upward through the constructor chain:

```cpp
// SelfTest.h
class SelfTest : public StateMachine {
public:
    SelfTest(uint8_t maxStates);
    virtual void Start() = 0;
    void Cancel();

protected:
    enum States { ST_IDLE, ST_COMPLETED, ST_FAILED, ST_MAX_STATES };

    void ST_Idle(const NoEventData* data);
    void ST_Completed(const NoEventData* data);
    void ST_Failed(const NoEventData* data);
    void EN_EntryIdle(const NoEventData* data);
};

// SelfTest.cpp
SelfTest::SelfTest(uint8_t maxStates) : StateMachine(maxStates)
{
    m_stateMap[ST_IDLE].action      += MakeStateDelegate(this, &SelfTest::ST_Idle);
    m_stateMap[ST_IDLE].entry       += MakeStateDelegate(this, &SelfTest::EN_EntryIdle);
    m_stateMap[ST_COMPLETED].action += MakeStateDelegate(this, &SelfTest::ST_Completed);
    m_stateMap[ST_FAILED].action    += MakeStateDelegate(this, &SelfTest::ST_Failed);
}
```

`CentrifugeTest` inherits those states and continues numbering from where `SelfTest` left off:

```cpp
// CentrifugeTest.h
class CentrifugeTest : public SelfTest {
    enum States {
        ST_START_TEST = SelfTest::ST_MAX_STATES,
        ST_ACCELERATION,
        ST_WAIT_FOR_ACCELERATION,
        ST_DECELERATION,
        ST_WAIT_FOR_DECELERATION,
        ST_MAX_STATES
    };
    // ...
};

// CentrifugeTest.cpp
CentrifugeTest::CentrifugeTest() : SelfTest(ST_MAX_STATES), m_pollActive(false), m_speed(0)
{
    // Override the base class Idle action while keeping the entry action from SelfTest
    m_stateMap[ST_IDLE].action.Clear();
    m_stateMap[ST_IDLE].action += MakeStateDelegate(this, &CentrifugeTest::ST_Idle);

    m_stateMap[ST_START_TEST].action  += MakeStateDelegate(this, &CentrifugeTest::ST_StartTest);
    m_stateMap[ST_START_TEST].guard    = MakeGuardDelegate(this, &CentrifugeTest::GD_GuardStartTest);

    m_stateMap[ST_ACCELERATION].action += MakeStateDelegate(this, &CentrifugeTest::ST_Acceleration);

    m_stateMap[ST_WAIT_FOR_ACCELERATION].action +=
        MakeStateDelegate(this, &CentrifugeTest::ST_WaitForAcceleration);
    m_stateMap[ST_WAIT_FOR_ACCELERATION].exit   +=
        MakeExitDelegate(this, &CentrifugeTest::EX_ExitWaitForAcceleration);

    m_stateMap[ST_DECELERATION].action += MakeStateDelegate(this, &CentrifugeTest::ST_Deceleration);

    m_stateMap[ST_WAIT_FOR_DECELERATION].action +=
        MakeStateDelegate(this, &CentrifugeTest::ST_WaitForDeceleration);
    m_stateMap[ST_WAIT_FOR_DECELERATION].exit   +=
        MakeExitDelegate(this, &CentrifugeTest::EX_ExitWaitForDeceleration);
}
```

Notice how `CentrifugeTest` overrides the base `ST_IDLE` action by calling `Clear()` on the multicast delegate and then re-registering its own implementation. The entry action registered by `SelfTest` is left intact. This gives the derived class full control over which parts of the base state behavior to keep or replace.

A guard condition blocks a transition if it returns `false`:

```cpp
bool CentrifugeTest::GD_GuardStartTest(const NoEventData* data)
{
    cout << "CentrifugeTest::GD_GuardStartTest" << endl;
    return m_speed == 0;   // only allow start if centrifuge is stopped
}
```

## Base class external event functions

When using state machine inheritance, it's not just the most-derived class that may define external event functions. The base and intermediary classes may also use transition maps. A parent state machine doesn't have awareness of child classes, so its transition map is a *partial* map covering only the states it knows about. Any event generated while the current state is outside the parent's enumeration range must be handled by the `PARENT_TRANSITION` macro placed directly above the transition map:

```cpp
void SelfTest::Cancel()
{
    PARENT_TRANSITION(ST_FAILED)

    BEGIN_TRANSITION_MAP                        // - Current State -
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_IDLE
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)     // ST_COMPLETED
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)     // ST_FAILED
    END_TRANSITION_MAP(nullptr)
}
```

The `PARENT_TRANSITION` example above is read "If the Cancel event is generated when the state machine is in a state that *this* class doesn't know about (i.e. a derived class state), transition to `ST_FAILED`". The macro-expanded code is:

```cpp
if (GetCurrentState() >= ST_MAX_STATES &&
    GetCurrentState() < GetMaxStates()) {
    ExternalEvent(ST_FAILED);
    return;
}
```

`GetCurrentState()` returns the current state. `ST_MAX_STATES` is the maximum `State` enum value known to this class. `GetMaxStates()` returns the maximum state value for the entire hierarchy. The code transitions to `ST_FAILED` if the current state is in a range owned by a derived class. If the current state is within the base class range, the partial transition map handles it normally. `EVENT_IGNORED` and `CANNOT_HAPPEN` are also valid arguments to `PARENT_TRANSITION`.

# State function inheritance

State functions can be overridden in the derived class. The derived class may call the base implementation if desired. In this case, `SelfTest` declares and defines an Idle state:

```cpp
void SelfTest::ST_Idle(const NoEventData* data)
{
    cout << "SelfTest::ST_Idle" << endl;
}

void SelfTest::EN_EntryIdle(const NoEventData* data)
{
    cout << "SelfTest::EN_EntryIdle" << endl;
}
```

`CentrifugeTest` also defines the same Idle state. The override replaces the action delegate in the constructor using `Clear()` and then re-binding. Within the override, it calls the base implementation explicitly:

```cpp
void CentrifugeTest::ST_Idle(const NoEventData* data)
{
    cout << "CentrifugeTest::ST_Idle" << endl;
    SelfTest::ST_Idle(data);   // call base class Idle
    StopPoll();
}
```

The entry action (`EN_EntryIdle`) registered by `SelfTest` is not cleared by `CentrifugeTest`, so it continues to fire when entering Idle. State function inheritance is a powerful mechanism to allow each level within the hierarchy to process the same state.

# Multithread safety

This implementation provides two modes of thread safety:

**Synchronous mode** — `ExternalEvent()` runs on the caller's thread. If a `StateMachine` instance is called by multiple threads, the caller is responsible for ensuring mutual exclusion (e.g. by using a mutex around the event call). Since all state logic completes within the `ExternalEvent()` call frame, a simple lock around the call is sufficient.

**Active-object mode** — calling `SetThread()` before the first event enables structural thread safety. All external events are marshaled to the SM thread's message queue. Because every event is serialized through that single queue, no mutex is needed inside the state machine. The thread safety is enforced by architecture rather than by locks.

In active-object mode, `ExternalEvent()` returns immediately after enqueuing the event. State logic, guard conditions, entry/exit actions, and all signals execute on the SM thread. External code must not assume that a state transition has completed when `ExternalEvent()` returns — the transition is pending on the queue.

Always call `smThread.ExitThread()` before the `StateMachine` object is destroyed in async mode. This drains the message queue and stops the thread, preventing a use-after-free if the SM thread is still processing an event while the destructor runs.

# Fixed-block allocator

On embedded systems, dynamic heap allocation can lead to fragmentation and unpredictable allocation failure over time. This project includes an optional fixed-block pool allocator, `xallocator`, that allocates memory from pre-sized pools rather than the general heap.

When the `DMQ_ALLOCATOR` build option is enabled, the following type aliases are active:

- `xlist<T>`, `xstring`, etc. — same pattern for other standard containers.
- `xmake_shared<T>(...)` — allocates both the object and the `shared_ptr` control block from the fixed-block pool using `std::allocate_shared`.

When `DMQ_ALLOCATOR` is disabled (the default), all of the above fall back to their standard library equivalents transparently — `xmake_shared` calls `std::make_shared`. Application code compiles identically in both configurations.

```cpp
// Works whether DMQ_ALLOCATOR is on or off
auto data = xmake_shared<MotorData>();
data->speed = 100;
motor.SetSpeed(data);
```

For more information on the underlying allocator, see the article [**Replace malloc/free with a Fast Fixed Block Memory Allocator**](http://www.codeproject.com/Articles/1084801/Replace-malloc-free-with-a-Fast-Fixed-Block-Memory).

# Comparison with other libraries

The table below compares this implementation against several widely used C++ state machine libraries across the features most relevant to embedded and multithreaded applications.

| Feature | This implementation | Boost.MSM | Boost.Statechart | SML (kgrzybek) | TinyFSM | QP/C++ |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| Compact binary footprint | ✓ | — | — | ✓ | ✓ | — |
| No external dependencies | ✓ | — | — | ✓ | ✓ | — |
| Embedded-friendly | ✓ | — | — | ✓ | ✓ | ✓ |
| Runtime state registration | ✓ | — | — | — | — | — |
| Typed event data per state | ✓ | ✓ | ✓ | ✓ | — | ✓ |
| Guard conditions | ✓ | ✓ | ✓ | ✓ | — | ✓ |
| Entry / exit actions | ✓ | ✓ | ✓ | ✓ | — | ✓ |
| State machine inheritance | ✓ | — | ✓ | — | — | ✓ |
| Built-in async active-object | ✓ | — | — | — | — | ✓ |
| Pub/sub signals (OnTransition etc.) | ✓ | — | — | — | — | — |
| `shared_ptr` event data (async safe) | ✓ | — | — | — | — | — |
| Validate() at startup | ✓ | — | — | — | — | — |
| C++17 standard only | ✓ | ✓ | ✓ | ✓ | ✓ | — |
| Hierarchical SM (HSM) | — | ✓ | ✓ | — | — | ✓ |
| Compile-time transition checking | partial | ✓ | — | ✓ | ✓ | — |

### Boost.MSM

Boost.MSM is one of the most feature-complete C++ SM libraries available. Transitions are defined entirely at compile time as a table of `Row<>` type entries, and the optimizer can reduce the dispatch overhead to near zero. The tradeoff is significant complexity: template metaprogramming drives the entire design, compile times are long, and error messages are notoriously difficult to interpret. It requires the full Boost installation and is impractical on most embedded targets.

### Boost.Statechart

Boost.Statechart provides a runtime Hierarchical State Machine that closely follows the UML semantics including orthogonal regions and history states. The runtime flexibility comes at a cost: each state is a separate heap-allocated object with virtual dispatch at every transition. The Boost dependency and per-state heap allocation make it unsuitable for constrained embedded systems.

### SML (kgrzybek)

SML is a modern, header-only C++14 library that defines transitions using a concise DSL. It has no heap allocation, no RTTI requirement, and compiles to very tight code. The main limitation is that the entire state machine structure — states, events, guards, and transitions — must be expressed as a compile-time type list. This makes runtime introspection, signals, and dynamic observer attachment impossible without significant extra work. There is also no built-in threading support.

### TinyFSM

TinyFSM is designed specifically for small embedded targets. The entire library is a single header with no dependencies, and dispatch is done through `static` member functions — effectively one state machine instance per type. This makes it very simple and very fast, but it means you cannot have two independent instances of the same state machine class. There is no support for per-transition typed event data, guards, or entry/exit actions in the base design.

### QP/C++

QP/C++ is a full embedded RTOS framework built around hierarchical active objects. It is battle-tested in safety-critical systems and supports a rich UML-compliant HSM with orthogonal regions, deferred events, and publish/subscribe. The active-object model provides the same structural thread safety as `SetThread()` here. The tradeoff is that QP is an entire framework, not a library you drop into an existing project — it brings its own event pool, time-event management, and RTOS abstraction layer. Commercial use may require a license.

### Summary

This implementation occupies the practical middle ground. It is compact and embedded-friendly like TinyFSM and SML, supports guards/entry/exit/inheritance like Boost.MSM and QP/C++, and uniquely adds a built-in async active-object mode and pub/sub signals that none of the others provide out of the box. Runtime delegate registration keeps the syntax straightforward — state functions are plain member functions with no special macros or template parameter packs — while the typed delegate adapters still catch signature errors at compile time. For projects that need a capable FSM without adopting a full framework or wrestling with heavy template metaprogramming, this design hits a practical sweet spot.


