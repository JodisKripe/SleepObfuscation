# Ekko rebuild

This is an ekko rebuild to understand what is actually happening when ekko sleep obfuscation takes place.
Original Code : [Cracked5pider](https://github.com/Cracked5pider/Ekko)

# Ekko Flow
1. Obtain the context of current thread
2. Clone the context to multiple variables
3. Modify variables to resume execution as if they were about to execute functions in the execution sequence.
4. Use System timers to resume context using NtContinue in a particular sequence.
5. Wait for all the context based functions to execute.
6. Delete TimerQueue

Ideally, this would go in a a loop where the EkkoSleep will replace sleep for the time where an implant is gonna sleep and not be a decrypted payload in the memory.

# Execution Sequence
1. Make the whole image Read/Write.
2. Encrypt the whole image.
3. Sleep for whatever time.
4. Decrypt the whole image.
5. Make the whole image Read/Execute.
6. Set the event object that the main thread is waiting form