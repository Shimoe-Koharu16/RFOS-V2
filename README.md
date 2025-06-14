# RFOS-V2
A predecessor of <a href=https://github.com/Shimoe-Koharu16/TagSyncNode>TagSyncNode</a>. RFOS V2 now has toast notifications, alert() and OTA updates via PrettyOTA. A feature was deprecated temporarily and will be soon added back

## Introduction of RFOS-V2

### Why is this made?
This repository is specially made for a local server that I made for my former high school (Kinatakutan National High School) ICT head/administrator (To keep her job easy and not to go 1-by-1 PCs whenever a worksheet is to be submitted or given out) to still be able to receive updates for the server remotely since I transferred to another school for the strand I'm suitable for (~~and also, to avoid more encounters with Ashley Nicole for my own sanity and well-being... you can read this? good not good~~). Actually this repository shouldn't be even up here in public since they paid me for this 

### Why did you made this public even tho they paid you for this?
Good question, they just actually paid for ONLY the hardware of the TagSyncNode repository and that hardware also supports RFOS V2 but soon might be unsupported by the time RFOS V3 drops. I won't post this repository in public mode if they DID paid me for the hardware + software + frontend + backend or logic of this repository before I left.

### Where did this repository got its name?
My brain of course. I'm just kidding, the name 'RFOS' actually came from its components/features, RFOS stands for **R**FID and **F**iles **O**perating **S**ystem (~~That was the most made-up shit I made... but it does kinda resembles an OS dues to its file management and IO with task scheduling~~). If you feel like the name is misleading, either do I. because if other name was used, this would probably sound cringe.

### What modules this uses to maximize it's functionality?

- MFRC522 = HSPI
- SD Card Module = HSPI also
- RGB LED = Configurable pins on the .ino
- DS1302 = I don't know why won't it run on ESP32 but runs on Arduino Nano, R3 and R4 Wi-Fi (incompatibility? I'm currently ~~fixing~~ it)

## How to Use RFOS V2
 ### Accessing the web interface
  Configure the UPLINK_SSID and UPLINK_PASS parameters (or #define whatever you call it) accordingly to your router credentials (Only works with 2.4G and ~~5G~~) and visit fileserver.local and continue to the site even tho its not secured since HTTPS isn't natively supported by ESP32-DW0Q-V3 chipset for it requires massive RAM (or heap) allocations and CPU power
  ### Renaming the RFID cards for its attendance 
 Using the Serial 'Send a message to ESP32 on COM X' and there you type the name of the card holder, they can be overwritten. There's two options for this, if you want separate FIRST_NAME and LAST_NAME, use a comma (,) to separate the names since this records attendance in CSV (Comma Separated Values) file format on /attendance subfolder (E.X: FIRST_NAME,LAST_NAME)

 ### Updating the Server with the Latest Releases
 If you already flashed the baseplate (the RFOS_V2.ino with Settings.h) in the ESP32 your server uses, just visit fileserver.local/update and put the latest binaries of this server to the "Upload .bin file" box of the interface, and if the update finished successfully. Please restart your server by unplugging the power source and plug it back in if it doesn't show the screen that says **Update Complete** after a minute.
 #### If you haven't flashed the baseplate yet
 Please install all required libraries:
- AsyncTCP
- ESPAsyncWebServer
- MFRC522 (by GithubCommunity)
- ArduinoMDNS (by Georg Kindl)
- ElegantOTA
- RtcDS1302 (or RTC by Makuna made by Michael C. Miller...)

  After extracting this repository and opening it on ArduinoIDE, please make sure that the value of '#define ELEGANTOTA_USE_ASYNC_WEB_SERVER' is 1 so the I/O file operations doesn't get interuptted if ever someone is uploading a file to the server while you're updating the server's firmware. After the initial flash, you won't need to ever touch the source code of this since the updates will be on the Releases section of this repository in which you can flash directly to fileserver/update or (your DNS or IP)/update. 
 
## Folder setup
  Don't think about this too much, just put the HTML with inline CSS and JS I provided on the src folder on root of your SD card on FAT32 filesystem since the ESP32 sketch will auto-generate the required folders. For wallpaper support, just name the .png or .jpg image into wallpaper.png. Favicons are also supported, just name it icon.png and for the cursor design, name the file into cur.png (NOTE: The dimensions of the .png you place WILL appear AS IS on the WebUI, therefore, I recommend you to downsize the desired cursor image/design into 64x64 to 32x32 px only based off your screen resolution)

## Updates for the server OS
  Since this has OTA using ~~<a href=https://github.com/LostInCompilation/PrettyOTA> PrettyOTA </a>~~ (Taken down by the DMCA with a coppyright strike and piracy allegations and deprecated it to avoid this repository being taken down too due to this repository is technically a fork of that repo) <a href="https://github.com/ayushsharma82/ElegantOTA">ElegantOTA</a>, the updates will be posted here in .bin file format on the Updates folder of this repository if you already flashed the version 2.0.0 for easier version control on your end
# LAST NOTES

## For the people who have 0 experience in coding and HTML
Contact me via E-mail that's provided on my profile

## For the developers who found this
Feel free to fork this repository and place it on a new branch (~~Please fix the DS1302 incompatibility issue~~) 

## For the person I sent here
I'll send you the custom HTML file with your own personalized touches (or not... HUH??)

## For the students of <a href=https://www.facebook.com/301338depedknnhs> Kinatakutan National High School </a> who read this...

Shoutout to y'all, this is actually a creation of the Asst. Secretary of the SSLG on S.Y. 2024-2025 and a G10 student. If you think this is impossible, it's isn't, just believe in yourself and first before these... learn to rename a file in 5 seconds and make a .docx within 45 mins of the ICT subject period so your class won't be like what class I came from who takes 2-3 days to make .docx and a good minute or two to rename and/or move files (ngl, I'm not a slow worker since I basically finish the tasks before them and had time before Sir Joseph's class to develop this)... and also to avoid the ICT teacher being mad or upset about some of your incompetent classmates who still don't know how to move objects in Word or just type the values that was supposed to be automated with COUNTA, AVERAGE and a bunch more API/Formula uses on Microsoft Excel
