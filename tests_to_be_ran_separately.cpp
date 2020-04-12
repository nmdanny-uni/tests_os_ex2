#include <uthreads.h>
#include <iostream>
#include <gtest/gtest.h>
#include <random>
#include <algorithm>

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *                        IMPORTANT
 * Each test, that is, whatever begins with TEST(XXX,XXX),
 * must be ran separately, that is, by clicking on the green button
 * and doing 'Run XXX.XXX'
 *
 * If you run all tests in a file/directory, e.g, by right clicking this .cpp
 * file and doing 'Run all in ...', THE TESTS WILL FAIL
 *
 * The reason is, each test expects a clean slate, that is, it expects that
 * the threads library wasn't initialized yet (and thus will begin with 1 quantum
 * once initialized).
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

TEST(DO_NOT_RUN_ALL_TESTS_AT_ONCE, READ_THIS)
{
    FAIL() << "Do not run all tests at once via CLion, use the provided script" << std::endl;
}

// conversions to microseconds:
static const int MILLISECOND = 1000;
static const int SECOND = MILLISECOND * 1000;

/**
 * This function initializes the library with given priorities.
 * If it fails, either you didn't implement these correctly,
 * or you ran multiple tests at the same time - see above note.
 */
template <int priorityCount>
void initializeWithPriorities(int lengths[priorityCount])
{
    ASSERT_EQ( uthread_init(lengths, priorityCount), 0);
    // First total number of quantums is always 1(the main thread)
    ASSERT_EQ ( uthread_get_total_quantums(), 1);
}



/**
 * Causes the currently running thread to sleep for a given number
 * of thread-quantums, that is, by counting the thread's own quantums.
 * Unlike "block", this
 *
 *
 *
 * @param threadQuants Positive number of quantums to sleep for
 */
void threadQuantumSleep(int threadQuants)
{
    assert (threadQuants > 0);
    int myId = uthread_get_tid();
    int start = uthread_get_quantums(myId);
    int end = start + threadQuants;
    /* Note, from the thread's standpoint, it is almost impossible for two consecutive calls to
     * 'uthread_get_quantum' to yield a difference larger than 1, therefore, at some point, uthread_get_quantums(myId)
     * must obtain 'end'.
     *
     * Theoretically, it's possible that the thread will be preempted before the condition check occurs, if this happens,
     * you'll have an infinite loop(at least if/until the quantum count overflows and returns to 'end'...)
     * But with the quantum lengths specified in the test initialization, this should NOT happen.
     *
     * Therefore, if somehow you do have an infinite loop here, the problem might be on your end.
     */
    while (uthread_get_quantums(myId) != end)
    {
    }
}


TEST(Test1, BasicFunctionality)
{
    int priorites[] = { 100 * MILLISECOND};
    initializeWithPriorities<1>(priorites);

    // main thread has only started one(the current) quantum
    EXPECT_EQ(uthread_get_quantums(0), 1);

    static bool ran = false;
    // most CPP compilers will translate this to a normal function call (there's no closure)
    auto t1 = []()
    {
        EXPECT_EQ(uthread_get_tid(), 1);

        // every thread begins with 1 quantums
        EXPECT_EQ(uthread_get_quantums(1), 1);

        // main thread's quantums are unchanged
        EXPECT_EQ(uthread_get_quantums(0), 1);

        // this is the 2nd quantum in the program entire run
        EXPECT_EQ(uthread_get_total_quantums(), 2);
        ran = true;
        EXPECT_EQ ( uthread_terminate(1), 0);
    };
    EXPECT_EQ(uthread_spawn(t1, 0), 1);
    // spawning a thread shouldn't cause a switch
    // while theoretically it's possible that at the end of uthread_spawn
    // we get a preempt-signal, with the quantum length we've specified at the test initialization, it shouldn't happen.

    // (unless your uthread_spawn implementation is very slow, in which case you might want to check that out, or
    //  just increase the quantum length at test initialization)

    EXPECT_EQ(uthread_get_total_quantums(), 1);
    EXPECT_EQ(uthread_get_quantums(0), 1);

    // see implementation of this function for explanation
    threadQuantumSleep(1);

    EXPECT_TRUE(ran);
    EXPECT_EQ(uthread_get_quantums(0), 2);
    EXPECT_EQ(uthread_get_total_quantums(), 3);

   ASSERT_EXIT(uthread_terminate(0), ::testing::ExitedWithCode(0), "");
}


TEST(Test2, ThreadInteraction)
{
    int priorities[] = { 100 * MILLISECOND };
    initializeWithPriorities<1>(priorities);


    // maps number of passed quantums(ran in each of the loops)
    // to thread id
    static std::map<int, int> quantumsToTids;

    static std::vector<int> sequence;

    // thread 1 emits the numbers 1,2,3,4 to sequence,
    // blocking itself after every number
    auto t1 = [](){
        int tid = 1;
        EXPECT_EQ(tid, uthread_get_tid());
        for (int i=1; i <= 4; ++i)
        {
            EXPECT_EQ( uthread_get_quantums(tid), i );
            sequence.push_back(i);
            quantumsToTids[uthread_get_total_quantums()] = tid;
            EXPECT_EQ( uthread_block(tid), 0);
        }
        EXPECT_EQ( uthread_terminate(tid), 0);
    };

    // thread 2 does the same as 1, but emits -1,-2,-3,-4
    auto t2 = [](){
        int tid = 2;
        EXPECT_EQ(tid, uthread_get_tid());
        for (int i=1; i <= 4; ++i)
        {
            EXPECT_EQ( uthread_get_quantums(tid), i );
            sequence.push_back(-i);
            quantumsToTids[uthread_get_total_quantums()] = tid;
            EXPECT_EQ( uthread_block(tid), 0);

        }
        EXPECT_EQ( uthread_terminate(tid), 0);
    };

    EXPECT_EQ(uthread_spawn(t1, 0), 1);
    EXPECT_EQ(uthread_spawn(t2, 0), 2);

    // meanwhile, thread 0 (which began first) will be emitting
    // '50', '60', '70', '80'
    for (int i=1; i <= 4; ++i)
    {

        // if both threads are blocked(for i>=2),
        // we'll first enqueue 2 then 1 into the ready queue
        // otherwise(i=1) this has no effect
        EXPECT_EQ( uthread_resume(2), 0);
        EXPECT_EQ( uthread_resume(1), 0);

        // see below the expected thread order
        EXPECT_EQ(uthread_get_quantums(0), i);
        sequence.push_back(40 + i*10);
        quantumsToTids[uthread_get_total_quantums()] = 0;
        threadQuantumSleep(1);
    }

    // expected thread order should be:
    // 0-1-2 2-1-0 2-1-0 2-1-0
    std::vector<int> expectedSequence { 50, 1, -1,
                                        60, -2, 2,
                                        70, -3, 3,
                                        80, -4, 4};
    EXPECT_EQ(sequence, expectedSequence);

    // if the above succeeded, so should the following one
    // unless 'uthread_get_total_quantums' is bugged
    std::map<int, int> expectedQuantumsToTids {
        {1, 0},
        {2,1},
        {3,2},
        {4,0},
        {5,2},
        {6,1},
        {7,0},
        {8,2},
        {9,1},
        {10,0},
        {11,2},
        {12,1}
    };
    EXPECT_EQ( quantumsToTids, expectedQuantumsToTids);

    ASSERT_EXIT(uthread_terminate(0), ::testing::ExitedWithCode(0), "");
}


/** This test involves multiple aspects:
 * - Spawning the maximal amount of threads, all running at the same time
 * - Terminating some of them after they've all spawned and ran at least once
 * - Spawning some again, expecting them to get the lowest available IDs
 */
TEST(Test3, StressTestAndThreadCreationOrder) {
    // you can increase the quantum length, but even the smallest quantum should work
    int priorities[] = { 1 };
    initializeWithPriorities<1>(priorities);

    // this is volatile, otherwise when compiling in -O2, the compiler considers the waiting loop further below
    // as an infinite loop and optimizes it as such.
    static volatile int ranAtLeastOnce = 0;
    auto f = [](){
        ++ranAtLeastOnce;
        while (true) {}
    };

    // you can lower this if you're trying to debug, but this should pass as is
    const int SPAWN_COUNT = MAX_THREAD_NUM - 1;
    std::vector<int> spawnedThreads;
    for (int i=1; i <= SPAWN_COUNT ; ++i)
    {
        EXPECT_EQ ( uthread_spawn(f, 0), i);
        spawnedThreads.push_back(i);
    }

    // wait for all spawned threads to run at least once
    while (ranAtLeastOnce != SPAWN_COUNT) {}

    if (SPAWN_COUNT == MAX_THREAD_NUM - 1) {
        // by now, including the 0 thread, we have MAX_THREAD_NUM threads
        // therefore further thread spawns should fail

        EXPECT_EQ ( uthread_spawn(f, 0), -1);
    }

    // now, terminate 1/3 of the spawned threads (selected randomly)
    std::random_device rd;
    std::shuffle(spawnedThreads.begin(),
                 spawnedThreads.end(),
                 rd);
    std::vector<int> threadsToRemove ( spawnedThreads.begin(),
                                       spawnedThreads.begin() + SPAWN_COUNT * 1/3);
    for (const int& tid: threadsToRemove)
    {
        EXPECT_EQ (uthread_terminate(tid), 0);
    }

    // now, we'll spawn an amount of threads equal to those that were terminated
    // we expect the IDs of the spawned threads to equal those that were
    // terminated, but in sorted order from smallest to largest
    std::sort(threadsToRemove.begin(), threadsToRemove.end());
    for (const int& expectedTid: threadsToRemove)
    {
        EXPECT_EQ (uthread_spawn(f, 0), expectedTid);
    }

    ASSERT_EXIT ( uthread_terminate(0), ::testing::ExitedWithCode(0), "");
}
