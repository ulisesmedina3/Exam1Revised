#!/bin/bash

SCORE=0
RAND_NUM=$((1 + RANDOM % 100))
SHM_NAME="/encoded_message_${RAND_NUM}"

if ! command -v strace >/dev/null 2>&1; then
    echo "✘ strace is required for this performance check but is not installed."
    echo "Final Grade: 0 / 60"
    exit 1
fi

# Cleanup
rm -f encoder coordinator decoder strace_output.log execution_output.log

echo "--- Compiling Programs ---"
gcc -o ../build/coordinator ../src/main.c -lrt
gcc -o ../../encoder/build/encoder ../../encoder/src/main.c -lrt
gcc -o ../../decoder/build/decoder ../../decoder/src/main.c -lrt

if [ ! -x "../build/coordinator" ] || [ ! -x "../../encoder/build/encoder" ] || [ ! -x "../../decoder/build/decoder" ]; then
    echo "✘ Compilation failed."
    echo "Final Grade: 0 / 60"
    exit 1
fi

echo -e "\n--- Running Practical Evaluation ---"
# strace captures execve (process creation), kill (signals), and the under-the-hood
# open/unlink calls that Linux uses for POSIX shared-memory objects.

timeout --signal=TERM --kill-after=2s 8s strace -f -e trace=execve,kill,openat,unlinkat -o strace_output.log ../build/coordinator $SHM_NAME > execution_output.log 2>&1

echo -e "\n--- Verification Results ---"

# 1. Verify both processes were launched via execve (12 pts)
TEST1_PASSED=0
if grep -q 'execve(.*"encoder"' strace_output.log && grep -q 'execve(.*"decoder"' strace_output.log; then
    echo "✔ Both Encoder and Decoder processes were successfully launched. (+12 pts)"
    SCORE=$((SCORE + 12))
    TEST1_PASSED=1
else
    echo "✘ Failed to detect launch of both child processes."
fi

# 2. Verify memory block created with expected random name (9 pts)
TEST2_PASSED=0
if [ "$TEST1_PASSED" -ne 1 ]; then
    echo "✘ Test 2 cannot pass because test 1 did not pass."
elif grep -q "openat(AT_FDCWD, \"/dev/shm${SHM_NAME}\"" strace_output.log; then
    echo "✔ Shared memory block $SHM_NAME was verified in system calls. (+9 pts)"
    SCORE=$((SCORE + 9))
    TEST2_PASSED=1
else
    echo "✘ Shared memory block name mismatch or creation failed."
fi

# 3. Verify encoder handles SIGUSR2 (6 pts)
TEST3_PASSED=0
if [ "$TEST2_PASSED" -ne 1 ]; then
    echo "✘ Test 3 cannot pass because test 2 did not pass."
elif grep -q "Encoder: Received SIGUSR2" execution_output.log; then
    echo "✔ Encoder received SIGUSR2 and handled the signal. (+6 pts)"
    SCORE=$((SCORE + 6))
    TEST3_PASSED=1
else
    echo "✘ Encoder did not report receiving SIGUSR2."
fi

# 4. Verify decoded message output format (21 pts)
TEST4_PASSED=0
if [ "$TEST3_PASSED" -ne 1 ]; then
    echo "✘ Test 4 cannot pass because test 3 did not pass."
elif grep -q "\[DECODED MESSAGE\]: " execution_output.log; then
    echo "✔ Decoder correctly intercepted the memory block and printed the decoded message. (+21 pts)"
    SCORE=$((SCORE + 21))
    TEST4_PASSED=1
    grep "\[DECODED MESSAGE\]:" execution_output.log
else
    echo "✘ Failed to find the output format '[DECODED MESSAGE]:' in standard output."
fi

# 5. Verify decoder unlinks the memory block (12 pts)
if [ "$TEST4_PASSED" -ne 1 ]; then
    echo "✘ Test 5 cannot pass because test 4 did not pass."
elif [ ! -e "/dev/shm${SHM_NAME}" ]; then
    echo "✔ Decoder properly unlinked the shared memory block upon termination. (+12 pts)"
    SCORE=$((SCORE + 12))
else
    echo "✘ Failed to unlink the shared memory block from the OS."
fi

# Verify lingering shared memory to be absolutely sure
if [ -f "/dev/shm$SHM_NAME" ]; then
    echo "⚠️ Warning: Found lingering memory block in /dev/shm. Unlinking failed."
    rm -f "/dev/shm$SHM_NAME"
fi

echo -e "\n============================="
echo "Final Grade: $SCORE / 60"
echo "============================="

