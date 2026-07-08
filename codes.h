#ifndef __CODES_H
#define __CODES_H

#define CMD_ERROR       0x00 // Error code (with extra information).
#define CMD_INIT        0x01 // Initiate check sequence.
#define CMD_OK          0x02 // Successfully completed the requested operation.
#define CMD_REGISTER    0x03 // Registration data.
#define CMD_LOGIN       0x05 // Login data.
#define CMD_LOGOUT      0x06 // Log out.
#define CMD_CREATECHAT  0x07 // Create a chat.
#define CMD_JOINCHAT    0x09 // Join a chat.
#define CMD_LISTCHATS   0x10 // List all chats.
#define CMD_QUITCHAT    0x0a // Quit current chat.
#define CMD_SENDMSG     0x0b // Send a message.
#define CMD_LISTMSGS    0x0d // List all messages.
#define CMD_QUIT        0x20 // Quit.

#endif
