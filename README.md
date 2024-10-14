# SevensegLKM
Simple &amp; lightweight Linux Kernel Module to operate a 7-segment Display with common cathode targeted to be used with Raspberry Pi. Project also contains a Python app that enables interaction with each separate segment of the display through a GUI.

Used as practical example of a lecture given at the 5th WeekEng - Computer Engineering Week at IFSULDEMINAS Poços de Caldas, MG, Brazil.

Instructions to build and run:
* make
* sudo insmod sevenseg.ko
* sudo chmod 666 /dev/sevenseg
* echo ‘1110111’ > /dev/sevenseg # writes a binary string to the chardev turning each mapped pin on or off
* head -n 1 /dev/sevenseg  # reads the chardev current value
* sudo rmmod sevenseg


![Untitled Sketch 2_bb](https://github.com/user-attachments/assets/7129862c-8892-4eda-b3ef-2dea17a68c26)
![imagem_2024-10-14_000350911](https://github.com/user-attachments/assets/c5689e62-7ec3-4862-aa6c-c231aedbddde)


