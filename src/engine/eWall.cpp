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

#include "eWall.h"
#include "rRender.h"
#include "rTexture.h"
#include "eGameObject.h"
#include "eTess2.h"
#include "eGrid.h"

/* ***************************************************
   eWall:
   *************************************************** */

tHeap<eWallView> se_wallsVisible[MAX_VIEWERS];
// List<eWall> se_wallsNotYetInserted;

eWallView::~eWallView()
{
    RemoveFromHeap();
}

tHeapBase *eWallView::Heap() const{
    return &(se_wallsVisible[viewer]);
}


void eWallView::SetValue(REAL v){
    tHeapElement::SetVal( v, se_wallsVisible[viewer] );
}

eWall::eWall(eGrid *g):holder_(NULL), grid(g), flipped(0)
{
    id = -1;
    //Remove();
    //initialize viewers
    for(int i=MAX_VIEWERS-1;i>=0;i--){
        view[i].Set(i,this);
        se_wallsVisible[i].Insert(&view[i]);
    }
}

eWall::~eWall()
{
    if (holder_ )
        holder_->wall_ = NULL;

    for(int i=MAX_VIEWERS-1;i>=0;i--)
        se_wallsVisible[i].Remove(&view[i]);
    tCHECK_DEST;

    Insert();
}

eHalfEdge* eWall::Edge() const
{
    if ( holder_ )
    {
        return static_cast< eHalfEdge* >( holder_ );
    }
    else
    {
        return NULL;
    }
}

void eWall::CalcLen(){
    len = sqrt(Edge()->Vec().NormSquared());

    //  Insert();
}




#ifdef DEDICATED_XXX
void eWall::Render_helper(eWall *w,REAL tBeg,REAL tEnd,REAL h,REAL hfrac,REAL bot){
    glDisable(GL_CULL_FACE);

    const eCoord *p1 = &w->EndPoint(0);
    const eCoord *p2 = &w->EndPoint(1);

    BeginQuads();
    IsEdge(false);
    TexVertex(p1->x, p1->y, bot,
              tBeg        , hfrac);


    IsEdge(false);
    TexVertex(p1->x, p1->y, h*hfrac,
              tBeg        , 0);


    IsEdge(false);
    TexVertex(p2->x, p2->y, h*hfrac,
              0           , 0);

    TexVertex(p2->x, p2->y, bot,
              0           , hfrac);

    RenderEnd();

    if (TextureMode[rTEX_WALL]<0){
        Color(1,1,1);

        Line(p1->x,p1->y,h*hfrac,
             p2->x,p2->y,h*hfrac);

    }
}

void eWall::Render(){
    return;
    /*
    if (edge){
      const eCoord *p1 = &EndPoint(0);
      const eCoord *p2 = &EndPoint(1);
      
      Color(0,0,1,.5);
      
      eWall::Render_helper(this,(p1->x+p1->y)/40,(p2->x+p2->y)/40,4,1);
    }
    */
}
#endif

//ArmageTron_eWalltype eWall::type(){return ArmageTron_GENERIC_WALL;}

//void eWall::Flip(){}

bool eWall::Splittable() const {return 0;}
bool eWall::Deletable() const {return 0;}

void eWall::Split(eWall *& s1,eWall *& s2,REAL){
    s1=tNEW(eWall)(*this);
    s2=tNEW(eWall)(*this);
}

void eWall::SplitComplete(eWall *& s1,eWall *& s2,REAL a){
    /*
    if ( !edge )
    	return Split( s1, s2, a );

    eHalfEdge* other = edge->other();


     situation:

     *--------------*
     |			  /	|
     |			/	|
     |		e /		|
     |		/o		|
     |	  /			|
     |  /			|
     |/				|
     *--------------*

    grid->Check();
    */
}

void eWall::PassingGameObject(eGameObject *pass,REAL,REAL,int){
    pass->Kill();
}

void eWall::SplitByActive( eWall * oldWall )
{
    if ( oldWall )
        oldWall->SplitByPassive( this );
}

void eWall::SplitByPassive( eWall * newWall )
{
}

bool eWall::RunsParallelActive( eWall * oldWall )
{
    if ( oldWall )
        return oldWall->RunsParallelPassive( this );
    else
        return true;
}

bool eWall::RunsParallelPassive( eWall * newWall )
{
    return true;
}

void eWall::SetVisHeight(int v,REAL h){
    if (h>=SeeHeight()-EPS)
        view[v].SetValue(h);
    else
        view[v].SetValue(-1000);
}


void eWall::Insert(){
    if (grid)
        grid->wallsNotYetInserted.Remove(this,id);
}


void eWall::Remove(){
    if (grid)
        grid->wallsNotYetInserted.Add(this,id);
}

const eCoord& eWall::EndPoint(int i) const{
    eHalfEdge* edge = this->Edge();

    if (edge)
    {
        if (flipped != i)
            return *edge->Other()->Point();
        else
            return *(edge->Point());
    }
    else
        return se_zeroCoord;
}

eCoord eWall::Point(REAL a) const{
    eCoord beg = EndPoint(0);
    eCoord end = EndPoint(1);

    return beg + ( end - beg ) * a;
}

eCoord eWall::Vec() const{
    eHalfEdge* edge = this->Edge();

    if (edge)
        return (edge->Vec())*((1 - 2*flipped)*.5f);
    else
        return eCoord(0,0);
}

void eWallHolder::SetWall( eWall* wall )
{
    if ( wall_ )
    {
        wall_->holder_ = NULL;
    }

    wall_ = wall;

    if ( wall )
    {
        if ( wall->holder_ )
        {
            wall->holder_->wall_ = NULL;
        }

        wall->holder_ = this;
    }
}

eWall* eWallHolder::GetWall( void ) const
{
    return wall_;
}

eWallHolder::eWallHolder()
{
}

eWallHolder::~eWallHolder()
{
    if ( wall_ )
    {
        wall_->holder_ = NULL;
    }
}

eWall *eWallView::Belongs()
{
#ifdef CAUTION_WALL
    return wall;
#else
    static eWall* pwall = NULL;
    static eWall& wall = *pwall;

    // Z-Man: this code is still evil, but it seems to work. The proper way would be to enable CAUTION_WALL in the header, but that wastes 16 bytes per eWall. Can't go arount wasting memory for the sake of portability, can we?
    return reinterpret_cast<eWall *> (reinterpret_cast<char*>(&this[-viewer]) - reinterpret_cast<char*>(&wall.view[0]) + reinterpret_cast<char*>(&wall));
#endif
}

// *******************************************************************************************
// *
// *	OnBlocksCamera
// *
// *******************************************************************************************
//!
//! @param camera the camera that is blocked
//! @param height the maximal height the wall would be allowed to have without blocking the view
//!
// *******************************************************************************************

void eWall::OnBlocksCamera( eCamera * camera, REAL height ) const
{
}


