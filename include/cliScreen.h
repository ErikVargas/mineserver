/*
   Copyright (c) 2011, Kasper F. Brandt
   All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the The Mineserver Project nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _CLISCREEN_H
#define _CLISCREEN_H

#include <string>
#include <vector>

#include "screenBase.h"
#include "logtype.h"

class CliScreen : public Screen
{
public:
  void init(std::string version);
  void log(LogType::LogType type, const std::string& source, const std::string& message);
  void updatePlayerList(std::vector<User*> users);
  void end();
  bool hasCommand();
  std::string getCommand();

  static bool CheckForCommand();
  static bool LogEvent(int type, const char* source, const char* message);

private:
  std::string currentCommand;
#ifdef _WIN32
  bool _hasCommand;
  static DWORD WINAPI _stdinThreadProc(LPVOID lpParameter);
  DWORD WINAPI stdinThreadProc();
  CRITICAL_SECTION ccAccess;
  HANDLE stdinThread;
#endif
};

#endif //_CLISCREEN_H
