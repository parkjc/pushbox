Pushbox (mailbox)  
=================    

This repository contains the files that read the weight from the pushbox and send the data to a database. The code contained here has been adapted for the Raspberry Pi.  

Installation  
------------  
Usbscale requires the development headers "libusb-1.0". "udev" rules may also need to be added in "/etc/udev/rules.d". The rules required are in the file "50-usb-scale.rules". More information can be found [here](https://github.com/erjiang/usbscale).  

WiringPi also needs to be installed. Get the code from "git.drogon.net/wiringPi". Then run "./build" in the wiringPi directory. More information can be found [here](http://wiringpi.com/download-and-install/).  

MySQL requires installing MySQL on the Raspberry Pi. This can be installed with "sudo apt-get install libmysqlclient-dev".  

Download "pushbox" as a zip or get the repository with "git clone https://github.com/parkjc/pushbox".  
Compile the code with "gcc $(mysql_config --libs) -l wiringPi -lm -lusb-1.0 -Os -Wall -o AutoMail AutoMail.c".  
Next as "root", use the command "chmod 755 AutoMail" to make it an executable.  
Lastly as "root", edit the "/etc/rc.local" file to contain the line "path-to-pushbox/pushbox/AutoMail &" (This will run the code on boot).    

Usage  
-----  
After compiling the code, it can be either run after reboot or directly with "sudo ./AutoMail".  


Credits  
-------  
[Usbscale](https://github.com/erjiang/usbscale) is a program that reads the weight from a USB scale. Usbscale is licensed under a GPLv3 license.    
[WiringPi](http://wiringpi.com/) is a program for accessing the Raspberry Pi GPIOs. Wiring Pi is licensed under a GNU GPLv3 license.  

License  
-------  
Pushbox (mailbox) is licensed under the GPLv3. The license can be found in the file "LICENSE.txt".  

Contact  
-------  

