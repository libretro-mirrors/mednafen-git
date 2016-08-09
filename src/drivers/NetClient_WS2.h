/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* NetClient_WS2.h:
**  Copyright (C) 2012-2016 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __MDFN_DRIVERS_NETCLIENT_WS2_H
#define __MDFN_DRIVERS_NETCLIENT_WS2_H

#include "NetClient.h"

class NetClient_WS2 : public NetClient
{
 public:

 NetClient_WS2();
 virtual ~NetClient_WS2();

 virtual void Connect(const char *host, unsigned int port);

 virtual void Disconnect(void);

 virtual bool IsConnected(void);

 virtual bool CanSend(int32 timeout = 0);
 virtual bool CanReceive(int32 timeout = 0);

 virtual uint32 Send(const void *data, uint32 len);

 virtual uint32 Receive(void *data, uint32 len);

 private:

 void *sd;
};

#endif
