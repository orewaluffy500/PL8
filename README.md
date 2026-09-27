# PL8

PL8 is me trying to re-create a CPU.

I got motivation to do this from "misterbob" on you-tube, However the main difference is that bob actually made a CPU and I just made a "CPU emulator simulator"

Anyways, I still had a lot of fun developing this project and I'm still working on it.

I even made an assembler for it.

## INSTRUCTIONS
1. To assemble a program do this

`xmake run --workdir=. asm` or if you've compiled it then just `asm.exe`

  It'll then ask for a file name, That's your PL8 assembly file.
  It'll produce a new file with the same name but `.pa` at the end

2. To run your program do this

`xmake run --workdir=. vm` or if you've compiled it then just `vm.exe`

  It'll ask for a executable file name, give it the name of the file that `asm.exe` produced.
  Then it'll run the program, Also it'll warn you about any invalid instructions but it wont stop.

# DOCUMENTATION

I'm not really someone who writes documentation so just read the `assembly_rules.txt` and `conventions.txt` file and you should get the basics.
