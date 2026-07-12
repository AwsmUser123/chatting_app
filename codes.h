#ifndef __CODES_H
#define __CODES_H

#define CMD_ERROR       0x00 // Error code (with extra information).
#define CMD_INIT        0x01 // Initiate check sequence.
#define CMD_OK          0x02 // Successfully completed the requested operation.
#define CMD_REGISTER    0x03 // Registration data.
#define CMD_LOGIN       0x04 // Login data.
#define CMD_LOGOUT      0x05 // Log out.
#define CMD_CREATECHAT  0x06 // Create a chat.
#define CMD_JOINCHAT    0x07 // Join a chat.
#define CMD_LISTCHATS   0x08 // List all chats.
#define CMD_QUITCHAT    0x0a // Quit current chat.
#define CMD_SENDMSG     0x0b // Send a message.
#define CMD_LISTMSGS    0x0c // List all messages.
#define CMD_QUIT        0x10 // Quit.

#endif
