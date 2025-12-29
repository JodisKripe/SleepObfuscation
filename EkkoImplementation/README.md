# Ekko Notes
While implementing Ekko, the binary image becomes encrypted during sleep time<br>
![memory](../media/ekkomem1.png)

When the sleep time finishes, the binary image gets decrypted for the time we need to execute rest of the things.
![memory](../media/ekkomem2.png)

Since the whole image gets encrypted, hence our callstack also gets corrupted. There are three situations which our call stack is in:

1. When the main thread is waiting for `hEvent` to be set.<br>
![ekkk_stack](../media/ekkoStack1.png)

2. When the image is decrypted<br>
![ekko_stack](../media/ekkoStack2.png)

3. The small Sleep segment where we want to wait for the context to be captured.
![ekko_stack](../media/ekkoStack3.png)

## Hunt Sleeping Beacons
[HSB](https://github.com/thefLink/Hunt-Sleeping-Beacons) is able to detect these issues in the call stack and list our the following issues:<br>

1. When the main thread is waiting for `hEvent` to be set.<br>
![Ekk](../media/ekkoHSB1.png)

2. When the image is decrypted<br>
![Ekk](../media/ekkoHSB2.png)

> The critical issues are something that I will target at a later stage, first analysis of detection of Blocked Timers is something i want to remove while following along [Dylan Tran's modification to Ekko](https://dtsec.us/2023-04-24-Sleep/).