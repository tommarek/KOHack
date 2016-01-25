# KOHack

KOHack is an automated farmer for KalOnline game, that sticks to an area where the bot is started. The bot is able to use Meds, walk and TP out in case of emergency.

The project consists of two parts - injector, that injects self-erasing code to the beginning of the dll file and allows us to load our own library with the hack from within a trusted library without HS noticing it. (HS only checks in-memory state of loaded dll libs). The second part is the bot itself that handles the packet crypting/decrypting/sending/recieving and the game logic.

All third party stuff was removed from the sources but you can find those if you try hard enough. Also InixSoft changes the order of packets (packets.h) quite often so current order is most definitly not correct. 

Special thanks to KoKuToru
