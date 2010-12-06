/*
   Copyright (c) 2010, The Mineserver Project
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

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <cmath>
#include <ctime>
#include <queue>

#include "logger.h"
#include "constants.h"

#include "config.h"
#include "nbt.h"
#include "map.h"

#include "lighting.h"

Lighting *Lighting::mLight;


bool Lighting::generateLight(int x, int z, sChunk *chunk)
{
  
  uint8 *blocks     = chunk->blocks;
  uint8 *skylight   = chunk->skylight;
  uint8 *blocklight = chunk->blocklight;
  uint8 *heightmap  = chunk->heightmap;

  uint64 *blocks64 = (uint64 *)blocks;
  uint32 *skylight32 = (uint32 *)skylight;
  uint8 meta,block,blockl,skyl;

  std::queue<lightInfo> lightQueue;

  int highest_y = 0;

  // Clear lightmaps
  memset(skylight, 0, 16*16*128/2);
  memset(blocklight, 0, 16*16*128/2);

  int light = 0;

  // Sky light 8 bytes per cycle search
  for(int block_x = 0; block_x < 16; block_x++)
  {
    for(int block_z = 0; block_z < 16; block_z++)
    {
      int blockx_blockz=((block_z << 7) + (block_x << 11))>>3;
      int absolute_x = (x<<4)+block_x;
      int absolute_z = (z<<4)+block_z;

      for(int block_y = (127/8)-1; block_y >= 0; block_y--)
      {
        int index      = block_y + blockx_blockz;
        uint64 block64    = blocks64[index];

        // if one of these 8 blocks is 
        if(block64 != 0)
        {
          //Iterate which of the 8 is the first non-air
          for(int i=7;i>=0;i--)
          {            
            //Set light value if air
            setLight(absolute_x, (block_y<<3)+i, absolute_z, 15,0, 1, chunk);
            light = 15;
            int block = blocks[(index<<3)+i];
            if(block != BLOCK_AIR)
            {
              lightQueue.push(lightInfo(absolute_x,(block_y<<3)+i+1,absolute_z,15));
              heightmap[block_z+(block_x<<4)] = (block_y<<3)+i;

              for(int block_yy = (block_y<<3)+i; block_yy >= 0; block_yy --)
              {
                light -= stopLight[block];
                if (light < 0) { light = 0; }
        
                if (light < 1)
                {
                  break;
                }
                setLight(absolute_x, block_yy, absolute_z, light, 0, 1, chunk);
                lightQueue.push(lightInfo(absolute_x,block_yy,absolute_z,light));
              }

              break;
            }            
          }
          
          if(heightmap[block_z+(block_x<<4)] > highest_y)
          {
            highest_y = heightmap[block_z+(block_x<<4)];
          }
          break;
        }
        //These 8 blocks are full lit
        skylight32[index] = 0xffffffff;        
      }
    }
  }

  spreadLight(&lightQueue,chunk);


  //Get light from border chunks
  for(int block_x = 0; block_x < 16; block_x+=15)
  {
    for(int block_z = 0; block_z < 16; block_z+=15)
    {
      int blockx_blockz=((block_z << 7) + (block_x << 11))>>3;
      int absolute_x = (x<<4)+block_x;
      int absolute_z = (z<<4)+block_z;

      int xdir=absolute_x;      
      int zdir=absolute_z;
      int skipdir=-1;

      //Which border?
      if(block_z == 0)       { zdir--; skipdir = 5;}
      else if(block_z == 15) { zdir++; skipdir = 4;}
      else if(block_x == 0)  { xdir--; skipdir = 3;}
      else if(block_x == 15) { xdir++; skipdir = 2;}


      for(int block_y = heightmap[block_z+(block_x<<4)]; block_y >= 0; block_y--)
      {
        if(Map::get()->getBlock(xdir,block_y,zdir,&block,&meta,false))
        {
          uint8 curblocklight,curskylight;
          if(getLight(xdir, block_y, zdir,&skyl,&blockl,chunk)                           && 
             getLight(absolute_x, block_y, absolute_z,&curskylight,&curblocklight,chunk))
          {
            if(skyl-1>curskylight)
            {
              if(skyl-stopLight[block] > 1)
              {
                lightQueue.push(lightInfo(absolute_x,block_y,absolute_z,skyl-stopLight[block]-1, skipdir));
                setLight(absolute_x, block_y, absolute_z, skyl-1, 0, 1, chunk);
              }            
            }
            if(blockl-1>curblocklight)
            {
              //ToDo: get blocklight from this chunk
            }
          }
        }
      }
    }
  }

  spreadLight(&lightQueue,chunk);
  

  
  return true;
}

bool Lighting::spreadLight(std::queue<lightInfo> *lightQueue, sChunk *chunk)
{
  uint8 meta,block,blockl,skyl;
    //Next up, skylight spreading  
  while(!lightQueue->empty())
  {
    lightInfo info=lightQueue->front();
    lightQueue->pop();
    
    for(int direction = 0; direction < 6; direction ++)
    {
      int xdir=info.x;
      int ydir=info.y;
      int zdir=info.z;
      int skipdir=-1;

      //If we came from this direction, skip
      if(direction == info.skipdir) continue;

      switch(direction)
      {
        case 0: ydir--; skipdir=1; break;
        case 1: ydir++; skipdir=0; break;
        case 2: xdir--; skipdir=3; break;
        case 3: xdir++; skipdir=2; break;
        case 4: zdir--; skipdir=5; break;
        case 5: zdir++; skipdir=4; break;
      }
      //Going too high
      if(ydir == 128) continue;
      
      //Stop of this block light value already higher
      if(getLight(xdir, ydir, zdir,&skyl,&blockl,chunk) && skyl<info.light-1)
      {        
        if(Map::get()->getBlock(xdir,ydir,zdir,&block,&meta,false))
        {
          //If still light left, generate for this block also!
          if(info.light-stopLight[block] > 1)
          {
            lightQueue->push(lightInfo(xdir,ydir,zdir,info.light-stopLight[block]-1,skipdir));
          }
          setLight(xdir, ydir, zdir, info.light-1, 0, 1, chunk);          
        }
      }
    }
  }
  return true;
}

// Light get/set
bool Lighting::getLight(int x, int y, int z, uint8 *skylight, uint8 *blocklight, sChunk *chunk)
{
  Map::get()->getLight(x,y,z,skylight,blocklight,chunk);
  return true;
}

bool Lighting::setLight(int x, int y, int z, int skylight, int blocklight, int setLight, sChunk *chunk)
{
  Map::get()->setLight(x,y,z,skylight,blocklight,setLight,chunk);
  return true;
}
