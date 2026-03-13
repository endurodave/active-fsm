#include "Motor.h"
#include "Player.h"
#include "CentrifugeTest.h"
#include "delegate-mq/DelegateMQ.h"
#include "delegate-mq/predef/util/Fault.h"
#include <iostream>

// @see https://github.com/endurodave/delegate-fsm
// David Lafreniere

using namespace std;
using namespace dmq;

int main()
{
    // -----------------------------------------------------------------------
    // Synchronous Motor — ExternalEvent runs on the caller's thread.
    // SetSpeed/Halt block until the state action completes before returning.
    // -----------------------------------------------------------------------
    cout << "=== Synchronous Motor ===" << endl;

    Motor syncMotor;

    // Validate that every state has an action registered. Call once after
    // construction, before the first event, to catch states added to the enum
    // but never wired up in the constructor. The callback receives each
    // offending state index; ASSERT_TRUE hard-aborts if any are missing.
    ASSERT_TRUE(syncMotor.Validate([](uint8_t state) {
        cerr << "  [validate] state " << (int)state << " has no action registered" << endl;
    }));

    // OnTransition fires after every completed state action.
    // fromState == toState indicates a self-transition.
    auto syncConn = syncMotor.OnTransition.Connect(
        MakeDelegate(std::function<void(uint8_t, uint8_t)>(
            [](uint8_t from, uint8_t to) {
                cout << "  [transition " << (int)from << " -> " << (int)to << "]" << endl;
            })));

    // OnEntry/OnExit fire on every actual state change — not on self-transitions.
    // OnExit fires before the new state's entry action; OnEntry before its action.
    auto entryConn = syncMotor.OnEntry.Connect(
        MakeDelegate(std::function<void(uint8_t)>(
            [](uint8_t state) {
                cout << "  [entry " << (int)state << "]" << endl;
            })));

    auto exitConn = syncMotor.OnExit.Connect(
        MakeDelegate(std::function<void(uint8_t)>(
            [](uint8_t state) {
                cout << "  [exit " << (int)state << "]" << endl;
            })));

    // OnCannotHappen fires before FaultHandler (which calls abort) when a
    // CANNOT_HAPPEN transition is taken. Use this to flush logs, record the
    // offending state, or trigger a controlled shutdown before the process dies.
    auto faultConn = syncMotor.OnCannotHappen.Connect(
        MakeDelegate(std::function<void(uint8_t)>(
            [](uint8_t state) {
                cerr << "  [CANNOT_HAPPEN from state " << (int)state << "]" << endl;
            })));

    // xmake_shared falls back to std::make_shared when DMQ_ALLOCATOR is disabled
    // xmake_shared uses the fixed-block pool allocator when DMQ_ALLOCATOR is enabled
    auto d1 = xmake_shared<MotorData>(); d1->speed = 100;
    syncMotor.SetSpeed(d1);   // blocks — state executes before returning

    auto d2 = xmake_shared<MotorData>(); d2->speed = 200;
    syncMotor.SetSpeed(d2);

    syncMotor.Halt();
    syncMotor.Halt();   // ignored — motor is already idle

    // -----------------------------------------------------------------------
    // Asynchronous Motor — active-object pattern.
    // SetSpeed/Halt post to the SM thread and return immediately.
    // All state logic, guards, entry/exit, and signals fire on that thread.
    // No mutex needed inside the SM — structural thread safety via dispatch.
    // -----------------------------------------------------------------------
    cout << "\n=== Asynchronous Motor (Active Object) ===" << endl;

    Thread smThread("MotorSMThread");
    smThread.CreateThread();

    Motor asyncMotor;
    asyncMotor.SetThread(smThread);

    auto asyncConn = asyncMotor.OnTransition.Connect(
        MakeDelegate(std::function<void(uint8_t, uint8_t)>(
            [](uint8_t from, uint8_t to) {
                // Fires on smThread — output may interleave with main thread cout
                cout << "  [async transition " << (int)from << " -> " << (int)to << "]" << endl;
            })));

    auto a1 = std::make_shared<MotorData>(); a1->speed = 100;
    cout << "Posting SetSpeed(100)..." << endl;
    asyncMotor.SetSpeed(a1);   // returns immediately; SM thread processes asynchronously

    auto a2 = std::make_shared<MotorData>(); a2->speed = 200;
    cout << "Posting SetSpeed(200)..." << endl;
    asyncMotor.SetSpeed(a2);

    cout << "Posting Halt()..." << endl;
    asyncMotor.Halt();

    cout << "Posting Halt() again (will be ignored)..." << endl;
    asyncMotor.Halt();

    // Drain the queue and stop the SM thread before asyncMotor goes out of scope.
    // Without this the SM thread may still be processing events while the Motor
    // destructor runs, causing a use-after-free.
    smThread.ExitThread();

    // -----------------------------------------------------------------------
    // Player and CentrifugeTest (synchronous)
    // -----------------------------------------------------------------------
    cout << "\n=== Player ===" << endl;

    Player player;
    player.OpenClose();
    player.OpenClose();
    player.Play();
    player.Pause();
    player.EndPause();
    player.Stop();
    player.Play();
    player.Play();
    player.OpenClose();

    cout << "\n=== CentrifugeTest ===" << endl;

    CentrifugeTest test;
    test.Cancel();
    test.Start();
    while (test.IsPollActive())
        test.Poll();

    return 0;
}
