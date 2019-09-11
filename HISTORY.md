## CS-3950 Progress Report History
Update Progress (Starting August 2019)
- August 28-30:
    - Interrupt handler manager was refactored to use function pointers with loops (w/o tutorial)
    - Keyboard driver was refactored to use allow shell access (added key buffer)
    - Kernel level shell (Basch) was further implemented
    - Basic terminal commands were added for debugging
    - Kernel panics no longer infinitely cycle text on screen. (Halt asm is now called)
    - VGA drivers were implemented in a hack-ish manner (lots of temporary code)
    - Unused kernel libc functions and files were removed to slim kernel size
    - Ported some useful string functions from the master branch into the Switch-to-Grub branch
    - Basic refactorization in various areas (variable renames, comment and syntax cleaning, etc.)

- September 1-7:
    - Added shutdown command which allows the kernel to complete and halt the processor (w/o tutorial)
    - Refactored some of the interrupt logic to be more centrally located (w/o tutorial)
    - Added exception descriptions to help debug when a panic occurs (w/o tutorial)
    - Added the cowsay "deadcow" art when the kernel panics (w/o tutorial)
    - Added rudementary PC speaker support. Plays any square wave frequency. (w/o tutorial)
- September 8-14:
    - Added move VGA support for windows, widgets, desktop, etc. Ongoing.
    - Added CPU timer to the InterruptManager (w/o tutorial)
    - Accidentally tested and proved that memory protection and it's associated kernel panic works perfectly.
    - Cleaned up more InterruptManager code by moving panic code to a function in stdio.hpp (w/o tutorial)

### Course Requirements:
Turn-in Requirements:
- Final Report - 8+ pages
- Biweekly Progress Report
- Two proposed lab assignment proposals
- One OS project proposal
- List of resources/bibliography