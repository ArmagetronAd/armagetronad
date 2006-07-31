/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#include <iostream>
#include <ctype.h>
#include <string>

int main(){
    float sk[4][4]={{0,.1,0,0},
                    {-.1,0,0,0},
                    {0,0,.1,0},
                    //{1,.2,-1.05,1}};
                    {1/.025,.2/.025,-1.05/.025,1}};

    float sn[3][3]={{0,1,0},
                    {-1,0,0},
                    {0,0,1}};

    while(!std::cin.eof() && std::cin.good()){
        char name[1000];
        std::cin >> name;
        std::cout << name;

        float in[4];
        int n;

        if (!strcmp("*MESH_VERTEX",name)){
            std::cin >> n >> in[0] >> in[1] >> in[2];
            in[3]=1;
            std::cout << " " << n;
            int i,j;

            for(i=0;i<3;i++){
                float x=0;
                for(j=0;j<4;j++)
                    x+=sk[j][i]*in[j];
                std::cout << " " << x;
            }
        }

        if (!strcmp("*MESH_FACENORMAL",name)){
            std::cin >> n >> in[0] >> in[1] >> in[2];
            in[3]=1;
            std::cout << " " << n;
            int i,j;

            for(i=0;i<3;i++){
                float x=0;
                for(j=0;j<3;j++)
                    x+=sn[j][i]*in[j];
                std::cout << " " << x;
            }
        }

        char c;
        std::cin.get(c);
        while (isspace(c) && !std::cin.eof() && std::cin.good()){
            std::cout.put(c);
            std::cin.get(c);
        }
        std::cin.unget();
    }
}
