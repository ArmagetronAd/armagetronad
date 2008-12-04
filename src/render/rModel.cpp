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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#include "rModel.h"
#include <string>
#include <fstream>
#include <stdlib.h>
#include "rScreen.h"
#include "tString.h"
#include "tDirectories.h"
#include "tConfiguration.h"
#include "tLocale.h"
#include "rGL.h"
#include <string.h>

#define DONTDOIT
#include "rRender.h"

tCONFIG_ENUM(rDisplayListUsage);

static tConfItem<rDisplayListUsage> mod_udl("USE_DISPLAYLISTS", sr_useDisplayLists);

#ifndef DEDICATED

void Vec3::RenderVertex(){
    glVertex3f(x[0],x[1],x[2]);
}

void Vec3::RenderNormal(){
    glNormal3f(x[0],x[1],x[2]);
}
#endif

void rModel::Load(std::istream &in,const char *fileName){

#ifndef DEDICATED
    modelTexFacesCoherent = false;
    bool calc_normals=true;

    if (strstr(fileName,".mod")){ // old loader
        REAL xmax=-1E+32f, xmin=1E+32f;
        REAL zmax=-1E+32f, zmin=1E+32f;

        while (in && !in.eof()){
            char c;

            REAL x,y,z;
            int A,B,C;
            int num;

            in >> c;
            switch (c){
            case('v'):
                            in >> num >> x >> y >> z;

                if (x > xmax)
                    xmax = x;
                if (x < xmin)
                    xmin = x;
                if (z > zmax)
                    zmax = z;
                if (z < zmin)
                    zmin = z;

                vertices[num]=Vec3(x,y,z);
                normals[num]=Vec3(0,0,0);
                break;

            case('f'):
                            in >> A >> B >> C;
                modelFaces[modelFaces.Len()]=rModelFace(A,B,C);
                break;

            default:
                break;
            }
        }

        int i;
        for (i = vertices.Len()-1; i>=0; i--)
{
            Vec3 &v = vertices[i];
            texVert[i] = Vec3((v.x[0]-xmin)/(xmax-xmin), (zmax-v.x[2])/(zmax-zmin), 0);
        }
        modelTexFacesCoherent = true;
        for (i = modelFaces.Len()-1; i>=0; i--)
        {
            modelTexFaces[i] = modelFaces[i];
        }

    }
    else{ // new 3DSMax loader
        int offset=0;       //
        int current_vertex=0; //

#define MAXVERT 10000

        int   translate[MAXVERT];

        enum {VERTICES,FACES} status=VERTICES;

        int my_vert=1;

        while(in && !in.eof()){
            char c;
            char word[100];
            in >> c;
            if (c=='*'){
                in >> word;
                if (!strcmp(word,"MESH_TVERT")){
                    int n;
                    REAL x,y,z;
                    in >> n >> x >> y >> z;
                    texVert[n]=Vec3(x,1-y,z);
                }

                if (!strcmp(word,"MESH_TFACE")){
                    int n;
                    int a,b,c;
                    in >> n >> a >> b >> c;
                    modelTexFaces[n]=rModelFace(a,b,c);
                }

                if (!strcmp(word,"MESH_VERTEX")){
                    if (status==FACES){
                        status=VERTICES;
                        offset+=current_vertex+1;
                    }

                    float vec[3];
                    in >> current_vertex >> vec[0] >> vec[1] >> vec[2];
                    int realvert=current_vertex+offset;

                    translate[realvert]=-1;
                    /*
                      for(int i=realvert-1;i>0;i--){
                      float dist=0;
                      for(int j=2;j>=0;j--)
                      dist+= (vec[j]-vertices[i].x[j])*(vec[j]-vertices[i].x[j]);
                      if (dist<.1)
                      translate[realvert]=i;
                      }
                    */
                    if (translate[realvert]<0){
                        translate[realvert]=my_vert;
                        //std::cerr << "v " << my_vert;
                        for(int i=0;i<3;i++){
                            //std::cerr << '\t' << vec[i]*.025; // change inch to meters
                            vertices[my_vert].x[i]=vec[i]*.025;
                        }
                        //std::cerr << '\n';
                        my_vert++;
                    }
                }
                if (!strcmp(word,"MESH_FACE")){
                    status=FACES;
                    in >> word;
                    in >> word;
                    //std::cerr << "f ";

                    int face=modelFaces.Len();

                    if (strcmp(word,"A:")){
                        std::cerr << "wrong face format: expected A:, got " << word << '\n';
                        exit(-1);
                    }
                    int n;
                    in >> n;
                    modelFaces[face].A[0]=translate[n+offset];
                    //std::cerr << '\t' << translate[n+offset];

                    in >> word;
                    if (strcmp(word,"B:")){
                        std::cerr << "wrong face format: expected B:, got " << word << '\n';
                        exit(-1);
                    }
                    in >> n;
                    modelFaces[face].A[1]=translate[n+offset];
                    // std::cerr << '\t' << translate[n+offset];

                    in >> word;
                    if (strcmp(word,"C:")){
                        std::cerr << "wrong face format: expected C:, got " << word << '\n';
                        exit(-1);
                    }
                    in >> n;
                    modelFaces[face].A[2]=translate[n+offset];
                    // std::cerr << '\t' << translate[n+offset];

                    // std::cerr << '\n';

                    word[0]='\0';
                }

            }
        }
    }

    if (calc_normals)
        for(int i=modelFaces.Len()-1;i>=0;i--){
            Vec3 &A=vertices(modelFaces[i].A[0]);
            Vec3 &B=vertices(modelFaces[i].A[1]);
            Vec3 &C=vertices(modelFaces[i].A[2]);
            Vec3 X(B.x[0]-A.x[0],B.x[1]-A.x[1],B.x[2]-A.x[2]);
            Vec3 Y(C.x[0]-A.x[0],C.x[1]-A.x[1],C.x[2]-A.x[2]);
            Vec3 normal(X.x[1]*Y.x[2]-X.x[2]*Y.x[1],
                        X.x[2]*Y.x[0]-X.x[0]*Y.x[2],
                        X.x[0]*Y.x[1]-X.x[1]*Y.x[0]);
            normal=normal*(1/normal.Norm());

            for(int j=2;j>=0;j--)
                normals[modelFaces[i].A[j]]+=normal;
        }
    for(int i=normals.Len()-1;i>=0;i--){
        normals[i]=normals[i]*(1/normals[i].Norm());
    }

#endif
}

rModel::rModel(const char *fileName,const char *fileName_alt)
{
#ifndef DEDICATED
    //	tString s;
    //s << "models/";
    //	s << fileName;

    std::ifstream in;

    if ( !tDirectories::Data().Open( in, fileName ) )
    {
        if (fileName_alt){
            std::ifstream in2;
            tDirectories::Data().Open( in2, fileName_alt );

            if (!in2.good()){
                tERR_ERROR("\n\nModel file " << fileName_alt << " could not be found.\n" <<
                           "are you sure you are running " << tOutput("$program_name") << " in it's own directory?\n\n");
            }
            else
                Load(in2,fileName_alt);
        }
        else
            tERR_ERROR("\n\nModel file " << fileName << " could not be found.\n" <<
                       "are you sure you are running " << tOutput("$program_name") << " in it's own directory?\n\n");

    }
    else
        Load(in,fileName);
#endif
}

#ifndef DEDICATED
void rModel::Render(){
    if (!sr_glOut)
        return;
    if ( !displayList_.Call() )
    {
        // close pending glBegin() blocks
        RenderEnd();

        // model display lists should definitely be compiled before other lists
        rDisplayList::Cancel();

        bool texcoord=true;
        if (texVert.Len()<0)
            texcoord=false;
        if (modelTexFaces.Len()!=modelFaces.Len())
            texcoord=false;
        if ( !modelTexFacesCoherent )
            texcoord=false;

        if (texcoord)
        {
            glTexCoordPointer(3,GL_FLOAT,0,&texVert[0]);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }

        rDisplayListFiller filler( displayList_ );
           
        glEnable(GL_CULL_FACE);

        if ( !modelTexFacesCoherent )
        {
            // sigh, we need to do it the complicated way
            glBegin( GL_TRIANGLES );
            for(int i=modelFaces.Len()-1;i>=0;i--)
            {
                for(int j=0;j<=2;j++)
                {
                    if ( modelTexFaces.Len() > 0 )
                    {
                        glTexCoord3fv(reinterpret_cast<REAL *>(&(texVert(modelTexFaces(i).A[j]))));
                    }
                    if ( normals.Len() > 0 )
                    {
                        glNormal3fv(reinterpret_cast<REAL *>(&(normals(modelFaces(i).A[j]))));
                    }
                    glVertex3fv(reinterpret_cast<REAL *>(&(vertices(modelFaces(i).A[j]))));
                }
            }
            glEnd();

        }
        else
        {
            // glDrawElements works
            if (normals.Len()>=vertices.Len())
            {
                glNormalPointer(GL_FLOAT,0,&normals[0]);
                glEnableClientState(GL_NORMAL_ARRAY);
            }
            glVertexPointer(3,GL_FLOAT,0,&vertices[0]);
            glEnableClientState(GL_VERTEX_ARRAY);

            glDrawElements(GL_TRIANGLES,
                           modelFaces.Len()*3,
                           GL_UNSIGNED_INT,
                           &modelFaces(0));
        }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        glDisable(GL_CULL_FACE);
    }
}
#endif

rModel::~rModel(){
    tCHECK_DEST;
}





