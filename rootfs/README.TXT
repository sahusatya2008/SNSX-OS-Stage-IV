SNSX live root filesystem

This image is loaded by GRUB as a Multiboot module and copied into a writable
RAM-backed virtual filesystem during boot.

Key ideas:

- The bundled files below are your starting point.
- Changes made with edit, touch, mkdir, rm, cp, and mv live for the current boot.
- The shell includes built-in apps such as files, calc, and edit.

Suggested first commands:

help
apps
ls /
ls /home
cat /home/WELCOME.TXT
files
calc (2 + 3) * 4
edit /home/NOTES.TXT
run /bin/hello.app
